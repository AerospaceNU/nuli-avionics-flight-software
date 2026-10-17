#include "shared.h"
#include <cstdio>
#include <string>
std::string fromA(); const char* headerPtrA();
struct CapB : DebugStream { std::string s; size_t write(const void* b, size_t n) override { s.append((const char*)b, n); return n; } };
int main() {
  CapB c; Shared d{7, 1.5f, -9000000000LL}; printShared(d, &c);
  std::string b = std::string(SHARED_HEADER) + "|" + c.s;
  std::string a = fromA();
  printf("A=[%s]\nB=[%s]\nsame=%d\n", a.c_str(), b.c_str(), a == b);
  // BasicLogger::setup takes a const char* header and a void(*)(const T&, DebugStream*)
  const char* h = SHARED_HEADER; void (*fp)(const Shared&, DebugStream*) = printShared; (void)fp; (void)h;
  return a == b && a == "t\tx\tbig|7\t1.5\t-9000000000\n" ? 0 : 1;
}
