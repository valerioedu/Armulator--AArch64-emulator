#include <common.hpp>

u32 extractBits(u32 inst, u8 high, u8 low) {
    u8 count = high - low + 1;
    if (count >= 32) return inst >> low;
    return (inst >> low) & ((1u << count) - 1);
}

i64 signExtend(u32 val, u32 bits) {
	u32 shift = 64 - bits;
	return (i64(val) << shift) >> shift;
}