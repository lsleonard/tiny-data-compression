//
//  td64.c
//  tiny data compression of 2 to 64 bytes based on td64
//  version 1.1
//
//  Created by Stevan Leonard on 10/29/21.
//  Copyright © 2021 L. Stevan Leonard. All rights reserved.
#include "td64.h"

// fixed bit compression (fbc): for the number of uniques in input, the minimum number of input values for 25% compression
// uniques   1  2  3  4  5   6   7   8   9   10  11  12  13  14  15  16
// nvalues   2, 4, 7, 9, 15, 17, 19, 23, 40, 44, 48, 52, 56, 60, 62, 64};
static const uint32_t uniqueLimits25[MAX_TD64_BYTES+1]=
//      2    4      7    9            15   17   19       23
{ 0, 0, 1,1, 2,2,2, 3,3, 4,4,4,4,4,4, 5,5, 6,6, 7,7,7,7, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
//  40       44           48           52           56           60     62     64
    9,9,9,9, 10,10,10,10, 11,11,11,11, 12,12,12,12, 13,13,13,13, 14,14, 15,15, 16
};

#define MAX_PREDEFINED_CHAR_COUNT 16 // text mode: 16 most frequent characters from Morse code
static const uint32_t textChars[MAX_PREDEFINED_CHAR_COUNT]={
    ' ', 'e', 't', 'a', 'i', 'n', 'o', 's', 'h', 'r', 'd', 'l', 'u', 'c', 'm', 'g'
};

// text mode: a one indicates a character from the most frequent characters based on Morse code set
static const uint32_t predefinedTextChars[256]={
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1,
    0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// text mode: index to predefined char or 16 if another value
static const uint32_t textEncoding[256]={
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    0, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 3, 16, 13, 10, 1, 16, 15, 8, 4, 16, 16, 11, 14, 5, 6,
    16, 16, 9, 7, 2, 12, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
};

const uint32_t bitMask[]={0,1,3,7,15,31,63,127,255,511};

// -----------------------------------------------------------------------------------
int32_t td5(const unsigned char *inVals, unsigned char *outVals, const uint32_t nValues)
// -----------------------------------------------------------------------------------
// Compress 2 to 5 values with 1 or 2 unique values
// Management of whether compressible and number of input values must be maintained
//    by caller. Decdode requires number of input values and only accepts compressed data.
// Decode first byte:
// 01 = single unique, followed by low 6 bits of input then high two bits in second byte
// 11 = text mode characters, followed by a 4-bit character reference for each input value; all values must be one of the 16 textChars
// 0 = encoding by number of values:
//    2 bytes: two unique nibbles
//    3 bytes: two unique nibbles
//    4 or 5 bytes: two uniques
// Arguments:
//   inVals   input data
//   outVals  compressed data
//   nValues  number of values to compress
// returns number of bits compressed, 0 if not compressible, or -1 if error
{
    switch (nValues)
    {
        case 2:
        {
            const uint32_t ival1=(unsigned char)inVals[0];
            // encode for 1 unique or 2 nibbles
            if (ival1 == (unsigned char)inVals[1])
            {
                outVals[0] = (unsigned char)(ival1 << 2) | 1;
                outVals[1] = (unsigned char)ival1 >> 6;
                return 10;
            }
            // check for two text chars
            if (predefinedTextChars[ival1] && predefinedTextChars[inVals[1]])
            {
                // encode two text chars
                outVals[0] = (unsigned char)(3 | (int32_t)textEncoding[ival1] << 2) | ((int32_t)textEncoding[inVals[1]] << 6);
                outVals[1] = (unsigned char)textEncoding[inVals[1]] >> 2;
                return 10;
            }
            // check for two unique nibbles
            uint32_t outBits=0; // keep track of which nibbles match nibble1 (0) or otherVal (1)
            uint32_t otherVal=256;
            const uint32_t nibble1=ival1 >> 4;
            const uint32_t nibble2=ival1 & 0xf;
            const uint32_t nibble3=(unsigned char)inVals[1] >> 4;
            if (nibble2 != nibble1)
            {
                otherVal = nibble2;
                outBits = 2;
            }
            if (nibble3 != nibble1)
            {
                if ((nibble3 != otherVal) && (otherVal != 256))
                    return 0;
                if (otherVal == 256)
                    otherVal = nibble3;
                outBits |= 4; // set this bit to other value
            }
            const uint32_t nibble4=(unsigned char)inVals[1] & 0xf;
            if (nibble4 != nibble1)
            {
                if ((nibble4 != otherVal) && (otherVal != 256))
                    return 0;
                if (otherVal == 256)
                    otherVal = nibble4;
                outBits |= 8;  // set this bit to other value
            }
            outVals[0] = (unsigned char)((nibble1 << 4) | outBits);
            outVals[1] = (unsigned char)otherVal;
            return 12; // save 4 bits
        }
        case 3:
        {
            // encode for 1 unique
            const uint32_t ival1=(unsigned char)inVals[0];
            if ((ival1 == (unsigned char)inVals[1]) && (ival1 == (unsigned char)inVals[2]))
            {
                outVals[0] = (unsigned char)(ival1 << 2) | 1;
                outVals[1] = (unsigned char)ival1 >> 6;
                return 10;
            }
            // check for three text chars
            if (predefinedTextChars[ival1] && predefinedTextChars[inVals[1]] && predefinedTextChars[inVals[2]])
            {
                // encode three text chars
                outVals[0] = (unsigned char)(3 | textEncoding[ival1] << 2) | (textEncoding[inVals[1]] << 6);
                outVals[1] = (unsigned char)(textEncoding[inVals[1]] >> 2) | (textEncoding[inVals[2]] << 2);
                return 14;
            }
            // check for two unique nibbles
            uint32_t outBits=0; // keep track of which nibbles match nibble1 or otherVal
            uint32_t otherVal=256;
            const uint32_t nibble1=(unsigned char)ival1 >> 4;
            const uint32_t nibble2=(unsigned char)ival1 & 0xf;
            const uint32_t nibble3=(unsigned char)inVals[1] >> 4;
            if (nibble2 != nibble1)
            {
                otherVal = nibble2;
                outBits = 2;
            }
            if (nibble3 != nibble1)
            {
                if ((nibble3 != otherVal) && (otherVal != 256))
                    return 0;
                if (otherVal == 256)
                    otherVal = nibble3;
                outBits |= 4; // set this bit to other value
            }
            const uint32_t nibble4=(unsigned char)inVals[1] & 0xf;
            if (nibble4 != nibble1)
            {
                if ((nibble4 != otherVal) && (otherVal != 256))
                    return 0;
                if (otherVal == 256)
                    otherVal = nibble4;
                outBits |= 8;  // set this bit to other value
            }
            const uint32_t nibble5=(unsigned char)inVals[2] >> 4;
            if (nibble5 != nibble1)
            {
                if ((nibble5 != otherVal) && (otherVal != 256))
                    return 0;
                if (otherVal == 256)
                    otherVal = nibble5;
                outBits |= 16;  // set this bit to other value
            }
            const uint32_t nibble6=(unsigned char)inVals[2] & 0xf;
            if (nibble6 != nibble1)
            {
                if ((nibble6 != otherVal) && (otherVal != 256))
                    return 0;
                if (otherVal == 256)
                    otherVal = nibble6;
                outBits |= 32;  // set this bit to other value
            }
            outVals[0] = (unsigned char)(nibble1 << 6) | (unsigned char)outBits;
            outVals[1] = (unsigned char)(otherVal << 2) | (unsigned char)(nibble1 >> 2);
            return 14; // save 10 bits
        }
        case 4:
        case 5:
        {
            const int32_t ival1=(unsigned char)inVals[0];
            const int32_t ival2=(unsigned char)inVals[1];
            const int32_t ival3=(unsigned char)inVals[2];
            const int32_t ival4=(unsigned char)inVals[3];
            if (ival1 == ival2 == ival3 == ival4)
            {
                if (nValues == 4)
                {
                    // all 4 values equal
                    outVals[0] = (unsigned char)(ival1 << 2) | 1;
                    outVals[1] = (unsigned char)ival1 >> 6;
                    return 10;
                }
                else if ((nValues == 5) && (ival1 == inVals[4]))
                {
                    // all 5 values equal
                    outVals[0] = (unsigned char)(ival1 << 2) | 1;
                    outVals[1] = (unsigned char)ival1 >> 6;
                    return 10;
                }
            }
            // check for four and five text chars,
            uint32_t nTextChars=predefinedTextChars[ival1]+predefinedTextChars[ival2]+predefinedTextChars[ival3]+predefinedTextChars[ival4];
            if (nTextChars == 4)
            {
                // encode four text chars
                outVals[0] = (unsigned char)(3 | textEncoding[ival1] << 2) | (textEncoding[ival2] << 6);
                outVals[1] = (unsigned char)(textEncoding[ival2] >> 2) | (textEncoding[ival3] << 2) | (textEncoding[ival4] << 6);
                outVals[2] = (unsigned char)textEncoding[ival4] >> 2;
                if (nValues == 4)
                    return 18;
                // encode fifth text char
                if (predefinedTextChars[inVals[4]])
                {
                    // fifth value is a text char, signal with a 1 in third bit
                    outVals[2] |= 4 | (unsigned char)textEncoding[inVals[4]] << 3;
                    return 23;
                }
                // fifth value is not a text char, output a 0 folllwed by itself
                outVals[2] |= inVals[4] << 3;
                outVals[3] = inVals[4] >> 5;
                return 27;
            }
            // encode for 2 uniques
            int32_t outBits=0; // first bit is 1 for one unique
            // encode 0 for first val, 1 for other val, starting at second bit
            int32_t otherVal=-1;
            if (ival2 != ival1)
            {
                otherVal=ival2;
                outBits |= 2;
            }
            if (ival3 != ival1)
            {
                if ((ival3 != otherVal) && (otherVal != -1))
                    return 0;
                if (otherVal ==  -1)
                    otherVal = ival3;
                outBits |= 4;
            }
            if (ival4 != ival1)
            {
                if ((ival4 != otherVal) && (otherVal != -1))
                    return 0;
                if (otherVal == -1)
                    otherVal = ival4;
                outBits |= 8;
            }
            if (nValues == 4)
            {
                // or in low 4 bits second unique in top bits
                outVals[0] = (unsigned char)(outBits | (ival1 << 4));
                outVals[1] = (unsigned char)((ival1 >> 4) | (otherVal << 4));
                outVals[2] = (unsigned char)(otherVal >> 4);
                return 20;
            }
            // continue to check fifth value
            const int32_t ival5=(unsigned char)inVals[4];
            if (ival5 != ival1)
            {
                if ((ival5 != otherVal) && (otherVal != -1))
                    return 0;
                if (otherVal == -1)
                    otherVal = ival5;
                outBits |= 0x10;
            }
            // clear low bit byte 0 (not single unique) and or in low 3 bits ival1
            outVals[0] = (unsigned char)(outBits | (ival1 << 5));
            outVals[1] = (unsigned char)(((ival1 >> 3)) | (otherVal << 5));
            outVals[2] = (unsigned char)(otherVal >> 3);
            return 21;
        }
        default:
            return -9; // number of values specified not supported (2 to 5 handled here)
    }
} // end td5

// -----------------------------------------------------------------------------------
int32_t td5d(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed)
// -----------------------------------------------------------------------------------
// Decode 2 to 5 values encoded by td5.
// Decode first byte:
// 01 = single unique, followed by low 6 bits of input; otherwise, high two bits in second byte
// 11 = text encoding: replace 4-bit text index by corresponding char
//      2 to 4 values requires all values text, 5 chars can have final char non-text, indicated by 0 or 1 bit following 4 text indexes
// 0 = encoding by number of values:
//    2 bytes: two nibbles the same
//    3 bytes: single unique
//    4 or 5 bytes: two uniques
// Arguments:
//   inVals   compressed data with fewer bits than in original values
//   outVals  decompressed data
//   nOriginalValues  number of values in the original input to td5: required input
//   bytesProcessed  number of input bytes processed, which can be used by caller to position past these bytes
// returns number of bytes output or -1 if error
{
    const uint32_t firstByte=(unsigned char)inVals[0];
    if ((firstByte & 3) == 1)
    {
        // process single unique
        uint32_t unique = (firstByte >> 2) | (inVals[1] >> 6);
        memset(outVals, (unsigned char)unique, nOriginalValues);
        *bytesProcessed = (firstByte & 2) ? 1 : 2;
        return (int)nOriginalValues;
    }
    uint32_t val1;
    uint32_t val2;
    const uint32_t secondByte = (unsigned char)inVals[1];
    switch (nOriginalValues)
    {
        // special encoding for 2 to 5 bytes
        case 2:
        {
            *bytesProcessed = 2;
            if ((firstByte & 3) == 3)
            {
                // text mode encoding: next two four-bit values are text offsets
                outVals[0] = (unsigned char)textChars[(firstByte>>2) & 0xf];
                outVals[1] = (unsigned char)textChars[(firstByte>>6) | ((secondByte<<2) & 0xf)];
                return 2;
            }
            // decode nibble compression for 2 bytes
            // nibble1 nibble 2   nibble3 nibble4
            // high    low        high    low
            // first byte         second byte
            const uint32_t cbits = (unsigned char)(firstByte >> 1) & 0x7; // 3 control bits
            const uint32_t nibble1 = (unsigned char)firstByte >> 4;
            const uint32_t nibble2 = (unsigned char)secondByte & 0xf;
            outVals[0] = (unsigned char)(nibble1 << 4) | (unsigned char)((cbits & 1) ? nibble2 : nibble1);
            outVals[1] = (unsigned char)((((cbits & 2) ? nibble2 : nibble1) << 4) |
            (unsigned char)((cbits & 4) ? nibble2 : nibble1));
            return 2;
        }
        case 3:
        {
            *bytesProcessed = 2;
            if ((firstByte & 3) == 3)
            {
                // text mode encoding: next three four-bit values are text offsets
                outVals[0] = (unsigned char)textChars[(firstByte>>2) & 0xf];
                outVals[1] = (unsigned char)textChars[(firstByte>>6) | ((secondByte<<2) & 0xf)];
                outVals[2] = (unsigned char)textChars[(secondByte>>2) & 0xf];
                return 3;
            }
            // decode nibble compression for 3 bytes
            // nibble1 nibble 2   nibble3 nibble4   nibble5 nibble6
            // high    low        high    low       high    low
            // first byte         second byte       third byte
            const uint32_t cbits = ((unsigned char)firstByte >> 1) & 0x1f; // 5 control bits
            const uint32_t nibble1 = ((unsigned char)firstByte >> 6 | ((unsigned char)secondByte << 2)) & 0xf;
            const uint32_t nibble2 = ((unsigned char)secondByte >> 2) & 0xf;
            outVals[0] = (unsigned char)(nibble1 << 4) | (unsigned char)((cbits & 1) ? nibble2 : nibble1);
            outVals[1] = (unsigned char)((((cbits & 2) ? nibble2 : nibble1) << 4) |
            (unsigned char)((cbits & 4) ? nibble2 : nibble1));
            outVals[2] = (unsigned char)((((cbits & 8) ? nibble2 : nibble1) << 4) |
            (unsigned char)((cbits & 16) ? nibble2 : nibble1));
            return 3;
        }
        case 4:
        {
            *bytesProcessed = 3;
            if ((firstByte & 3) == 3)
            {
                // text mode encoding: next four four-bit values are text offsets
                outVals[0] = (unsigned char)textChars[(firstByte>>2) & 0xf];
                outVals[1] = (unsigned char)textChars[(firstByte>>6) | ((secondByte<<2) & 0xf)];
                outVals[2] = (unsigned char)textChars[(secondByte>>2) & 0xf];
                outVals[3] = (unsigned char)textChars[(secondByte>>6) | ((inVals[2]<<2) & 0xf)];
                return 3;
            }
            const int32_t thirdByte = (unsigned char)inVals[2];
            val1 = (unsigned char)(firstByte >> 4) | (unsigned char)(secondByte << 4);
            val2 = (unsigned char)(secondByte >> 4) | (unsigned char)(thirdByte << 4);
            outVals[0] = (unsigned char)val1;
            outVals[1] = (firstByte & 2) ? (unsigned char)val2 : (unsigned char)val1;
            outVals[2] = (firstByte & 4) ? (unsigned char)val2 : (unsigned char)val1;
            outVals[3] = (firstByte & 8) ? (unsigned char)val2 : (unsigned char)val1;
            return 4;
        }
        case 5:
        {
            *bytesProcessed = 3;
            if ((firstByte & 3) == 3)
            {
                // text mode encoding: next four four-bit values are text offsets
                outVals[0] = (unsigned char)textChars[(firstByte>>2) & 0xf];
                outVals[1] = (unsigned char)textChars[(firstByte>>6) | ((secondByte<<2) & 0xf)];
                outVals[2] = (unsigned char)textChars[(secondByte>>2) & 0xf];
                uint32_t thirdByte = inVals[2];
                outVals[3] = (unsigned char)textChars[(secondByte>>6) | ((thirdByte<<2) & 0xf)];
                if (thirdByte & 0x4)
                {
                    // fifth byte is text char
                    outVals[4] = (unsigned char)(thirdByte >> 3) & 0xf;
                }
                else
                {
                    // fifth byte is 8-bit value
                    outVals[4] = (unsigned char)(thirdByte >> 3) | ((inVals[3]<<5) & 0xff);
                    *bytesProcessed = 4;
                }
                return 5;
            }
            const int32_t thirdByte = (unsigned char)inVals[2];
            val1 = (unsigned char)(firstByte >> 5) | (unsigned char)(secondByte << 3);
            val2 = (unsigned char)(secondByte >> 5) | (unsigned char)(thirdByte << 3);
            outVals[0] = (unsigned char)val1;
            outVals[1] = (firstByte & 2) ? (unsigned char)val2 : (unsigned char)val1;
            outVals[2] = (firstByte & 4) ? (unsigned char)val2 : (unsigned char)val1;
            outVals[3] = (firstByte & 8) ? (unsigned char)val2 : (unsigned char)val1;
            outVals[4] = (firstByte & 0x10) ? (unsigned char)val2 : (unsigned char)val1;
            return 5;
        }
        default:
            return -3; // unexpected program error
    }
} // end td5d

// -----------------------------------------------------------------------------------
int32_t encodeTextMode(unsigned char *inVals, unsigned char *outVals, const uint32_t nValues, const uint32_t maxBits)
// -----------------------------------------------------------------------------------
{
    // if value is predefined, use its index; otherwise, output 8-bit value
    // generate control bit 1 if predefined text char, 0 if 8-bit value
    unsigned char *pInVal=inVals;
    unsigned char *pLastInValPlusOne=inVals+nValues;
    uint32_t inVal;
    uint32_t nextOutVal=(nValues-1)/8+2; // allocate space for control bits in bytes following first
    uint64_t controlByte=0;
    uint64_t controlBit=1;
    uint32_t predefinedTCs=0x07; // two predefined text chars (PTCs), on init, written to outVals[0]: indicates text mode
    uint32_t predefinedTCnt=2; // indicate whether first PTC is encoded for output
    uint32_t predefinedTCsOut=0; // write a 0 to outVals[0] when first PTC encountered
    
    while (pInVal < pLastInValPlusOne)
    {
        if (textEncoding[(inVal=*(pInVal++))] < 16)
        {
            // encode 4-bit predefined index textIndex
            controlBit <<= 1; // control bit gets 0 for text index
            if (predefinedTCnt == 2)
            {
                // write a 0 to outVals[0] when first PTC encountered
                outVals[predefinedTCsOut] = (unsigned char)predefinedTCs;
                predefinedTCsOut = nextOutVal++;
                predefinedTCs = textEncoding[inVal];
                predefinedTCnt = 1;
            }
            else
            {
                predefinedTCs |= textEncoding[inVal] << 4;
                predefinedTCnt++;
            }
        }
        else
        {
            // encode 8-bit value
            // controlByte gets a 1 at this bit position for next undefined value
            controlByte |= controlBit;
            controlBit <<= 1;
            outVals[nextOutVal++] = (unsigned char)inVal;
            if (nextOutVal*8 > maxBits)
            {
                // main verifies only 1/2 of 5/16 of data values are text
                return 0; // data failed to compress
            }
        }
    }
    if (nextOutVal*8 > maxBits)
    {
        // main verifies only 1/2 of 5/16 of data values are text
        return 0; // data failed to compress
    }

    // output control bytes for nValues
    switch ((nValues-1)/8)
    {
        case 0:
            outVals[1] = (unsigned char)controlByte;
            break;
        case 1:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            break;
        case 2:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            outVals[3] = (unsigned char)(controlByte>>16);
            break;
        case 3:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            outVals[3] = (unsigned char)(controlByte>>16);
            outVals[4] = (unsigned char)(controlByte>>24);
            break;
        case 4:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            outVals[3] = (unsigned char)(controlByte>>16);
            outVals[4] = (unsigned char)(controlByte>>24);
            outVals[5] = (unsigned char)(controlByte>>32);
            break;
        case 5:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            outVals[3] = (unsigned char)(controlByte>>16);
            outVals[4] = (unsigned char)(controlByte>>24);
            outVals[5] = (unsigned char)(controlByte>>32);
            outVals[6] = (unsigned char)(controlByte>>40);
            break;
        case 6:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            outVals[3] = (unsigned char)(controlByte>>16);
            outVals[4] = (unsigned char)(controlByte>>24);
            outVals[5] = (unsigned char)(controlByte>>32);
            outVals[6] = (unsigned char)(controlByte>>40);
            outVals[7] = (unsigned char)(controlByte>>48);
            break;
        case 7:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            outVals[3] = (unsigned char)(controlByte>>16);
            outVals[4] = (unsigned char)(controlByte>>24);
            outVals[5] = (unsigned char)(controlByte>>32);
            outVals[6] = (unsigned char)(controlByte>>40);
            outVals[7] = (unsigned char)(controlByte>>48);
            outVals[8] = (unsigned char)(controlByte>>56);
            break;
    }
    // output last byte of text char encoding
    outVals[predefinedTCsOut] = (unsigned char)predefinedTCs;
    
    return (int32_t)nextOutVal * 8; // round up to full byte
} // end encodeTextMode

// -----------------------------------------------------------------------------------
int32_t encodeSingleValueMode(unsigned char *inVals, unsigned char *outVals, const uint32_t nValues, int32_t singleValue)
// -----------------------------------------------------------------------------------
{
    // generate control bit 1 if single value, otherwise 0 plus 8-bit value
    unsigned char *pInVal=inVals;
    unsigned char *pLastInValPlusOne=inVals+nValues;
    uint32_t inVal;
    uint32_t nextOutVal=(nValues-1)/8+2; // allocate space for control bits in bytes following first
    uint64_t controlByte=0;
    uint64_t controlBit=1;
    
    // output indicator byte, number uniques is 0, followed by next bit set
    outVals[0] = 0x05; // indicate single value mode
    outVals[nextOutVal++] = (unsigned char)singleValue;
    while (pInVal < pLastInValPlusOne)
    {
        if ((inVal=*(pInVal++)) != (uint32_t)singleValue)
        {
            // encode 4-bit predefined index textIndex
            controlBit <<= 1;
            outVals[nextOutVal++] = (unsigned char)inVal;
        }
        else
        {
            // controlByte gets a 1 at this bit position to indicate single value
            controlByte |= controlBit;
            controlBit <<= 1;
        }
    }

    // output control bytes for nValues
    switch ((nValues-1)/8)
    {
        case 0:
            outVals[1] = (unsigned char)controlByte;
            break;
        case 1:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            break;
        case 2:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            outVals[3] = (unsigned char)(controlByte>>16);
            break;
        case 3:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            outVals[3] = (unsigned char)(controlByte>>16);
            outVals[4] = (unsigned char)(controlByte>>24);
            break;
        case 4:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            outVals[3] = (unsigned char)(controlByte>>16);
            outVals[4] = (unsigned char)(controlByte>>24);
            outVals[5] = (unsigned char)(controlByte>>32);
            break;
        case 5:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            outVals[3] = (unsigned char)(controlByte>>16);
            outVals[4] = (unsigned char)(controlByte>>24);
            outVals[5] = (unsigned char)(controlByte>>32);
            outVals[6] = (unsigned char)(controlByte>>40);
            break;
        case 6:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            outVals[3] = (unsigned char)(controlByte>>16);
            outVals[4] = (unsigned char)(controlByte>>24);
            outVals[5] = (unsigned char)(controlByte>>32);
            outVals[6] = (unsigned char)(controlByte>>40);
            outVals[7] = (unsigned char)(controlByte>>48);
            break;
        case 7:
            outVals[1] = (unsigned char)controlByte;
            outVals[2] = (unsigned char)(controlByte>>8);
            outVals[3] = (unsigned char)(controlByte>>16);
            outVals[4] = (unsigned char)(controlByte>>24);
            outVals[5] = (unsigned char)(controlByte>>32);
            outVals[6] = (unsigned char)(controlByte>>40);
            outVals[7] = (unsigned char)(controlByte>>48);
            outVals[8] = (unsigned char)(controlByte>>56);
            break;
    }
    
    return (int32_t)nextOutVal * 8; // round up to full byte
} // end encodeSingleValueMode

#define MIN_VALUES_7_BIT_MODE 16
int32_t encode7bits(const unsigned char *inVals, unsigned char *outVals, const uint32_t nValues)
{
    uint32_t nextOutVal=1;
    uint32_t nextInVal=0;
    uint32_t val1;
    uint32_t val2;
    
    if (nValues < MIN_VALUES_7_BIT_MODE)
        return 0;
    outVals[0] = 0x03; // indicate 7-bit mode
    // process groups of 8 bytes to output 7 bytes
    while (nextInVal + 7 < nValues)
    {
        // copy groups of 8 7-bit vals into 7 bytes
        val1 = inVals[nextInVal++];
        val2 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)(val1 | (val2 << 7));
        val1 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)((val2 >> 1) | (val1 << 6));
        val2 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)((val1 >> 2) | (val2 << 5));
        val1 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)((val2 >> 3) | (val1 << 4));
        val2 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)((val1 >> 4) | (val2 << 3));
        val1 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)((val2 >> 5) | (val1 << 2));
        val2 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)((val1 >> 6) | (val2 << 1));
    }
    while (nextInVal < nValues)
    {
        // output final values as full bytes because no bytes saved, only bits
        outVals[nextOutVal++] = inVals[nextInVal++];
        if (nextInVal == nValues)
            break;
        outVals[nextOutVal++] = inVals[nextInVal++];
        if (nextInVal == nValues)
            break;
        outVals[nextOutVal++] = inVals[nextInVal++];
        if (nextInVal == nValues)
            break;
        outVals[nextOutVal++] = inVals[nextInVal++];
        if (nextInVal == nValues)
            break;
        outVals[nextOutVal++] = inVals[nextInVal++];
        if (nextInVal == nValues)
            break;
        outVals[nextOutVal++] = inVals[nextInVal++];
        if (nextInVal == nValues)
            break;
        outVals[nextOutVal++] = inVals[nextInVal++];
            break;
    }
    return (int32_t)nextOutVal*8;
} // end encode7bits

static inline void esmOutputBits(unsigned char *outVals, const uint32_t nBits, const uint32_t bitVal, uint32_t *nextOutIx, uint32_t *nextOutBit)
{
    // output 1 to 8 bits
    outVals[*nextOutIx] |= bitVal << *nextOutBit;
    *nextOutBit += nBits;
    if (*nextOutBit >= 8)
    {
        *nextOutBit -= 8;
        outVals[++(*nextOutIx)] = (unsigned char)bitVal >> (nBits - *nextOutBit);
    }
    return;
} // end esmOutputBits

#define STRING_LIMIT 9
const uint32_t encodingBits[64]={1,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6};

int32_t encodeStringMode(const unsigned char *inVals, unsigned char *outVals, const uint32_t nValues, const uint32_t nUniquesIn, const uint32_t *uniqueOccurrence, const uint32_t earlyExit, const uint32_t maxBits)
{
    uint32_t twoValsPos[256]; // set to position+1 of first occurrence of value
    uint32_t nUniques; // first value is always a unique
    uint32_t nextOutIx;
    uint32_t nextOutBit;
    const uint32_t maxBytes=maxBits/8; // compare agains bytes out during loop
    
    outVals[0] = 1 | (unsigned char)((nUniquesIn-1)<<3); // indicate string mode in first 3 bits and number uniques - 1 in next 5 bits
    nextOutIx = nUniquesIn + 1; // start of encoding past uniques written from outer loop
    // output two initial values
    // first unique assumed
    const unsigned char inVal0=inVals[0];
    if (inVal0 == inVals[1])
    {
        // first two values are the same
        nUniques = 1;
        // output 1 to indicate first unique value repeated
        nextOutBit = 1;
        outVals[nextOutIx] = 1; // 1=repeat for second value
        twoValsPos[inVal0] = 1;
    }
    else
    {
        // second val is a new unique
        nUniques = 2;
        // set up position of 2nd unique
        nextOutBit = 1;
        outVals[nextOutIx] = 0; // 0=uniques in first two values
        twoValsPos[inVal0] = 1;
        twoValsPos[inVals[1]] = 2;
    }
    uint32_t inPos=2; // start loop after init of first two values
    const uint32_t lastPos=nValues-1;
    // early exit not currently used
    // use early exit to determine whether to check at half of values
    // when 7-bit mode can be used, it's quicker to complete string mode if it does succeed than return early and use 7-bit mode
    uint32_t halfNvalues;
    if (earlyExit)
        halfNvalues = nValues / 2; // used if 7-bit mode works for this data
    else
        halfNvalues = nValues;
    uint32_t nextInVal = inVals[2];
    while (inPos < lastPos)
    {
        if (nextOutIx > maxBytes)
            return 0; // already failed compression minimum
        // early exit not currently used
/*        if (inPos > halfNvalues)
        {
            // this check is reached only if 7-bit mode can be called if string mode fails
            if ((nextOutIx + ((nextOutIx-nUniquesIn)*2))*8 > maxBits)
                return 0; // misses some compressible blocks, though compression averages < 20%
            halfNvalues = nValues; // one-time check
        }*/
        const uint32_t inVal=nextInVal;
        const uint32_t uoInVal=uniqueOccurrence[inVal];
        nextInVal = inVals[++inPos]; // set up for next input as well as comparison later
        if (uoInVal > nUniques-1)
        {
            // first occurrence of this unique
            // output a 0 to indicate new unique
            if (++nextOutBit == 8)
            {
                // update out index and next out bit
                outVals[++nextOutIx] = 0;
                nextOutBit = 0;
            }
            twoValsPos[inVal] = inPos;
            nUniques++;
            continue;
        }
        uint32_t tvPos = twoValsPos[inVal];
        if (nextInVal == inVals[tvPos])
        {
            if (tvPos == inPos-1)
            {
                // pos of unique plus one and next input value match
                // output repeated value: 01 plus unique
                esmOutputBits(outVals, 2+encodingBits[nUniques-1], 1|(uoInVal<<2), &nextOutIx, &nextOutBit);
                continue;
                
            }
            // look for continuation of matching characters
            uint32_t strPos=inPos+1;
            uint32_t strLimit;
            if (inPos - tvPos < STRING_LIMIT)
                strLimit = inPos - tvPos; // don't take string past current input pos
            else
                strLimit = STRING_LIMIT;
            if (nValues-strPos+2 < strLimit)
                strLimit = nValues - strPos + 2; // don't go past end of input
            tvPos++; // point to 2 past original unique
            uint32_t strCount=1;
            while(++strCount < strLimit)
            {
                if (inVals[strPos] == inVals[tvPos])
                {
                    strPos++;
                    tvPos++;
                }
                else
                    break;
            }
            // output 11 plus 3 more bits for string length 2 to 9
            esmOutputBits(outVals, 5, 3 | ((strCount-2)<<2), &nextOutIx, &nextOutBit);
            // output the unique that started this string, which gives its position
            esmOutputBits(outVals, encodingBits[nUniques-1], uoInVal, &nextOutIx, &nextOutBit);
            inPos += strCount - 1;
            nextInVal = inVals[inPos]; // new next val after string
        }
        else
        {
            // this pair doesn't match the one for first occurrence of this unique
            // repeated value: 01
            esmOutputBits(outVals, 2+encodingBits[nUniques-1], 1|(uoInVal<<2), &nextOutIx, &nextOutBit);
        }
    }

    // output final bits
    if (nextOutBit > 0)
        nextOutIx++; // index past final bits
    if (inPos < nValues)
    {
        outVals[nextOutIx++] = inVals[lastPos]; // output last input byte
    }
    if (nextOutIx*8 <= maxBits)
        return (int32_t)nextOutIx * 8;
    return 0; // not compressible
} // end encodeStringMode

// -----------------------------------------------------------------------------------
int32_t td64(unsigned char *inVals, unsigned char *outVals, const uint32_t nValues)
// -----------------------------------------------------------------------------------
// td64: Compress nValues bytes. Return 0 if not compressible (no output bytes),
//    -1 if error; otherwise, number of bits written to outVals.
//    Management of whether compressible and number of input values must be maintained
//    by caller. Decdode requires number of input values and only accepts compressed data.
// Arguments:
//   inVals   input byte values
//   outVals  output byte values if compressed, max of inVals bytes
//   nValues  number of input byte values
// Returns number of bits compressed, 0 if not compressed, or -1 if error
{
    if (nValues <= 5)
        return td5(inVals, outVals, nValues);
    
    if (nValues > MAX_TD64_BYTES)
        return -1; // only values 2 to 64 supported
    
    uint32_t betterCompression=1; // under development--faster when disabled
    uint32_t highBitCheck=0;
    uint32_t predefinedTextCharCnt=0; // count of text chars encountered
    uint32_t uniqueOccurrence[256]; // order of occurrence of uniques
    uint32_t nUniqueVals=0; // count of unique vals encountered
    unsigned char val256[256]={0}; // init characters found to 0
    const uint32_t uniqueLimit=uniqueLimits25[nValues]; // if exceeded, return uncompressible by fixed bit coding
    const uint32_t nValsInitLoop=(nValues*5/16)+1;
    // save uniques for use after check for text mode
    unsigned char saveUniques[MAX_TD64_BYTES];
    uint32_t textModeCalled=0; // need to restore uniques for some modes after text mode called

    // process enough input vals to eliminate most random data and to check for text mode
    // for fixed bit coding find and output the uniques starting at outVal[1]
    //    and saving the unique occurrence value to be used when values are output
    // for 7 bit mode OR every value
    // for text mode count number of text characters
    // for single value mode accumulate frequency counts
    uint32_t inPos=0;
    while (inPos < nValsInitLoop)
    {
        const uint32_t inVal=inVals[inPos++];
        predefinedTextCharCnt += predefinedTextChars[inVal]; // count text chars for text char mode
        if (val256[inVal]++ == 0)
        {
            // first occurrence of value, for fixed bit coding:
            uniqueOccurrence[inVal] = nUniqueVals; // save occurrence count for this unique
            outVals[++nUniqueVals] = (unsigned char)inVal; // store unique starting at second byte
            highBitCheck |= inVal; // keep watch on high bit of unique values
        }
    }
    if (nUniqueVals > uniqueLimit+1)
    {
        // supported unique values exceeded
        if ((highBitCheck & 0x80) == 0)
        {
            // attempt to compress based on high bit clear across all values
            // confirm remaining values have high bit clear
            while (inPos < nValues)
                highBitCheck |= inVals[inPos++];
            if ((highBitCheck & 0x80) == 0)
                return encode7bits(inVals, outVals, nValues);
        }
        return 0; // too many uniques to compress with fixed bit coding, random data fails here
    }
    if (nUniqueVals >= uniqueLimit / 2)
    {
        // check text mode validity as it's not considered after this
        // standard text will have 3/4 text values, but for this case, use .5 because this number of values will compress for standard text data.
        // encodeTextMode verifies compression occurs
        const uint32_t minTextChars=nValsInitLoop / 2 + 1;
        if (predefinedTextCharCnt > minTextChars)
        {
            textModeCalled = nUniqueVals; // indicate for restore of uniques when used by subsequent mode
            memcpy(saveUniques, outVals+1, nUniqueVals);
            // encode in text mode if 12% compression achieved
            uint32_t retBits=encodeTextMode(inVals, outVals, nValues, nValues*7);
            if (retBits)
                return retBits;
        }
    }
    // continue fixed bit loop with checks for high bit set and repeat counts
    // look for minimum count to validate single value mode
    const uint32_t minRepeatsSingleValueMode=nValues<16 ? nValues/2 : (unsigned char)nValues/4+1;
    int32_t singleValue=-1; // set to value if min repeats reached
    while (inPos < nValues)
    {
        const uint32_t inVal=inVals[inPos++];
        if (val256[inVal]++ == 0)
        {
            // first occurrence of value, for fixed bit coding:
            uniqueOccurrence[inVal] = nUniqueVals; // save occurrence count for this unique
            outVals[++nUniqueVals] = (unsigned char)inVal; // store unique starting at second byte
            highBitCheck |= inVal;
        }
        else if (val256[inVal] >= minRepeatsSingleValueMode)
        {
            singleValue = (int32_t)inVal;
            break; // continue loop without further checking
        }
    }
    if (nUniqueVals <= uniqueLimit) // confirm unique limit has not been exceeded
    {
        // continue fixed bit loop with checks for high bit set and repeat counts, but not single value
        while (inPos < nValues)
        {
            const uint32_t inVal=inVals[inPos++];
            if (val256[inVal]++ == 0)
            {
                // first occurrence of value, for fixed bit coding:
                uniqueOccurrence[inVal] = nUniqueVals; // save occurrence count for this unique
                outVals[++nUniqueVals] = (unsigned char)inVal; // store unique starting at second byte
                highBitCheck |= inVal;
            }
        }
    }
    if (nUniqueVals > uniqueLimit)
    {
        // fixed bit coding failed, try for other compression modes
        if (singleValue >= 0)
        {
            return encodeSingleValueMode(inVals, outVals, nValues, singleValue);
        }
        if ((nValues >= MIN_VALUES_STRING_MODE) && (nUniqueVals <= MAX_STRING_MODE_UNIQUES))
        {
            // string mode for 32+ values with 32 or fewer uniques
            int32_t retBits;
            uint32_t earlyExit=(highBitCheck & 0x80) == 0; // perform early exit only if 7-bit mode works
            if (betterCompression)
                earlyExit = 0; // always complete string mode
            if (textModeCalled)
                memcpy(outVals+1, saveUniques, textModeCalled); // restore uniques to outVals starting in second byte
            uint32_t maxBits = nValues*7;
            if ((highBitCheck & 0x80) != 0)
                maxBits += nValues / 2; // set minimum compression to 6%
            if ((retBits=encodeStringMode(inVals, outVals, nValues, nUniqueVals, uniqueOccurrence, earlyExit, maxBits)))
                return retBits;
        }
        if ((highBitCheck & 0x80) == 0)
        {
            // compress in high bit mode
            return encode7bits(inVals, outVals, nValues);
        }
        return 0; // unable to compress
    }
    else if (nUniqueVals > 8)
    {
        // requires at least 38 input values to have 9 or more uniques
        const uint32_t singleValueOverFixexBitRepeats=nValues/2-nValues/16;
        if (val256[singleValue] >= singleValueOverFixexBitRepeats)
        {
            // favor single value over fixed 4-bit encoding
            return encodeSingleValueMode(inVals, outVals, nValues, singleValue);
        }
    }
    
    // process fixed bit coding
    if (textModeCalled)
        memcpy(outVals+1, saveUniques, textModeCalled); // restore uniques to outVals starting in second byte
    uint32_t i;
    uint32_t nextOut;
    uint32_t encodingByte;
    uint32_t ebShift;

    switch (nUniqueVals)
    {
        case 0:
            return -4; // unexpected program error
        case 1:
        {
            // 1 unique so all bytes same value
                outVals[0] = 0;
                outVals[1] = inVals[0];
                return 16; // return number of bits output
        }
        case 2:
        {
            // 2 uniques so 1 bit required for each input value
            encodingByte=2; // indicate 2 uniques in bits 1-4
            // fill in upper 3 bits for inputs 2, 3 and 4 (first is implied by first unique) and output
            encodingByte |= uniqueOccurrence[inVals[1]] << 5;
            encodingByte |= uniqueOccurrence[inVals[2]] << 6;
            encodingByte |= uniqueOccurrence[inVals[3]] << 7;
            outVals[0] = (unsigned char)encodingByte;
            i = 4;
            nextOut = 3;
            while (i + 7 < nValues)
            {
                encodingByte = (uniqueOccurrence[inVals[i++]]);
                encodingByte |= (uniqueOccurrence[inVals[i++]]) << 1;
                encodingByte |= (uniqueOccurrence[inVals[i++]]) << 2;
                encodingByte |= (uniqueOccurrence[inVals[i++]]) << 3;
                encodingByte |= (uniqueOccurrence[inVals[i++]]) << 4;
                encodingByte |= (uniqueOccurrence[inVals[i++]]) << 5;
                encodingByte |= (uniqueOccurrence[inVals[i++]]) << 6;
                encodingByte |= (uniqueOccurrence[inVals[i++]]) << 7;
                outVals[nextOut++] = (unsigned char)encodingByte;
            }
            if (i < nValues)
            {
                ebShift = 0;
                encodingByte = 0;
                while (i < nValues)
                {
                    encodingByte |= (uniqueOccurrence[inVals[i++]]) << ebShift;
                    ebShift++;
                }
                outVals[nextOut] = (unsigned char)encodingByte;
            }
            return (int)nValues-1 + 21; // one bit encoding for each value + 5 indicator bits + 2 uniques
        }
        case 3:
        {
            // include with 4 as two bits are used to encode each value
        }
        case 4:
        {
            // 3 or 4 uniques so 2 bits required for each input value
            if (nUniqueVals == 3)
                encodingByte = 4; // 3 uniques
            else
                encodingByte = 6; // 4 uniques
            encodingByte |= uniqueOccurrence[inVals[1]] << 5;
            // skipping last bit in first byte to be on even byte boundary
            outVals[0] = (unsigned char)encodingByte;
            nextOut = nUniqueVals + 1; // start output past uniques
            i = 2; // start input on third value (first is implied by first unique)
            while (i+3 < nValues)
            {
                encodingByte = uniqueOccurrence[inVals[i++]];
                encodingByte |= uniqueOccurrence[inVals[i++]] << 2;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 4;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 6;
                outVals[nextOut++] = (unsigned char)encodingByte;
            }
            if (i < nValues)
            {
                encodingByte = uniqueOccurrence[inVals[i++]];
                if (i < nValues)
                {
                    encodingByte |= uniqueOccurrence[inVals[i++]] << 2;
                    if (i < nValues)
                    {
                        encodingByte |= uniqueOccurrence[inVals[i]] << 4;
                    }
                }
                outVals[nextOut] = (unsigned char)encodingByte; // output last partial byte
            }
            return (int)(((nValues-1) * 2) + 6 + (nUniqueVals * 8)); // two bits for each value plus 6 indicator bits + 3 or 4 uniques
        }
        case 5:
        case 6:
        case 7:
        case 8:
        {
            // 3 bits to encode
            encodingByte = (nUniqueVals-1) << 1; // 5 to 8 uniques
            encodingByte |= uniqueOccurrence[inVals[1]] << 5; // first val
            outVals[0] = (unsigned char)encodingByte; // save first byte
            nextOut=nUniqueVals + 1; // start output past uniques
            // process 8 values and output 3 8-bit values
            i = 2; // second val in first byte (first val is implied by first unique)
            uint32_t partialVal;
            while (i + 7 < nValues)
            {
                encodingByte = uniqueOccurrence[inVals[i++]];
                encodingByte |= uniqueOccurrence[inVals[i++]] << 3;
                partialVal = inVals[i++];
                encodingByte |= uniqueOccurrence[partialVal] << 6;
                outVals[nextOut++] = (unsigned char)encodingByte;
                encodingByte = uniqueOccurrence[partialVal] >> 2;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 1;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 4;
                partialVal = inVals[i++];
                encodingByte |= uniqueOccurrence[partialVal] << 7;
                outVals[nextOut++] = (unsigned char)encodingByte;
                encodingByte = uniqueOccurrence[partialVal] >> 1;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 2;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 5;
                outVals[nextOut++] = (unsigned char)encodingByte;
            }
            // 0 to 7 vals left to process, check after each one for completion
            uint32_t partialByte=0;
            while (i < nValues)
            {
                partialByte = 1;
                encodingByte = uniqueOccurrence[inVals[i++]];
                if (i == nValues)
                    break;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 3;
                if (i == nValues)
                    break;
                encodingByte |= uniqueOccurrence[(partialVal=inVals[i++])] << 6;
                outVals[nextOut++] = (unsigned char)encodingByte;
                encodingByte = uniqueOccurrence[partialVal] >> 2;
                if (i == nValues)
                    break;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 1;
                if (i == nValues)
                    break;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 4;
                if (i == nValues)
                    break;
                encodingByte |= uniqueOccurrence[(partialVal=inVals[i++])] << 7;
                outVals[nextOut++] = (unsigned char)encodingByte;
                encodingByte = uniqueOccurrence[partialVal] >> 1;
                if (i == nValues)
                    break;
                encodingByte |= uniqueOccurrence[inVals[i++]] << 2;
            }
            if (partialByte)
                outVals[nextOut] = (unsigned char)encodingByte;

            return (int)(((nValues-1) * 3) + 5 + (nUniqueVals * 8)); // three bits for each value plus 5 indicator bits
            }
        default: // nUniques 9 through 16
        {
            if (nUniqueVals > MAX_UNIQUES)
                return -5; // unexpected program error
            // cases 9 through 16 take 4 bits to encode
            // skipping last 3 bits in first byte to be on even byte boundary
            outVals[0] = (unsigned char)((nUniqueVals-1) << 1);
            nextOut = nUniqueVals + 1; // start output past uniques
            i = 1; // first value is implied by first unique
            while (i + 7 < nValues)
            {
                encodingByte = uniqueOccurrence[inVals[i++]];
                outVals[nextOut++] = (unsigned char)(encodingByte | (uniqueOccurrence[inVals[i++]] << 4));
                encodingByte = uniqueOccurrence[inVals[i++]];
                outVals[nextOut++] = (unsigned char)(encodingByte | (uniqueOccurrence[inVals[i++]] << 4));
                encodingByte = uniqueOccurrence[inVals[i++]];
                outVals[nextOut++] = (unsigned char)(encodingByte | (uniqueOccurrence[inVals[i++]] << 4));
                encodingByte = uniqueOccurrence[inVals[i++]];
                outVals[nextOut++] = (unsigned char)(encodingByte | (uniqueOccurrence[inVals[i++]] << 4));
            }
            while (i < nValues)
            {
                // 1 to 7 values remain
                encodingByte = uniqueOccurrence[inVals[i++]];
                if (i < nValues)
                {
                    outVals[nextOut++] = (unsigned char)(encodingByte | (uniqueOccurrence[inVals[i++]] << 4));
                }
                else
                {
                    outVals[nextOut] = (unsigned char)encodingByte;
                    break;
                }
            }
            return (int)(((nValues-1) * 4) + 8 + (nUniqueVals * 8)); // four bits for each value plus 8 indicator bits + 9 to 16 uniques
        }
    }
    return -6; // unexpected program error
} // end td64

// -----------------------------------------------------------------------------------
int32_t decodeTextMode(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed)
// -----------------------------------------------------------------------------------
{
    uint32_t nextInVal=(nOriginalValues-1)/8+2;
    uint32_t nextOutVal=0;
    uint64_t controlByte=0;
    uint64_t controlBit=1;
    uint32_t predefinedTCs=0;
    uint32_t predefinedTCnt=1; // 1 = first 4-bit PTC is encoded for output, otherwise no

    // read in control bits starting from second byte
    switch (nextInVal-1)
    {
        case 1:
            controlByte = inVals[1];
            break;
        case 2:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            break;
        case 3:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            controlByte |= (uint64_t)inVals[3]<<16;
            break;
        case 4:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            controlByte |= (uint64_t)inVals[3]<<16;
            controlByte |= (uint64_t)inVals[4]<<24;
            break;
        case 5:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            controlByte |= (uint64_t)inVals[3]<<16;
            controlByte |= (uint64_t)inVals[4]<<24;
            controlByte |= (uint64_t)inVals[5]<<32;
            break;
        case 6:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            controlByte |= (uint64_t)inVals[3]<<16;
            controlByte |= (uint64_t)inVals[4]<<24;
            controlByte |= (uint64_t)inVals[5]<<32;
            controlByte |= (uint64_t)inVals[6]<<40;
            break;
        case 7:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            controlByte |= (uint64_t)inVals[3]<<16;
            controlByte |= (uint64_t)inVals[4]<<24;
            controlByte |= (uint64_t)inVals[5]<<32;
            controlByte |= (uint64_t)inVals[6]<<40;
            controlByte |= (uint64_t)inVals[7]<<48;
            break;
        case 8:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            controlByte |= (uint64_t)inVals[3]<<16;
            controlByte |= (uint64_t)inVals[4]<<24;
            controlByte |= (uint64_t)inVals[5]<<32;
            controlByte |= (uint64_t)inVals[6]<<40;
            controlByte |= (uint64_t)inVals[7]<<48;
            controlByte |= (uint64_t)inVals[8]<<56;
            break;
    }
    while (nextOutVal < nOriginalValues)
    {
        if (controlByte & controlBit)
        {
            // read in and output next 8-bit value
            outVals[nextOutVal++] = inVals[nextInVal++];
        }
        else
        {
            // use predefined text chars based on 4-bit index
            if (predefinedTCnt == 1)
            {
                predefinedTCs = inVals[nextInVal++];
                outVals[nextOutVal++] = (unsigned char)textChars[(unsigned char)predefinedTCs & 15];
                predefinedTCnt = 0;
            }
            else
            {
                outVals[nextOutVal++] = (unsigned char)textChars[predefinedTCs >> 4];
                predefinedTCnt++;
            }
        }
        controlBit <<= 1;
    }
    *bytesProcessed = nextInVal;
    return (int32_t)nOriginalValues;
} // end decodeTextMode

// -----------------------------------------------------------------------------------
int32_t decodeSingleValueMode(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed)
// -----------------------------------------------------------------------------------
{
    uint32_t nextInVal=(nOriginalValues-1)/8+2;
    uint32_t nextOutVal=0;
    uint64_t controlByte=0;
    uint64_t controlBit=1;
    unsigned char singleValue;

    // read in control bits starting from second byte
    switch (nextInVal-1)
    {
        case 1:
            controlByte = inVals[1];
            break;
        case 2:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            break;
        case 3:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            controlByte |= (uint64_t)inVals[3]<<16;
            break;
        case 4:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            controlByte |= (uint64_t)inVals[3]<<16;
            controlByte |= (uint64_t)inVals[4]<<24;
            break;
        case 5:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            controlByte |= (uint64_t)inVals[3]<<16;
            controlByte |= (uint64_t)inVals[4]<<24;
            controlByte |= (uint64_t)inVals[5]<<32;
            break;
        case 6:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            controlByte |= (uint64_t)inVals[3]<<16;
            controlByte |= (uint64_t)inVals[4]<<24;
            controlByte |= (uint64_t)inVals[5]<<32;
            controlByte |= (uint64_t)inVals[6]<<40;
            break;
        case 7:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            controlByte |= (uint64_t)inVals[3]<<16;
            controlByte |= (uint64_t)inVals[4]<<24;
            controlByte |= (uint64_t)inVals[5]<<32;
            controlByte |= (uint64_t)inVals[6]<<40;
            controlByte |= (uint64_t)inVals[7]<<48;
            break;
        case 8:
            controlByte = inVals[1];
            controlByte |= (uint64_t)inVals[2]<<8;
            controlByte |= (uint64_t)inVals[3]<<16;
            controlByte |= (uint64_t)inVals[4]<<24;
            controlByte |= (uint64_t)inVals[5]<<32;
            controlByte |= (uint64_t)inVals[6]<<40;
            controlByte |= (uint64_t)inVals[7]<<48;
            controlByte |= (uint64_t)inVals[8]<<56;
            break;
    }
    singleValue = inVals[nextInVal++]; // single value output when control bit is 1
    while (nextOutVal < nOriginalValues)
    {
        if (controlByte & controlBit)
        {
            // output single value
            outVals[nextOutVal++] = singleValue;
        }
        else
        {
            // output next value from input
            outVals[nextOutVal++] = inVals[nextInVal++];
        }
        controlBit <<= 1;
    }
    *bytesProcessed = nextInVal;
    return (int32_t)nOriginalValues;
} // end decodeTextMode

int32_t decode7bits(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed)
{
    uint32_t nextOutVal=0;
    uint32_t nextInVal=1;
    uint32_t val1;
    uint32_t val2;
    
    // process groups of 8 bytes to output 7 bytes
    while (nextOutVal + 7 < nOriginalValues)
    {
        // move groups of 7-bit vals into 8 bytes
        val1 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)(val1 & 127);
        val2 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)(((val2 << 1) & 127) | (val1 >> 7));
        val1 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)(((val1 << 2) & 127) | (val2 >> 6));
        val2 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)(((val2 << 3) & 127) | (val1 >> 5));
        val1 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)(((val1 << 4) & 127) | (val2 >> 4));
        val2 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)(((val2 << 5) & 127) | (val1 >> 3));
        val1 = inVals[nextInVal++];
        outVals[nextOutVal++] = (unsigned char)(((val1 << 6) & 127) | (val2 >> 2));
        outVals[nextOutVal++] = (unsigned char)val1 >> 1;
    }
    while (nextOutVal < nOriginalValues)
    {
        // output final values as full bytes because no bytes saved, only bits
        outVals[nextOutVal++] = inVals[nextInVal++];
        if (nextOutVal == nOriginalValues)
            break;
        outVals[nextOutVal++] = inVals[nextInVal++];
        if (nextOutVal == nOriginalValues)
            break;
        outVals[nextOutVal++] = inVals[nextInVal++];
        if (nextOutVal == nOriginalValues)
            break;
        outVals[nextOutVal++] = inVals[nextInVal++];
        if (nextOutVal == nOriginalValues)
            break;
        outVals[nextOutVal++] = inVals[nextInVal++];
        if (nextOutVal == nOriginalValues)
            break;
        outVals[nextOutVal++] = inVals[nextInVal++];
        if (nextOutVal == nOriginalValues)
            break;
        outVals[nextOutVal++] = inVals[nextInVal++];
            break;
    }
    *bytesProcessed = nextInVal;
    return (int32_t)nOriginalValues;
} // end decode7bits

static inline void dsmGetBits(const unsigned char *inVals, const uint32_t nBitsToGet, uint32_t *thisInVal, uint32_t *bitPos, int32_t *theBits)
{
    // get 1 to 9 bits from inVals into theBits
    *theBits = inVals[*thisInVal] >> (*bitPos);
    if ((*bitPos += nBitsToGet) < 8)
    {
        // all bits in current value with bits to spare
        *theBits &= bitMask[nBitsToGet];
        return;
    }
    if (*bitPos == 8)
    {
        // bits to get use remainder of current value
        *bitPos = 0;
        (*thisInVal)++;
        return;
    }
    else if (*bitPos == 16)
    {
        *theBits |= (inVals[++(*thisInVal)]) << 1; // must be 9 bits
        (*thisInVal)++;
        *bitPos = 0;
        return;
    }
    // bits split between two values with bits left (bitPos > 0)
    *bitPos -= 8;
    *theBits |= (inVals[++(*thisInVal)]) << (nBitsToGet-*bitPos);
    *theBits &= bitMask[nBitsToGet];
    return;
} // end dsmGetBits

int32_t decodeStringMode(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed)
{
    uint32_t nextOutVal;
    uint32_t thisInVal;
    uint32_t bitPos;
    int32_t theBits;
    uint32_t nUniques;
    uint32_t uPos[MAX_STRING_MODE_UNIQUES];
    
    // process one of three encodings:
    // 0   new unique value
    // 01  repeated value by original unique count
    // 11  two or more values of a repeated string with string count and position of first occurrence
    // first value is always the first unique
    outVals[0] = inVals[1]; // unique vals start at second val
    thisInVal = (inVals[0] >> 3) + 2; // number uniques in bits 1-5
    uPos[0] = 0; // unique position for first unique
    if (inVals[thisInVal] & 1)
    {
        // second value matches first
        outVals[1] = outVals[0];
        bitPos = 1;
        nUniques = 1;
    }
    else
    {
        // second value is a unique
        outVals[1] = inVals[2];
        bitPos = 1;
        nUniques = 2;
        uPos[1] = 1; // unique position for second unique
    }
    nextOutVal=2; // start with third output value
    const uint32_t nOrigMinus1=nOriginalValues-1;
    while (nextOutVal < nOrigMinus1)
    {
        if ((inVals[thisInVal] & (1 << (bitPos))) == 0)
        {
            // new unique value
            uPos[nUniques] = nextOutVal; // unique position for first unique
            nUniques++;
            if (bitPos == 7)
            {
                // unique from start of input values
                outVals[nextOutVal++] = inVals[nUniques];
                thisInVal++;
                bitPos = 0;
            }
            else
            {
                outVals[nextOutVal++] = inVals[nUniques];
                bitPos++;
            }
        }
        else
        {
            // repeat or string
            if (++bitPos == 8)
            {
                thisInVal++;
                bitPos = 0;
            }
            if ((inVals[thisInVal] & (1 << (bitPos))) == 0)
            {
                // repeat: next number of bits, determined by nUniques, indicates repeated value
                if (++bitPos == 8)
                {
                    thisInVal++;
                    bitPos = 0;
                }
                uint32_t nUniqueBits = encodingBits[nUniques-1]; // current number uniques determines 1-5 bits used
                dsmGetBits(inVals, nUniqueBits, &thisInVal, &bitPos, &theBits);
                outVals[nextOutVal++] = inVals[theBits+1]; // get uniques from start of inVals
            }
            else
            {
                // string: 11 plus string length of 2 to 9 in 3 bits
                if (++bitPos == 8)
                {
                    thisInVal++;
                    bitPos = 0;
                }
                // multi-character string: location of values in bits needed to code current pos
/*                uint32_t nPosBits = encodingBits[nextOutVal];*/
                uint32_t nPosBits = encodingBits[nUniques-1];
                dsmGetBits(inVals, 3+nPosBits, &thisInVal, &bitPos, &theBits);
                uint32_t stringLen = (uint32_t)(theBits & 7) + 2;
                assert(stringLen <= STRING_LIMIT);
                uint32_t stringPos=uPos[(uint32_t)theBits >> 3];
                assert((uint32_t)stringPos+stringLen <= nextOutVal);
                assert(nextOutVal+stringLen <= nOriginalValues);
                memcpy(outVals+nextOutVal, outVals+stringPos, stringLen);
                nextOutVal += stringLen;
            }
        }
    }
    if (bitPos > 0)
        thisInVal++; // inc past partial input value
    if (nextOutVal == nOrigMinus1)
    {
        // output last byte in input when not ending with a string
        // string at end will catch last byte
        outVals[nOrigMinus1] = inVals[thisInVal++];
    }
    *bytesProcessed = thisInVal;
    return (int32_t)nOriginalValues;
} // end decodeStringMode

// -----------------------------------------------------------------------------------
int32_t td64d(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed)
// -----------------------------------------------------------------------------------
// fixed bit decoding requires number of original values and encoded bytes
// uncompressed data is not acceppted
// 2 to 16 unique values were encoded for 2 to 64 input values.
// 2 to 5 input values are handled separately.
// inVals   compressed data with fewer bits than in original values
// outVals  decompressed data
// nOriginalalues  number of values in the original input to td64: required input
// return number of bytes output or -1 if error
{
    if (nOriginalValues <= 5)
        return td5d(inVals, outVals, nOriginalValues, bytesProcessed);
    
    if (nOriginalValues > MAX_TD64_BYTES)
        return -2;
        
    // first bit of first byte 1: decode one of four modes
    const unsigned char firstByte=inVals[0];
    if ((firstByte & 7) == 0x01)
    {
        // string mode
/*        return decodeTwoValueMode(inVals, outVals, nOriginalValues, bytesProcessed);*/
        return decodeStringMode(inVals, outVals, nOriginalValues, bytesProcessed);
    }
    if ((firstByte & 7) == 0x03)
    {
        // 7-bit mode
        return decode7bits(inVals, outVals, nOriginalValues, bytesProcessed);
    }
    if ((firstByte & 7) == 0x05)
    {
        // single value mode
        return decodeSingleValueMode(inVals, outVals, nOriginalValues, bytesProcessed);
    }
    if ((firstByte & 7) == 0x07)
    {
        // text mode using predefined text chars
        return decodeTextMode(inVals, outVals, nOriginalValues, bytesProcessed);
    }
    
    // first bit of first byte 0: fixed bit coding
    const uint32_t nUniques = ((firstByte >> 1) & 0xf) + 1;
    uint32_t uniques[MAX_UNIQUES];
    unsigned char uniques1;
    unsigned char uniques2;
    uint32_t nextInVal;
    int32_t nextOutVal;
    unsigned char inByte;
    switch (nUniques)
    {
        case 1:
        {
            // process single unique
            unsigned char unique = inVals[1];
            memset(outVals, unique, nOriginalValues);
            *bytesProcessed = 2;
            return (int)nOriginalValues;
        }
        case 2:
        {
            // 1-bit values
            uniques1 = inVals[1];
            uniques2 = inVals[2];
            outVals[0] = (unsigned char)uniques1;
            outVals[1] = (unsigned char)(((firstByte >> 5) & 1) ? uniques2 : uniques1);
            outVals[2] = (unsigned char)(((firstByte >> 6) & 1) ? uniques2 : uniques1);
            outVals[3] = (unsigned char)(((firstByte >> 7) & 1) ? uniques2 : uniques1);
            nextInVal=3;
            nextOutVal=4;
            while (nextOutVal+7 < nOriginalValues)
            {
                inByte = inVals[nextInVal++];
                outVals[nextOutVal++] = (unsigned char)((inByte & 1) ? uniques2 : uniques1);
                outVals[nextOutVal++] = (unsigned char)((inByte & 2) ? uniques2 : uniques1);
                outVals[nextOutVal++] = (unsigned char)((inByte & 4) ? uniques2 : uniques1);
                outVals[nextOutVal++] = (unsigned char)((inByte & 8) ? uniques2 : uniques1);
                outVals[nextOutVal++] = (unsigned char)((inByte & 16) ? uniques2 : uniques1);
                outVals[nextOutVal++] = (unsigned char)((inByte & 32) ? uniques2 : uniques1);
                outVals[nextOutVal++] = (unsigned char)((inByte & 64) ? uniques2 : uniques1);
                outVals[nextOutVal++] = (unsigned char)((inByte & 128) ? uniques2 : uniques1);
            }
            while (nextOutVal < nOriginalValues)
            {
                inByte = inVals[nextInVal++];
                outVals[nextOutVal++] = (unsigned char)((inByte & 1) ? uniques2 : uniques1);
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (unsigned char)((inByte & 2) ? uniques2 : uniques1);
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (unsigned char)((inByte & 4) ? uniques2 : uniques1);
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (unsigned char)((inByte & 8) ? uniques2 : uniques1);
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (unsigned char)((inByte & 16) ? uniques2 : uniques1);
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (unsigned char)((inByte & 32) ? uniques2 : uniques1);
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (unsigned char)((inByte & 64) ? uniques2 : uniques1);
            }
            *bytesProcessed = nextInVal;
            return (int)nOriginalValues;
        }
        case 3:
        case 4:
        {
            // 2-bit values
            uniques[0] = inVals[1];
            uniques[1] = inVals[2];
            uniques[2] = inVals[3];
            if (nUniques == 4)
            {
                uniques[3] = inVals[4];
                nextInVal = 5; // for 4 uniques
            }
            else
                nextInVal = 4; // for 3 uniques
            outVals[0] = (unsigned char)uniques[0];
            outVals[1] = (unsigned char)uniques[(firstByte >> 5)&3];
            nextOutVal = 2; // skip high bit of first byte
            while (nextOutVal + 3 < nOriginalValues)
            {
                inByte = inVals[nextInVal++];
                outVals[nextOutVal++] = (unsigned char)uniques[inByte&3];
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte>>2)&3];
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte>>4)&3];
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte>>6)&3];
            }
            while (nextOutVal < nOriginalValues)
            {
                inByte = inVals[nextInVal++];
                outVals[nextOutVal++] = (unsigned char)uniques[inByte&3];
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte>>2)&3];
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte>>4)&3];
            }
            *bytesProcessed = nextInVal;
            return (int)nOriginalValues;
        }
        case 5:
        case 6:
        case 7:
        case 8:
        {
            // 3-bit values for 5 to 8 uniques
            for (uint32_t i=0; i<nUniques; i++)
                uniques[i] = inVals[i+1];
            nextInVal = nUniques + 1; // for 4 uniques
            outVals[0] = (unsigned char)uniques[0];
            outVals[1] = (unsigned char)uniques[(firstByte >> 5)&7];
            nextOutVal = 2;
            uint32_t inByte2;
            uint32_t inByte3;
            while (nextOutVal + 7 < nOriginalValues)
            {
                inByte = inVals[nextInVal++];
                inByte2 = inVals[nextInVal++];
                inByte3 = inVals[nextInVal++];
                outVals[nextOutVal++] = (unsigned char)uniques[inByte&7];
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte>>3)&7];
                outVals[nextOutVal++] = (unsigned char)uniques[((inByte>>6) | (inByte2<<2))&7];
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte2>>1)&7];
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte2>>4)&7];
                outVals[nextOutVal++] = (unsigned char)uniques[((inByte2>>7) | (inByte3<<1))&7];
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte3>>2)&7];
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte3>>5)&7];
            }
            while (nextOutVal < nOriginalValues)
            {
                inByte = inVals[nextInVal++];
                outVals[nextOutVal++] = (unsigned char)uniques[inByte&7];
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte>>3)&7];
                if (nextOutVal == nOriginalValues)
                    break;
                inByte2 = inVals[nextInVal++];
                outVals[nextOutVal++] = (unsigned char)uniques[((inByte>>6) | (inByte2<<2))&7];
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte2>>1)&7];
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte2>>4)&7];
                if (nextOutVal == nOriginalValues)
                    break;
                inByte3 = inVals[nextInVal++];
                outVals[nextOutVal++] = (unsigned char)uniques[((inByte2>>7) | (inByte3<<1))&7];
                if (nextOutVal == nOriginalValues)
                    break;
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte3>>2)&7];
            }
            *bytesProcessed = nextInVal;
            return (int)nOriginalValues;
        }
        default:
        {
            // 4-bit values for 9 to 16 uniques
            if (nUniques > MAX_UNIQUES)
                return -7; // unexpected program error
            for (uint32_t i=0; i<nUniques; i++)
                uniques[i] = inVals[i+1];
            nextInVal = nUniques + 1; // skip past uniques
            outVals[0] = (unsigned char)uniques[0];
            nextOutVal = 1;
            while (nextOutVal + 3 < nOriginalValues)
            {
                inByte = inVals[nextInVal++];
                outVals[nextOutVal++] = (unsigned char)uniques[inByte&0xf];
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte>>4)];
                inByte = inVals[nextInVal++];
                outVals[nextOutVal++] = (unsigned char)uniques[inByte&0xf];
                outVals[nextOutVal++] = (unsigned char)uniques[(inByte>>4)];
            }
            if (nextOutVal < nOriginalValues)
            {
                // 1 to 3 values remain
                if (nextOutVal + 1 < nOriginalValues)
                {
                    // 2 values
                    inByte = inVals[nextInVal++];
                    outVals[nextOutVal++] = (unsigned char)uniques[inByte&0xf];
                    outVals[nextOutVal++] = (unsigned char)uniques[(inByte>>4)];
                }
                if (nextOutVal < nOriginalValues)
                    outVals[nextOutVal++] = (unsigned char)uniques[inVals[nextInVal++] & 0xf];
            }
            *bytesProcessed = nextInVal;
            return (int)nOriginalValues;
        }
    }
    return -8; // unexpected program error
} // end td64d
