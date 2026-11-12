#ifndef COMMON_HPP
#define COMMON_HPP

#include <cstdint>

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using u128 = __uint128_t;
using i64 = int64_t;

u32 extractBits(u32 inst, u8 high, u8 low);
i64 signExtend(u32 val, u32 bits);

#endif