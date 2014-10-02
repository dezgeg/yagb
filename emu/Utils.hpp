#pragma once

#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <string>

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

template<typename A, typename B, typename C>
A clamp(A val, B min, C max) {
    return val < min ? min : val > max ? max : val;
}

inline std::string replaceExtension(std::string name, const char* extension) {
    std::string tmp = name;
    size_t dotIndex = tmp.find_last_of('.');
    if (dotIndex != std::string::npos) {
        tmp = tmp.substr(0, dotIndex);
    }
    return tmp + "." + extension;
}
