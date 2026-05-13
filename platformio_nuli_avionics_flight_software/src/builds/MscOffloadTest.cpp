// Minimal standalone test build:
//   - Enumerates as a USB Mass Storage device using TinyUSB (Adafruit fork)
//   - Synthesizes a FAT16 filesystem entirely in code (no real storage)
//   - Exposes:
//       CONFIG.TXT   - editable text config; writes are parsed back into RAM
//       FLIGHT01.TSV - small  TSV log, generated on the fly
//       FLIGHT02.TSV - medium TSV log, generated on the fly
//       FLIGHT03.TSV - large  TSV log, generated on the fly
//
// Nothing here depends on the rest of the avionics codebase; the goal is to
// prove the USB plumbing works before integrating with real flash + config.

#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================ Disk Geometry ============================
//
// FAT16, 256 MB volume, 8 KB clusters. Cluster count (~32k) stays inside the
// FAT16 range (>4084, <65525). 8 KB clusters keep the FAT itself small while
// leaving room for the big stress-test flight file.

static constexpr uint16_t SECTOR_SIZE         = 512;
static constexpr uint8_t  SECTORS_PER_CLUSTER = 16;                                      // 8 KB clusters
static constexpr uint32_t CLUSTER_BYTES       = (uint32_t)SECTOR_SIZE * SECTORS_PER_CLUSTER;
static constexpr uint8_t  NUM_FATS            = 2;                                       // conventional for FAT16; some hosts get picky with 1
static constexpr uint16_t RESERVED_SECTORS    = 1;
static constexpr uint16_t ROOT_DIR_ENTRIES    = 64;
static constexpr uint16_t ROOT_DIR_SECTORS    = (ROOT_DIR_ENTRIES * 32) / SECTOR_SIZE;   // 4
static constexpr uint32_t TOTAL_SECTORS       = 262144;                                  // 128 MB
static constexpr uint16_t SECTORS_PER_FAT     = 64;                                      // covers ~16k clusters
static constexpr uint32_t FAT_START           = RESERVED_SECTORS;
static constexpr uint32_t ROOT_DIR_START      = FAT_START + (uint32_t)NUM_FATS * SECTORS_PER_FAT;
static constexpr uint32_t DATA_START          = ROOT_DIR_START + ROOT_DIR_SECTORS;

static const char VOLUME_LABEL[11] = {'S','I','L','L','Y','G','O','O','S','E',' '};

// ============================ TSV Log Generator ============================
//
// Rows are emitted at a fixed width so that any byte offset within the file
// maps directly to (rowIndex, byteInRow). That lets us serve arbitrary
// sector reads without having to "stream" through the whole file.

static constexpr uint16_t TSV_ROW_SIZE = 160;  // bytes per row including trailing \r\n
static const char TSV_HEADER_TEXT[] =
    "timestampMs\tpressurePa\tbarometerTempK\taccelX\taccelY\taccelZ"
    "\tgyroX\tgyroY\tgyroZ\taltitudeM\tvelocityMS\tflightState";

// Integer-only TSV generator: SAMD21 has no FPU, so float printf + sin/cos
// take milliseconds per row -- way too slow to satisfy the host on a multi-MB
// transfer. All values here are pseudo-physical integers in their native or
// "milli-" units (e.g. milli-Pa, milli-g, mm/s) so snprintf only has to format
// integers.
static void buildTsvRow(uint32_t flightSeed, uint32_t totalRows, uint32_t rowIdx, char* out) {
    memset(out, ' ', TSV_ROW_SIZE);
    out[TSV_ROW_SIZE - 2] = '\r';
    out[TSV_ROW_SIZE - 1] = '\n';

    char tmp[TSV_ROW_SIZE];
    int n = 0;
    if (rowIdx == 0) {
        n = snprintf(tmp, sizeof(tmp), "%s", TSV_HEADER_TEXT);
    } else {
        const uint32_t i     = rowIdx - 1;
        const uint32_t s     = flightSeed;
        const uint32_t t_ms  = i * 10u;                              // 100 Hz timeline
        const int32_t  p_mPa = 101325000L - (int32_t)i;              // milli-Pa, slowly drops
        const int32_t  k_mK  = (int32_t)(295000u + (i % 1000u));     // milli-K
        const int32_t  ax_mg = (int32_t)((i * 31u + s)        % 4000u) - 2000;
        const int32_t  ay_mg = (int32_t)((i * 37u + s * 3u)   % 4000u) - 2000;
        const int32_t  az_mg = (int32_t)((i * 41u + s * 5u)   % 2000u) + 8800;
        const int32_t  gx_uds = (int32_t)((i * 53u + s)       % 200000u) - 100000;
        const int32_t  gy_uds = (int32_t)((i * 59u + s)       % 200000u) - 100000;
        const int32_t  gz_uds = (int32_t)((i * 61u + s)       % 200000u) - 100000;
        const int32_t  alt_mm = (int32_t)((i * 67u + s)       % 500000u);
        const int32_t  vel_mms = (int32_t)((i * 71u + s)      % 100000u) - 50000;
        const int      state   = (int)((i * 4u) / (totalRows ? totalRows : 1u));

        n = snprintf(tmp, sizeof(tmp),
                     "%lu\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%d",
                     (unsigned long)t_ms,
                     (long)p_mPa, (long)k_mK,
                     (long)ax_mg, (long)ay_mg, (long)az_mg,
                     (long)gx_uds, (long)gy_uds, (long)gz_uds,
                     (long)alt_mm, (long)vel_mms,
                     state);
    }
    if (n < 0) n = 0;
    if ((uint32_t)n > TSV_ROW_SIZE - 2) n = TSV_ROW_SIZE - 2;
    memcpy(out, tmp, n);
}

static void readFlightTsv(uint32_t flightSeed, uint32_t totalRows, uint32_t offset,
                          uint8_t* buf, uint32_t len) {
    const uint32_t totalBytes = (totalRows + 1u) * TSV_ROW_SIZE; // header + data rows
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
        buildTsvRow(flightSeed, totalRows, rowIdx, row);
        uint32_t toCopy = TSV_ROW_SIZE - rowOff;
        if (toCopy > (len - pos)) toCopy = (len - pos);
        memcpy(buf + pos, row + rowOff, toCopy);
        pos += toCopy;
    }
}

// ============================ Config File ============================
//
// CONFIG.TXT lives in a single 4 KB cluster (cluster 2). On boot we render
// a default config into the buffer; on host writes we capture the new bytes
// into the same buffer and parse them back into g_config so a real firmware
// could pick up the change.

static constexpr uint16_t CONFIG_CAPACITY = (uint16_t)CLUSTER_BYTES;
static char     g_configBuf[CONFIG_CAPACITY];
static volatile bool g_configDirty = false;

struct ConfigValues {
    char     boardName[24];
    uint32_t drogueDelayMs;
    float    mainElevationM;
    float    voltageScale;
    uint32_t pyroFireDurationMs;
    bool     enableBuzzer;
};

static ConfigValues g_config = {
    "SillyGoose",
    1000,
    150.0f,
    0.00644f,
    1000,
    true,
};

static void renderConfig() {
    int n = snprintf(g_configBuf, CONFIG_CAPACITY,
                     "# SillyGoose USB config -- edit & save\r\n"
                     "# Lines starting with '#' are comments.\r\n"
                     "boardName=%s\r\n"
                     "drogueDelayMs=%lu\r\n"
                     "mainElevationM=%.2f\r\n"
                     "voltageScale=%.6f\r\n"
                     "pyroFireDurationMs=%lu\r\n"
                     "enableBuzzer=%s\r\n",
                     g_config.boardName,
                     (unsigned long)g_config.drogueDelayMs,
                     g_config.mainElevationM,
                     g_config.voltageScale,
                     (unsigned long)g_config.pyroFireDurationMs,
                     g_config.enableBuzzer ? "true" : "false");
    if (n < 0) n = 0;
    if ((uint32_t)n >= CONFIG_CAPACITY) n = CONFIG_CAPACITY - 1;
    // Pad remainder with newlines so the file is "clean" text from the host's view
    while (n < (int)CONFIG_CAPACITY - 2) {
        g_configBuf[n++] = '\r';
        g_configBuf[n++] = '\n';
    }
    if (n < (int)CONFIG_CAPACITY) g_configBuf[n] = '\0';
}

static void trim(char* s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' || s[len - 1] == '\r' || s[len - 1] == '\n')) {
        s[--len] = '\0';
    }
}

static void parseConfig(const char* text, uint32_t len) {
    uint32_t i = 0;
    while (i < len) {
        const uint32_t lineStart = i;
        while (i < len && text[i] != '\n' && text[i] != '\r') i++;
        const uint32_t lineEnd = i;
        while (i < len && (text[i] == '\r' || text[i] == '\n')) i++;
        if (lineEnd == lineStart) continue;
        if (text[lineStart] == '#') continue;

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
        trim(key);
        trim(val);

        if      (strcmp(key, "boardName")          == 0) { strncpy(g_config.boardName, val, sizeof(g_config.boardName) - 1); g_config.boardName[sizeof(g_config.boardName) - 1] = '\0'; }
        else if (strcmp(key, "drogueDelayMs")      == 0) { g_config.drogueDelayMs      = strtoul(val, nullptr, 10); }
        else if (strcmp(key, "mainElevationM")     == 0) { g_config.mainElevationM     = (float)atof(val); }
        else if (strcmp(key, "voltageScale")       == 0) { g_config.voltageScale       = (float)atof(val); }
        else if (strcmp(key, "pyroFireDurationMs") == 0) { g_config.pyroFireDurationMs = strtoul(val, nullptr, 10); }
        else if (strcmp(key, "enableBuzzer")       == 0) { g_config.enableBuzzer       = (strcmp(val, "true") == 0 || strcmp(val, "1") == 0); }
    }
}

// ============================ Virtual File Table ============================
//
// Each file is a contiguous run of clusters. The CONFIG file is first so it
// always lands on cluster 2; flights follow. Sizes for the flights are picked
// to exercise small/medium/large reads.

struct VFile {
    char     name83[11]; // 8.3 packed, space-padded, no separator
    uint32_t startCluster;
    uint32_t fileSize;
    uint32_t numClusters;
    uint8_t  attr;       // FAT directory attribute byte
};

static constexpr uint32_t NUM_FLIGHTS = 3;
// Approx sizes: 80 KB, 4.8 MB, 48 MB. The last is the offload stress test
// (~10x FLIGHT02) -- bigger and the host's USB-MSC copy starts truncating
// with zero-padding before the device finishes serving the file.
static constexpr uint32_t TSV_ROWS[NUM_FLIGHTS] = {500, 30000, 300000};

static constexpr uint8_t FILE_COUNT = NUM_FLIGHTS + 1;
static VFile g_files[FILE_COUNT];

static void buildFileTable() {
    uint32_t cluster = 2;

    // CONFIG.TXT -- needs to be writable so host can edit & save.
    static const char cfgName[11] = {'C','O','N','F','I','G',' ',' ','T','X','T'};
    memcpy(g_files[0].name83, cfgName, 11);
    g_files[0].startCluster = cluster;
    g_files[0].fileSize     = CONFIG_CAPACITY;
    g_files[0].numClusters  = 1;
    g_files[0].attr         = 0x20;                       // ARCHIVE
    cluster += g_files[0].numClusters;

    // Flight files -- marked READ_ONLY so the host doesn't try to mutate dir
    // entries (e.g. last-accessed time) while we're streaming a copy.
    for (uint32_t i = 0; i < NUM_FLIGHTS; ++i) {
        VFile& f = g_files[i + 1];
        memset(f.name83, ' ', 11);
        const char* base = "FLIGHT";
        memcpy(f.name83, base, 6);
        f.name83[6] = '0' + (char)((i + 1) / 10);
        f.name83[7] = '0' + (char)((i + 1) % 10);
        f.name83[8] = 'T'; f.name83[9] = 'S'; f.name83[10] = 'V';
        f.fileSize    = (TSV_ROWS[i] + 1u) * TSV_ROW_SIZE;
        f.numClusters = (f.fileSize + CLUSTER_BYTES - 1) / CLUSTER_BYTES;
        f.startCluster = cluster;
        f.attr         = 0x21;                            // READ_ONLY | ARCHIVE
        cluster += f.numClusters;
    }
}

static VFile* findFileForCluster(uint32_t cluster) {
    for (uint8_t i = 0; i < FILE_COUNT; ++i) {
        if (cluster >= g_files[i].startCluster &&
            cluster <  g_files[i].startCluster + g_files[i].numClusters) {
            return &g_files[i];
        }
    }
    return nullptr;
}

// ============================ Sector Generators ============================

static void readBootSector(uint8_t* buf) {
    memset(buf, 0, SECTOR_SIZE);
    // Jump + OEM name
    buf[0] = 0xEB; buf[1] = 0x3C; buf[2] = 0x90;
    memcpy(buf + 3, "MSDOS5.0", 8);
    // BIOS Parameter Block
    buf[11] = SECTOR_SIZE & 0xFF;          buf[12] = (SECTOR_SIZE >> 8) & 0xFF;
    buf[13] = SECTORS_PER_CLUSTER;
    buf[14] = RESERVED_SECTORS & 0xFF;     buf[15] = (RESERVED_SECTORS >> 8) & 0xFF;
    buf[16] = NUM_FATS;
    buf[17] = ROOT_DIR_ENTRIES & 0xFF;     buf[18] = (ROOT_DIR_ENTRIES >> 8) & 0xFF;
    if (TOTAL_SECTORS <= 0xFFFFu) {
        buf[19] = TOTAL_SECTORS & 0xFF;    buf[20] = (TOTAL_SECTORS >> 8) & 0xFF;
    }
    buf[21] = 0xF8; // fixed media
    buf[22] = SECTORS_PER_FAT & 0xFF;      buf[23] = (SECTORS_PER_FAT >> 8) & 0xFF;
    buf[24] = 0x3F; buf[25] = 0x00;        // sectors per track
    buf[26] = 0xFF; buf[27] = 0x00;        // num heads
    buf[28] = 0x00; buf[29] = 0x00; buf[30] = 0x00; buf[31] = 0x00; // hidden sectors
    if (TOTAL_SECTORS > 0xFFFFu) {
        buf[32] = TOTAL_SECTORS         & 0xFF;
        buf[33] = (TOTAL_SECTORS >>  8) & 0xFF;
        buf[34] = (TOTAL_SECTORS >> 16) & 0xFF;
        buf[35] = (TOTAL_SECTORS >> 24) & 0xFF;
    }
    // FAT16 extended BPB
    buf[36] = 0x80;        // drive number
    buf[37] = 0x00;
    buf[38] = 0x29;        // extended boot signature
    buf[39] = 0x12; buf[40] = 0x34; buf[41] = 0x56; buf[42] = 0x78; // volume serial
    memcpy(buf + 43, VOLUME_LABEL, 11);
    memcpy(buf + 54, "FAT16   ", 8);
    // Boot signature
    buf[510] = 0x55; buf[511] = 0xAA;
}

static void readFatSector(uint32_t fatSectorIdx, uint8_t* buf) {
    memset(buf, 0, SECTOR_SIZE);
    const uint32_t entriesPerSector = SECTOR_SIZE / 2;
    const uint32_t startEntry       = fatSectorIdx * entriesPerSector;
    for (uint32_t i = 0; i < entriesPerSector; ++i) {
        const uint32_t entry = startEntry + i;
        uint16_t v = 0x0000;
        if (entry == 0)      v = 0xFFF8;            // media + reserved bits
        else if (entry == 1) v = 0xFFFF;            // reserved
        else {
            VFile* f = findFileForCluster(entry);
            if (f != nullptr) {
                const uint32_t last = f->startCluster + f->numClusters - 1;
                v = (entry == last) ? 0xFFFF : (uint16_t)(entry + 1);
            }
        }
        buf[i * 2]     =  v       & 0xFF;
        buf[i * 2 + 1] = (v >> 8) & 0xFF;
    }
}

static void writeDirEntry(uint8_t* slot, const char name83[11], uint8_t attr,
                          uint32_t cluster, uint32_t size) {
    memcpy(slot, name83, 11);
    slot[11] = attr;
    // 12..19 reserved / timestamps (zero)
    slot[20] = (cluster >> 16) & 0xFF;
    slot[21] = (cluster >> 24) & 0xFF;
    // 22..25 last write time/date (zero)
    slot[26] =  cluster        & 0xFF;
    slot[27] = (cluster >>  8) & 0xFF;
    slot[28] =  size           & 0xFF;
    slot[29] = (size >>  8)    & 0xFF;
    slot[30] = (size >> 16)    & 0xFF;
    slot[31] = (size >> 24)    & 0xFF;
}

static void readRootDirSector(uint32_t dirSectorIdx, uint8_t* buf) {
    memset(buf, 0, SECTOR_SIZE);
    const uint32_t entriesPerSector = SECTOR_SIZE / 32;
    const uint32_t startEntry       = dirSectorIdx * entriesPerSector;
    for (uint32_t e = 0; e < entriesPerSector; ++e) {
        const uint32_t entryIdx = startEntry + e;
        uint8_t* slot = buf + e * 32;
        if (entryIdx == 0) {
            memcpy(slot, VOLUME_LABEL, 11);
            slot[11] = 0x08; // VOLUME_ID
        } else if (entryIdx - 1 < FILE_COUNT) {
            const VFile& f = g_files[entryIdx - 1];
            writeDirEntry(slot, f.name83, f.attr, f.startCluster, f.fileSize);
        } else {
            return; // 0x00 entries terminate the directory
        }
    }
}

static void readDataSector(uint32_t lba, uint8_t* buf) {
    memset(buf, 0, SECTOR_SIZE);
    const uint32_t dataLba         = lba - DATA_START;
    const uint32_t cluster         = 2u + (dataLba / SECTORS_PER_CLUSTER);
    const uint32_t sectorInCluster = dataLba % SECTORS_PER_CLUSTER;
    VFile* f = findFileForCluster(cluster);
    if (f == nullptr) return;

    const uint32_t offsetInFile =
        (cluster - f->startCluster) * CLUSTER_BYTES + sectorInCluster * SECTOR_SIZE;

    if (f == &g_files[0]) {
        if (offsetInFile < CONFIG_CAPACITY) {
            uint32_t toCopy = SECTOR_SIZE;
            if (offsetInFile + toCopy > CONFIG_CAPACITY) toCopy = CONFIG_CAPACITY - offsetInFile;
            memcpy(buf, g_configBuf + offsetInFile, toCopy);
        }
    } else {
        const uint32_t flightIdx = (uint32_t)(f - g_files) - 1u;
        readFlightTsv((flightIdx + 1u) * 7919u, TSV_ROWS[flightIdx],
                      offsetInFile, buf, SECTOR_SIZE);
    }
}

// ============================ TinyUSB Callbacks ============================

static Adafruit_USBD_MSC g_usbMsc;

static int32_t msc_read_cb(uint32_t lba, void* buffer, uint32_t bufsize) {
    uint8_t* out = (uint8_t*)buffer;
    const uint32_t sectors = bufsize / SECTOR_SIZE;
    for (uint32_t i = 0; i < sectors; ++i) {
        const uint32_t cur = lba + i;
        uint8_t* dst = out + i * SECTOR_SIZE;
        if (cur == 0) {
            readBootSector(dst);
        } else if (cur >= FAT_START && cur < FAT_START + (uint32_t)NUM_FATS * SECTORS_PER_FAT) {
            // Both FAT copies serve the same synthesized FAT.
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

static int32_t msc_write_cb(uint32_t lba, uint8_t* buffer, uint32_t bufsize) {
    const uint32_t sectors = bufsize / SECTOR_SIZE;
    for (uint32_t i = 0; i < sectors; ++i) {
        const uint32_t cur = lba + i;
        // We only persist writes that fall inside CONFIG.TXT's cluster.
        // Everything else (FAT scratching, dir entry timestamp updates,
        // OS metadata, Trash bins, etc.) is silently accepted.
        if (cur >= DATA_START && cur < TOTAL_SECTORS) {
            const uint32_t dataLba         = cur - DATA_START;
            const uint32_t cluster         = 2u + (dataLba / SECTORS_PER_CLUSTER);
            const uint32_t sectorInCluster = dataLba % SECTORS_PER_CLUSTER;
            VFile* f = findFileForCluster(cluster);
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

static void msc_flush_cb(void) {
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

// ============================ Arduino entry points ============================

void setup() {
    renderConfig();
    buildFileTable();

    USBDevice.setProductDescriptor("SillyGoose MSC Test");
    USBDevice.setManufacturerDescriptor("NULI");

    g_usbMsc.setID("NULI", "SillyGoose", "1.0");
    g_usbMsc.setCapacity(TOTAL_SECTORS, SECTOR_SIZE);
    g_usbMsc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);
    g_usbMsc.setUnitReady(true);
    g_usbMsc.begin();

    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    static uint32_t last = 0;
    const uint32_t now = millis();
    if (now - last > 1000) {
        last = now;
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
}