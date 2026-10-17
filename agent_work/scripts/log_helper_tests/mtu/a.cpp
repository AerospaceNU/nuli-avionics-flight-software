#include "shared.h"
#include <string>
struct CapA : DebugStream { std::string s; size_t write(const void* b, size_t n) override { s.append((const char*)b, n); return n; } };
std::string fromA() { CapA c; Shared d{7, 1.5f, -9000000000LL}; printShared(d, &c); return std::string(SHARED_HEADER) + "|" + c.s; }
const char* headerPtrA() { return SHARED_HEADER; }
