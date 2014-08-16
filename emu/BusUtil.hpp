#pragma once
#include "Platform.hpp"

struct BusUtil
{
    static inline void arrayMemAccess(Byte* array, Word address, Byte* pData, bool isWrite)
    {
        if (isWrite)
            array[address] = *pData;
        else
            *pData = array[address];
    }

    static inline void simpleRegAccess(Byte* pReg, Byte* pData, bool isWrite, Byte writeMask = 0xff)
    {
        if (isWrite)
            *pReg = *pData & writeMask;
        else
            *pData = *pReg;
    }
};
