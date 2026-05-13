#include "UsbMscOffload.h"
#include <Adafruit_TinyUSB.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/generic_hardware/DebugStream.h"

// DebugStream that captures formatted output into a caller-supplied buffer
// instead of writing to a serial port. Used by buildRow() to reuse the
// project's existing print-log functions without going through snprintf %f
// (which silently drops floats on SAMD21 newlib-nano).
class BufferDebugStream final : public DebugStream {
public:
    void bind(char* buf, uint16_t cap) { m_buf = buf; m_cap = cap; m_len = 0; }
    uint16_t length() const { return m_len; }

protected:
    size_t write(const void* buffer, size_t size) override {
        if (m_buf == nullptr) return 0;
        uint16_t toCopy = (uint16_t)size;
        if ((uint32_t)m_len + toCopy > m_cap) toCopy = (uint16_t)(m_cap - m_len);
        memcpy(m_buf + m_len, buffer, toCopy);
        m_len = (uint16_t)(m_len + toCopy);
        return toCopy;
    }

private:
    char*    m_buf = nullptr;
    uint16_t m_cap = 0;
    uint16_t m_len = 0;
};

// ============================ Disk Geometry ============================
//
// FAT16, 128 MB volume, 8 KB clusters, two FATs. Geometry chosen so the
// cluster count stays inside the FAT16 range and Windows is willing to do
// full-volume reads without truncating long copies.

namespace {

constexpr uint16_t SECTOR_SIZE         = 512;
constexpr uint8_t  SECTORS_PER_CLUSTER = 16;                                        // 8 KB clusters
constexpr uint32_t CLUSTER_BYTES       = (uint32_t)SECTOR_SIZE * SECTORS_PER_CLUSTER;
constexpr uint8_t  NUM_FATS            = 2;
constexpr uint16_t RESERVED_SECTORS    = 1;
constexpr uint16_t ROOT_DIR_ENTRIES    = 64;
constexpr uint16_t ROOT_DIR_SECTORS    = (ROOT_DIR_ENTRIES * 32) / SECTOR_SIZE;     // 4
constexpr uint32_t TOTAL_SECTORS       = 262144;                                    // 128 MB
constexpr uint16_t SECTORS_PER_FAT     = 64;                                        // covers ~16k clusters
constexpr uint32_t FAT_START           = RESERVED_SECTORS;
constexpr uint32_t ROOT_DIR_START      = FAT_START + (uint32_t)NUM_FATS * SECTORS_PER_FAT;
constexpr uint32_t DATA_START          = ROOT_DIR_START + ROOT_DIR_SECTORS;

constexpr uint16_t TSV_ROW_SIZE = 192;
constexpr char VOLUME_LABEL[11] = {'N','U','L','I',' ','L','O','G','S',' ',' '};

constexpr char BOOT_MARKER[]      = "Logger setup";
constexpr uint8_t BOOT_MARKER_LEN = sizeof(BOOT_MARKER) - 1;  // strlen, no NUL

// Max flash entry size we'll accept (= SillyGooseLogData + 1 with comfortable margin).
constexpr uint16_t MAX_ENTRY_SIZE = 128;

// Cap on number of flights we'll surface. Older flights are dropped.
constexpr uint8_t MAX_FLIGHTS = 32;
constexpr uint8_t MAX_FILES   = MAX_FLIGHTS + 1;  // +1 for CONFIG.TXT

// ============================ Config File (placeholder) ============================
//
// Not wired to the real Configuration object yet. Edits make it back into the
// in-memory ConfigValues struct via parseConfig() and are echoed to Serial so
// the round-trip can be observed.

constexpr uint16_t CONFIG_CAPACITY = (uint16_t)CLUSTER_BYTES;
char     g_configBuf[CONFIG_CAPACITY];
volatile bool g_configDirty = false;

struct ConfigValues {
    char     boardName[24];
    uint32_t drogueDelayMs;
    float    mainElevationM;
    float    voltageScale;
    uint32_t pyroFireDurationMs;
    bool     enableBuzzer;
};

ConfigValues g_config = {"SillyGoose", 1000, 150.0f, 0.00644f, 1000, true};

void renderConfig() {
    int n = snprintf(g_configBuf, CONFIG_CAPACITY,
                     "# SillyGoose USB config (placeholder -- not yet wired to flight config)\r\n"
                     "boardName=%s\r\n"
                     "drogueDelayMs=%lu\r\n"
                     "mainElevationM=%.2f\r\n"
                     "voltageScale=%.6f\r\n"
                     "pyroFireDurationMs=%lu\r\n"
                     "enableBuzzer=%s\r\n",
                     g_config.boardName,
                     (unsigned long)g_config.drogueDelayMs,
                     g_config.mainElevationM, g_config.voltageScale,
                     (unsigned long)g_config.pyroFireDurationMs,
                     g_config.enableBuzzer ? "true" : "false");
    if (n < 0) n = 0;
    while (n < (int)CONFIG_CAPACITY - 2) { g_configBuf[n++] = '\r'; g_configBuf[n++] = '\n'; }
}

void trimEol(char* s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' ||
                       s[len - 1] == '\r' || s[len - 1] == '\n')) {
        s[--len] = '\0';
    }
}

void parseConfig(const char* text, uint32_t len) {
    uint32_t i = 0;
    while (i < len) {
        const uint32_t lineStart = i;
        while (i < len && text[i] != '\n' && text[i] != '\r') i++;
        const uint32_t lineEnd = i;
        while (i < len && (text[i] == '\r' || text[i] == '\n')) i++;
        if (lineEnd == lineStart || text[lineStart] == '#') continue;
        uint32_t eq = lineStart;
        while (eq < lineEnd && text[eq] != '=') eq++;
        if (eq == lineEnd) continue;
        char key[32] = {0};
        char val[64] = {0};
        uint32_t kLen = eq - lineStart;
        if (kLen >= sizeof(key)) kLen = sizeof(key) - 1;
        memcpy(key, text + lineStart, kLen);
        uint32_t vLen = lineEnd - eq - 1;
        if (vLen >= sizeof(val)) vLen = sizeof(val) - 1;
        memcpy(val, text + eq + 1, vLen);
        trimEol(key);
        trimEol(val);
        if      (strcmp(key, "boardName")          == 0) { strncpy(g_config.boardName, val, sizeof(g_config.boardName) - 1); g_config.boardName[sizeof(g_config.boardName) - 1] = '\0'; }
        else if (strcmp(key, "drogueDelayMs")      == 0) g_config.drogueDelayMs      = strtoul(val, nullptr, 10);
        else if (strcmp(key, "mainElevationM")     == 0) g_config.mainElevationM     = (float)atof(val);
        else if (strcmp(key, "voltageScale")       == 0) g_config.voltageScale       = (float)atof(val);
        else if (strcmp(key, "pyroFireDurationMs") == 0) g_config.pyroFireDurationMs = strtoul(val, nullptr, 10);
        else if (strcmp(key, "enableBuzzer")       == 0) g_config.enableBuzzer       = (strcmp(val, "true") == 0 || strcmp(val, "1") == 0);
    }
}

// ============================ Singleton state ============================

struct VFile {
    char     name83[11];           // 8.3 packed, space-padded
    uint32_t startCluster;
    uint32_t fileSize;
    uint32_t numClusters;
    uint8_t  attr;                 // FAT directory attribute
    uint32_t flightStartEntry;     // first flash entry index of this flight (TSV files only)
    uint32_t flightRowCount;       // entries to render as rows (TSV files only)
};

VFile     g_files[MAX_FILES];
uint8_t   g_fileCount = 0;

FlashMemory*                  g_flash      = nullptr;
uint16_t                      g_entrySize  = 0;
const char*                   g_header     = nullptr;
UsbMscOffload::RowFormatterFn g_formatter  = nullptr;

Adafruit_USBD_MSC g_usbMsc;

// ============================ Flash scan ============================

// Binary search for the first entry whose id byte is 0xFF.
uint32_t findEndOfLog() {
    const uint32_t numEntries = g_flash->getMemorySizeBytes() / g_entrySize;
    uint32_t left = 0;
    uint32_t right = numEntries;
    while (left < right) {
        const uint32_t mid = left + (right - left) / 2;
        const uint8_t id = g_flash->read((uint32_t)mid * g_entrySize);
        if (id == 0xFF) right = mid;
        else            left  = mid + 1;
    }
    return left;
}

// Returns true if the entry at entryIdx is a "Logger setup" boot marker
// (id byte == 0x03 plus the literal string in the data area). Called only
// when we've already seen id == 0x03, so we only pay the extra read on
// message entries (which are rare).
bool isBootMarker(uint32_t entryIdx) {
    char buf[BOOT_MARKER_LEN];
    g_flash->read((uint32_t)entryIdx * g_entrySize + 1, (uint8_t*)buf, BOOT_MARKER_LEN);
    return memcmp(buf, BOOT_MARKER, BOOT_MARKER_LEN) == 0;
}

void addCfgFileEntry(uint32_t& cluster) {
    static const char cfgName[11] = {'C','O','N','F','I','G',' ',' ','T','X','T'};
    VFile& f = g_files[0];
    memcpy(f.name83, cfgName, 11);
    f.fileSize         = CONFIG_CAPACITY;
    f.numClusters      = 1;
    f.startCluster     = cluster;
    f.attr             = 0x20;                 // ARCHIVE -- writable
    f.flightStartEntry = 0;
    f.flightRowCount   = 0;
    g_fileCount        = 1;
    cluster           += f.numClusters;
}

void addFlightFileEntry(uint32_t& cluster, uint32_t startEntry, uint32_t rowCount) {
    if (g_fileCount >= MAX_FILES) return;
    if (rowCount == 0) return;
    const uint32_t fileSize    = (rowCount + 1u) * TSV_ROW_SIZE;        // +1 for header row
    const uint32_t numClusters = (fileSize + CLUSTER_BYTES - 1) / CLUSTER_BYTES;
    VFile& f = g_files[g_fileCount];
    memset(f.name83, ' ', 11);
    const uint8_t n = g_fileCount;                                       // 1..NUM_FLIGHTS
    memcpy(f.name83, "FLIGHT", 6);
    f.name83[6] = '0' + (char)(n / 10);
    f.name83[7] = '0' + (char)(n % 10);
    f.name83[8] = 'T'; f.name83[9] = 'S'; f.name83[10] = 'V';
    f.fileSize         = fileSize;
    f.numClusters      = numClusters;
    f.startCluster     = cluster;
    f.attr             = 0x21;                 // READ_ONLY | ARCHIVE
    f.flightStartEntry = startEntry;
    f.flightRowCount   = rowCount;
    cluster           += numClusters;
    g_fileCount++;
}

void scanFlights() {
    uint32_t cluster = 2;
    addCfgFileEntry(cluster);

    if (g_flash == nullptr || g_entrySize == 0) return;

    const uint32_t endOfLog = findEndOfLog();
    if (endOfLog == 0) return;

    // First pass: walk the log, marking flight boundaries at each boot marker.
    // We keep only the most recent MAX_FLIGHTS by recording (start,end) pairs
    // and committing them at the end so older flights drop off if there are
    // too many. For typical flash usage there's just a handful of boots.

    constexpr uint8_t MAX_BOOTS = MAX_FLIGHTS + 1;  // +1 buffer slot
    uint32_t bootStart[MAX_BOOTS];
    uint8_t  bootCount = 0;
    bootStart[bootCount++] = 0;  // implicit "flight 1" starts at entry 0 even
                                 // if the boot marker is a few entries in.

    for (uint32_t i = 0; i < endOfLog; ++i) {
        const uint8_t id = g_flash->read((uint32_t)i * g_entrySize);
        if (id == 0x03 && isBootMarker(i)) {
            if (i == 0) {
                // boot marker at index 0 -- bootStart[0] already points here
                continue;
            }
            if (bootCount < MAX_BOOTS) {
                bootStart[bootCount++] = i;
            } else {
                // shift left, drop oldest
                for (uint8_t k = 1; k < MAX_BOOTS; ++k) bootStart[k - 1] = bootStart[k];
                bootStart[MAX_BOOTS - 1] = i;
            }
        }
    }

    // Commit flights. Each flight spans [bootStart[k], bootStart[k+1]) (or
    // endOfLog for the last one).
    for (uint8_t k = 0; k < bootCount; ++k) {
        const uint32_t start = bootStart[k];
        const uint32_t end   = (k + 1 < bootCount) ? bootStart[k + 1] : endOfLog;
        addFlightFileEntry(cluster, start, end - start);
    }
}

VFile* findFileForCluster(uint32_t cluster) {
    for (uint8_t i = 0; i < g_fileCount; ++i) {
        if (cluster >= g_files[i].startCluster &&
            cluster <  g_files[i].startCluster + g_files[i].numClusters) {
            return &g_files[i];
        }
    }
    return nullptr;
}

// ============================ Row + sector synthesis ============================

void buildRow(uint32_t fileIdx, uint32_t rowIdx, char* out) {
    memset(out, ' ', TSV_ROW_SIZE);
    out[TSV_ROW_SIZE - 2] = '\r';
    out[TSV_ROW_SIZE - 1] = '\n';

    char tmp[TSV_ROW_SIZE];
    BufferDebugStream stream;
    stream.bind(tmp, TSV_ROW_SIZE);

    if (rowIdx == 0) {
        // Header is plain text; emit verbatim through the stream so the
        // capture path is uniform with data rows.
        if (g_header != nullptr) stream.data("%s", g_header);
    } else {
        const VFile& f = g_files[fileIdx];
        const uint32_t entryIdx = f.flightStartEntry + (rowIdx - 1);
        uint8_t entry[MAX_ENTRY_SIZE];
        if (g_entrySize > MAX_ENTRY_SIZE) return;
        g_flash->read((uint32_t)entryIdx * g_entrySize, entry, g_entrySize);
        if (g_formatter != nullptr) {
            g_formatter(entry, &stream);
        }
    }

    // DebugStream::data() appends a trailing '\n'; strip it so the row's own
    // CRLF terminator (already in `out`) is the only line ending.
    uint16_t n = stream.length();
    while (n > 0 && (tmp[n - 1] == '\n' || tmp[n - 1] == '\r')) n--;
    if (n > TSV_ROW_SIZE - 2) n = TSV_ROW_SIZE - 2;
    memcpy(out, tmp, n);
}

void readFlightTsv(uint32_t fileIdx, uint32_t offset, uint8_t* buf, uint32_t len) {
    const VFile& f = g_files[fileIdx];
    const uint32_t totalBytes = (f.flightRowCount + 1u) * TSV_ROW_SIZE;
    uint32_t pos = 0;
    while (pos < len) {
        const uint32_t abs = offset + pos;
        if (abs >= totalBytes) {
            memset(buf + pos, 0, len - pos);
            return;
        }
        const uint32_t rowIdx = abs / TSV_ROW_SIZE;
        const uint32_t rowOff = abs - rowIdx * TSV_ROW_SIZE;
        char row[TSV_ROW_SIZE];
        buildRow(fileIdx, rowIdx, row);
        uint32_t toCopy = TSV_ROW_SIZE - rowOff;
        if (toCopy > (len - pos)) toCopy = (len - pos);
        memcpy(buf + pos, row + rowOff, toCopy);
        pos += toCopy;
    }
}

void readBootSector(uint8_t* buf) {
    memset(buf, 0, SECTOR_SIZE);
    buf[0] = 0xEB; buf[1] = 0x3C; buf[2] = 0x90;
    memcpy(buf + 3, "MSDOS5.0", 8);
    buf[11] = SECTOR_SIZE & 0xFF;      buf[12] = (SECTOR_SIZE >> 8) & 0xFF;
    buf[13] = SECTORS_PER_CLUSTER;
    buf[14] = RESERVED_SECTORS & 0xFF; buf[15] = (RESERVED_SECTORS >> 8) & 0xFF;
    buf[16] = NUM_FATS;
    buf[17] = ROOT_DIR_ENTRIES & 0xFF; buf[18] = (ROOT_DIR_ENTRIES >> 8) & 0xFF;
    if (TOTAL_SECTORS <= 0xFFFFu) {
        buf[19] = TOTAL_SECTORS & 0xFF; buf[20] = (TOTAL_SECTORS >> 8) & 0xFF;
    }
    buf[21] = 0xF8;
    buf[22] = SECTORS_PER_FAT & 0xFF;  buf[23] = (SECTORS_PER_FAT >> 8) & 0xFF;
    buf[24] = 0x3F; buf[25] = 0x00;
    buf[26] = 0xFF; buf[27] = 0x00;
    if (TOTAL_SECTORS > 0xFFFFu) {
        buf[32] = TOTAL_SECTORS         & 0xFF;
        buf[33] = (TOTAL_SECTORS >>  8) & 0xFF;
        buf[34] = (TOTAL_SECTORS >> 16) & 0xFF;
        buf[35] = (TOTAL_SECTORS >> 24) & 0xFF;
    }
    buf[36] = 0x80;
    buf[38] = 0x29;
    buf[39] = 0x12; buf[40] = 0x34; buf[41] = 0x56; buf[42] = 0x78;
    memcpy(buf + 43, VOLUME_LABEL, 11);
    memcpy(buf + 54, "FAT16   ", 8);
    buf[510] = 0x55; buf[511] = 0xAA;
}

void readFatSector(uint32_t fatSectorIdx, uint8_t* buf) {
    memset(buf, 0, SECTOR_SIZE);
    constexpr uint32_t entriesPerSector = SECTOR_SIZE / 2;
    const uint32_t startEntry = fatSectorIdx * entriesPerSector;
    for (uint32_t i = 0; i < entriesPerSector; ++i) {
        const uint32_t entry = startEntry + i;
        uint16_t v = 0x0000;
        if (entry == 0)      v = 0xFFF8;
        else if (entry == 1) v = 0xFFFF;
        else {
            const VFile* f = findFileForCluster(entry);
            if (f != nullptr) {
                const uint32_t last = f->startCluster + f->numClusters - 1;
                v = (entry == last) ? 0xFFFF : (uint16_t)(entry + 1);
            }
        }
        buf[i * 2]     =  v       & 0xFF;
        buf[i * 2 + 1] = (v >> 8) & 0xFF;
    }
}

void writeDirEntry(uint8_t* slot, const char name83[11], uint8_t attr,
                   uint32_t cluster, uint32_t size) {
    memcpy(slot, name83, 11);
    slot[11] = attr;
    slot[20] = (cluster >> 16) & 0xFF;
    slot[21] = (cluster >> 24) & 0xFF;
    slot[26] =  cluster        & 0xFF;
    slot[27] = (cluster >>  8) & 0xFF;
    slot[28] =  size           & 0xFF;
    slot[29] = (size >>  8)    & 0xFF;
    slot[30] = (size >> 16)    & 0xFF;
    slot[31] = (size >> 24)    & 0xFF;
}

void readRootDirSector(uint32_t dirSectorIdx, uint8_t* buf) {
    memset(buf, 0, SECTOR_SIZE);
    constexpr uint32_t entriesPerSector = SECTOR_SIZE / 32;
    const uint32_t startEntry = dirSectorIdx * entriesPerSector;
    for (uint32_t e = 0; e < entriesPerSector; ++e) {
        const uint32_t entryIdx = startEntry + e;
        uint8_t* slot = buf + e * 32;
        if (entryIdx == 0) {
            memcpy(slot, VOLUME_LABEL, 11);
            slot[11] = 0x08; // VOLUME_ID
        } else if (entryIdx - 1 < g_fileCount) {
            const VFile& f = g_files[entryIdx - 1];
            writeDirEntry(slot, f.name83, f.attr, f.startCluster, f.fileSize);
        } else {
            return;
        }
    }
}

void readDataSector(uint32_t lba, uint8_t* buf) {
    memset(buf, 0, SECTOR_SIZE);
    const uint32_t dataLba         = lba - DATA_START;
    const uint32_t cluster         = 2u + (dataLba / SECTORS_PER_CLUSTER);
    const uint32_t sectorInCluster = dataLba % SECTORS_PER_CLUSTER;
    VFile* f = findFileForCluster(cluster);
    if (f == nullptr) return;

    const uint32_t offsetInFile =
        (cluster - f->startCluster) * CLUSTER_BYTES + sectorInCluster * SECTOR_SIZE;

    if (f == &g_files[0]) {
        // CONFIG.TXT
        if (offsetInFile < CONFIG_CAPACITY) {
            uint32_t toCopy = SECTOR_SIZE;
            if (offsetInFile + toCopy > CONFIG_CAPACITY) toCopy = CONFIG_CAPACITY - offsetInFile;
            memcpy(buf, g_configBuf + offsetInFile, toCopy);
        }
    } else {
        const uint32_t fileIdx = (uint32_t)(f - g_files);
        readFlightTsv(fileIdx, offsetInFile, buf, SECTOR_SIZE);
    }
}

// ============================ TinyUSB callbacks ============================

int32_t msc_read_cb(uint32_t lba, void* buffer, uint32_t bufsize) {
    uint8_t* out = (uint8_t*)buffer;
    const uint32_t sectors = bufsize / SECTOR_SIZE;
    for (uint32_t i = 0; i < sectors; ++i) {
        const uint32_t cur = lba + i;
        uint8_t* dst = out + i * SECTOR_SIZE;
        if (cur == 0) {
            readBootSector(dst);
        } else if (cur >= FAT_START && cur < FAT_START + (uint32_t)NUM_FATS * SECTORS_PER_FAT) {
            readFatSector((cur - FAT_START) % SECTORS_PER_FAT, dst);
        } else if (cur >= ROOT_DIR_START && cur < ROOT_DIR_START + ROOT_DIR_SECTORS) {
            readRootDirSector(cur - ROOT_DIR_START, dst);
        } else if (cur >= DATA_START && cur < TOTAL_SECTORS) {
            readDataSector(cur, dst);
        } else {
            memset(dst, 0, SECTOR_SIZE);
        }
    }
    return (int32_t)bufsize;
}

int32_t msc_write_cb(uint32_t lba, uint8_t* buffer, uint32_t bufsize) {
    const uint32_t sectors = bufsize / SECTOR_SIZE;
    for (uint32_t i = 0; i < sectors; ++i) {
        const uint32_t cur = lba + i;
        // Only persist writes that land in CONFIG.TXT's cluster. Everything
        // else (FAT scratching, OS metadata) is silently accepted.
        if (cur >= DATA_START && cur < TOTAL_SECTORS) {
            const uint32_t dataLba         = cur - DATA_START;
            const uint32_t cluster         = 2u + (dataLba / SECTORS_PER_CLUSTER);
            const uint32_t sectorInCluster = dataLba % SECTORS_PER_CLUSTER;
            const VFile* f = findFileForCluster(cluster);
            if (f == &g_files[0]) {
                const uint32_t off =
                    (cluster - f->startCluster) * CLUSTER_BYTES + sectorInCluster * SECTOR_SIZE;
                if (off < CONFIG_CAPACITY) {
                    uint32_t toCopy = SECTOR_SIZE;
                    if (off + toCopy > CONFIG_CAPACITY) toCopy = CONFIG_CAPACITY - off;
                    memcpy(g_configBuf + off, buffer + i * SECTOR_SIZE, toCopy);
                    g_configDirty = true;
                }
            }
        }
    }
    return (int32_t)bufsize;
}

void msc_flush_cb(void) {
    if (g_configDirty) {
        g_configDirty = false;
        parseConfig(g_configBuf, CONFIG_CAPACITY);
        Serial.print("[CONFIG] parsed: boardName='");
        Serial.print(g_config.boardName);
        Serial.print("' drogueDelayMs=");
        Serial.print(g_config.drogueDelayMs);
        Serial.print(" mainElevationM=");
        Serial.print(g_config.mainElevationM);
        Serial.print(" voltageScale=");
        Serial.print(g_config.voltageScale, 6);
        Serial.print(" pyroFireDurationMs=");
        Serial.print(g_config.pyroFireDurationMs);
        Serial.print(" enableBuzzer=");
        Serial.println(g_config.enableBuzzer ? "true" : "false");
    }
}

}  // namespace

// ============================ Public class ============================

UsbMscOffload::UsbMscOffload(FlashMemory* flash, uint16_t entrySize,
                             const char* tsvHeader, RowFormatterFn formatter) {
    g_flash     = flash;
    g_entrySize = entrySize;
    g_header    = tsvHeader;
    g_formatter = formatter;
}

void UsbMscOffload::begin() {
    renderConfig();
    scanFlights();

    g_usbMsc.setID("NULI", "SillyGoose", "1.0");
    g_usbMsc.setCapacity(TOTAL_SECTORS, SECTOR_SIZE);
    g_usbMsc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);
    g_usbMsc.setUnitReady(true);
    g_usbMsc.begin();
}
