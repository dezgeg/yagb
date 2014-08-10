#pragma once
#include "Platform.hpp"

struct BusUtil
{
    static void arrayMemAccess(Byte* array, Word address, Byte* pData, bool isWrite)
    {
        if (isWrite)
            array[address] = *pData;
        else
            *pData = array[address];
    }

    static void simpleRegAccess(Byte* pReg, Byte* pData, bool isWrite)
    {
        if (isWrite)
            *pReg = *pData;
        else
            *pData = *pReg;
    }
};
