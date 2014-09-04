#pragma once

#include <cassert>
#include <cstdlib>
#include <cstdint>

#define unreachable() assert(!"unreachable()")

template<typename T, unsigned long count>
std::size_t arraySize(const T (& array)[count]) {
    (void)array;
    return count;
}

inline unsigned long bit(unsigned long b) {
    return 1 << b;
}

inline uint8_t reverseBits(uint8_t b) {
    return ((b * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32;
}
