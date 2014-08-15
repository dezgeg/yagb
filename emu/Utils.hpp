#pragma once
#include <cassert>
#include <cstdlib>

#define unreachable() assert(!"unreachable()")

template<typename T, unsigned long count>
std::size_t arraySize(const T (&array)[count])
{
    (void) array;
    return count;
}
