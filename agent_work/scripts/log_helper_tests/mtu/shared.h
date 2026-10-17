#pragma once
#define remove_struct_padding __attribute__((packed))
#include "core/BasicLoggerHelper.h"
// clang-format off
LOG_DATA_AUTO_DEFINE(SHARED_DEF, struct Shared { uint32_t t; float x; int64_t big; } remove_struct_padding;)
// clang-format on
static constexpr auto& SHARED_HEADER = BasicLoggerHelper::generateBasicLogHeader<SHARED_DEF>();
inline void printShared(const Shared& d, DebugStream* s) { BasicLoggerHelper::genericBasicLogDataPrinter<SHARED_DEF>(d, s); }
