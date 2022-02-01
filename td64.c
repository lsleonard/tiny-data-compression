//
//  td64.c
//  high-speed lossless tiny data compression of 1 to 64 bytes based on td64
//
//  Created by L. Stevan Leonard on 3/21/20.
//  Copyright © 2020-2022 L. Stevan Leonard. All rights reserved.
/*
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "td64.h"
#include "td64_internal.h"
#include "tdString.h"

#ifdef TD64_TEST_MODE
// these globals are used to collect info
uint32_t g_td64FailedTextMode=0;
uint32_t g_td64FailedStringMode=0;
uint32_t g_td64MaxStringModeUniquesExceeded=0;
uint32_t g_td64Text8bitCount=0;
uint32_t g_td64AdaptiveText8bitCount=0;
uint32_t g_td64StringBlocks=0;
double g_td64CompressNonSingleValues=0;
double g_td64CompressNSVcnt=0;
uint32_t g_td64CompressNSVblocks=0;
uint32_t g_td64CompressNSVfailures=0;
uint32_t g_td64nNSVcnt=0;
#endif

// fixed bit compression (fbc): for the number of uniques in input, the minimum number of input values for 25% compression
// uniques   1  2  3  4  5   6   7   8   9   10  11  12  13  14  15  16
// nvalues   2, 4, 7, 9, 15, 17, 19, 23, 40, 44, 48, 52, 56, 60, 62, 64};
static const uint32_t uniqueLimits25[MAX_TD64_BYTES+1]=
//      2    4      7    9            15   17   19       23
{ 0, 0, 1,1, 2,2,2, 3,3, 4,4,4,4,4,4, 5,5, 6,6, 7,7,7,7, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
//  40       44           48           52           56           60     62     64
    9,9,9,9, 10,10,10,10, 11,11,11,11, 12,12,12,12, 13,13,13,13, 14,14, 15,15, 16
};

// text mode with 16 fixed 4-bit characters
#define MAX_PREDEFINED_CHAR_COUNT 16 // text mode: based on character frequencies from Morse code
static const uint32_t textChars[MAX_PREDEFINED_CHAR_COUNT]={
    ' ', 'e', 't', 'a', 'i', 'n', 'o', 's', 'h', 'r', 'd', 'l', 'u', 'c', 'm', 'f'
};

// text mode: a one indicates a character from textChars
static const uint32_t predefinedTextChars[256]={
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 1, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1,
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
    16, 3, 16, 13, 10, 1, 15, 16, 8, 4, 16, 16, 11, 14, 5, 6,
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

// adaptive text mode where characters have a variable number of bits
#define MAX_PREDEFINED_FREQUENCY_CHAR_COUNT 23 // adaptive text mode: most frequent characters from Morse code plus CR and comma
#define MAX_PREDEFINED_ADAPTIVE_CHAR_COUNT 23
static const uint32_t extendedTextChars[MAX_PREDEFINED_FREQUENCY_CHAR_COUNT]={
    ' ', 'e', 't', 'a', 'i', 'n', 'o', 's', 'h', 'r', 'd', 'l', 'u', 'c', 0xA, 'm', 'g', 'f', ',', 'y', 'w', 'p', 'b'
};
static const uint32_t XMLTextChars[MAX_PREDEFINED_ADAPTIVE_CHAR_COUNT]={
    ' ', '/', '<', '>', 'e', 't', 'a', 'i', 'n', 'o', 's', 'h', 'r', 'd', 'l', 0xA, '.', 'u', 'w', 'c', 0x27, '"', ':'
};
static const uint32_t CTextChars[MAX_PREDEFINED_ADAPTIVE_CHAR_COUNT]={
    ' ', 'e', 't', 'a', 'i', 'n', 'o', 's', 'h', 'r', 'd', 'l', '*', '=', '\t', 0xA, 'c', ';', 'f', '(', ')', 0x27, '/'
};

//  adaptive text mode: a one indicates a character from the most frequent characters based on Morse code set plus a few non-alpha chars
const uint32_t predefinedBitTextChars[256]={
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, // 0xA
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, // space, comma
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, // a,b,c,d,e,f,g,h,i,l,m,n,o
    1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, // p,r,s,t,u,w,y
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// adaptive text mode: index to predefined char or 99 if another value
static const uint32_t extendedTextEncoding[256]={
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 14, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
     0, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 18, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99,  3, 22, 13, 10,  1, 17, 16,  8,  4, 99, 99, 11, 15,  5,  6,
    21, 99,  9,  7,  2, 12, 99, 20, 99, 19, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99
    };

// adaptive text mode: values set to frequently occurring characters in XML or HTML data, see initAdaptiveTextMode
static uint32_t adaptiveXMLEncoding[256]={
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99
    };

// adaptive text mode: values set to frequently occurring characters in C or other programming language data, see initAdaptiveTextMode
static uint32_t adaptiveCEncoding[256]={
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99
    };

int32_t td5(const unsigned char *inVals, unsigned char *outVals, const uint32_t nValues)
// Compress 1 to 5 values
// Management of whether compressible and number of input values must be maintained
//    by caller. Decdode requires number of input values and only accepts compressed data.
// Decode first byte:
// 01 = single unique, followed by low 6 bits of input then high two bits in second byte
// 11 = text mode characters, followed by a 4-bit character reference for each input value; all values must be one of the 16 textChars
// 0 = encoding by number of values:
//    1 byte: text mode
//    2 bytes: two unique nibbles
//    3 bytes: two unique nibbles
//    4 or 5 bytes: two uniques
// Arguments:
//   inVals   input data
//   outVals  compressed data
//   nValues  number of values to compress
// returns number of bits compressed, 0 if not compressible, or negative value if error
{
    outVals[0] = 1; // default uncompressed reason is general failure
    switch (nValues)
    {
        case 1:
        {
            // text mode encoding for 1 byte
            const uint32_t ival1=(unsigned char)inVals[0];
            if (predefinedTextChars[ival1])
            {
                // output 0 in first bit followed by encoding in next four
                outVals[0] = (unsigned char)textEncoding[ival1] << 1;
                return 5;
            }
            return 0;
        }
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
            if (ival1 == ival2 && ival1 == ival3 && ival1 == ival4)
            {
                if (nValues == 4)
                {
                    // all 4 values equal
                    outVals[0] = (unsigned char)(ival1 << 2) | 1;
                    outVals[1] = (unsigned char)ival1 >> 6;
                    return 10;
                }
                else if (ival1 == inVals[4])
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
                otherVal = ival2;
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
            return -9; // number of values specified not supported (1 to 5 handled here)
    }
} // end td5

int32_t td5d(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed)
// Decode 1 to 5 values encoded by td5.
// Decode first byte:
// 01 = single unique, followed by low 6 bits of input; otherwise, high two bits in second byte
// 11 = text encoding: replace 4-bit text index by corresponding char
//      1 to 4 values requires all values text, 5 chars can have final char non-text, indicated by 0 or 1 bit following 4 text indexes
// 0 = encoding by number of values:
//    1 byte: text mode
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
        uint32_t unique = (firstByte >> 2) | (inVals[1] << 6);
        memset(outVals, (unsigned char)unique, nOriginalValues);
        *bytesProcessed = 2;
        return (int)nOriginalValues;
    }
    uint32_t val1;
    uint32_t val2;
    const uint32_t secondByte = (unsigned char)inVals[1];
    switch (nOriginalValues)
    {
        case 1:
        {
            // text mode encoding for 1 byte
            outVals[0] = (unsigned char)textChars[(firstByte>>1) & 0xf];
            *bytesProcessed = 1;
            return 1;
        }
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
                return 4;
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
                    outVals[4] = (unsigned char)textChars[(thirdByte >> 3) & 0xf];
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

static inline void esmOutputBits(unsigned char *outVals, const uint32_t nBits, const uint32_t bitVal, uint32_t *nextOutIx, uint32_t *nextOutBit)
{
    // output 1 to 8 bits
    outVals[*nextOutIx] |= (unsigned char)(bitVal << *nextOutBit);
    *nextOutBit += nBits;
    if (*nextOutBit >= 8)
    {
        *nextOutBit -= 8;
        outVals[++(*nextOutIx)] = (unsigned char)bitVal >> (nBits - *nextOutBit);
    }
} // end esmOutputBits

static uint32_t textNBitsTable[MAX_PREDEFINED_FREQUENCY_CHAR_COUNT]={
    3, 3, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    7, 7, 7, 7
};
static uint32_t textBitValTable[MAX_PREDEFINED_FREQUENCY_CHAR_COUNT]={
    1, 3, 7, 15,
    0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28,
    0x1e, 0x3e, 0x5e, 0x7e
};

void initAdaptiveTextMode(void)
{
    adaptiveXMLEncoding[' '] = 0;
    adaptiveXMLEncoding['/'] = 1;
    adaptiveXMLEncoding['<'] = 2;
    adaptiveXMLEncoding['>'] = 3;
    adaptiveXMLEncoding['e'] = 4;
    adaptiveXMLEncoding['t'] = 5;
    adaptiveXMLEncoding['a'] = 6;
    adaptiveXMLEncoding['i'] = 7;
    adaptiveXMLEncoding['n'] = 8;
    adaptiveXMLEncoding['o'] = 9;
    adaptiveXMLEncoding['s'] = 10;
    adaptiveXMLEncoding['h'] = 11;
    adaptiveXMLEncoding['r'] = 12;
    adaptiveXMLEncoding['d'] = 13;
    adaptiveXMLEncoding['l'] = 14;
    adaptiveXMLEncoding[0xA] = 15;
    adaptiveXMLEncoding['.'] = 16;
    adaptiveXMLEncoding['u'] = 17;
    adaptiveXMLEncoding['w'] = 18;
    adaptiveXMLEncoding['c'] = 19;
    adaptiveXMLEncoding[0x27] = 20;
    adaptiveXMLEncoding['"'] = 21;
    adaptiveXMLEncoding[':'] = 22;

    adaptiveCEncoding[' '] = 0;
    adaptiveCEncoding['e'] = 1;
    adaptiveCEncoding['t'] = 2;
    adaptiveCEncoding['a'] = 3;
    adaptiveCEncoding['i'] = 4;
    adaptiveCEncoding['n'] = 5;
    adaptiveCEncoding['o'] = 6;
    adaptiveCEncoding['s'] = 7;
    adaptiveCEncoding['h'] = 8;
    adaptiveCEncoding['r'] = 9;
    adaptiveCEncoding['d'] = 10;
    adaptiveCEncoding['l'] = 11;
    adaptiveCEncoding['*'] = 12;
    adaptiveCEncoding['='] = 13;
    adaptiveCEncoding['\t'] = 14;
    adaptiveCEncoding[0xA] = 15;
    adaptiveCEncoding['c'] = 16;
    adaptiveCEncoding[';'] = 17;
    adaptiveCEncoding['f'] = 18;
    adaptiveCEncoding['('] = 19;
    adaptiveCEncoding[')'] = 20;
    adaptiveCEncoding[0x27] = 21;
    adaptiveCEncoding['/'] = 22;
}

static uint32_t initAdaptiveChars; // called once to init

static inline uint32_t setAdaptiveChars(const unsigned char *val256, unsigned char *outVals, const uint32_t nValues, const uint32_t **textEncodingArray)
{
    // if count of characters particular to XML or HTML or C code, set the 8 lowest frequency characers to common characters for that data type
    const uint32_t minCharCount=nValues < 24 ? 2 : 3; // minimum chars to choose an adaptive text set
    if (initAdaptiveChars == 0)
    {
        initAdaptiveTextMode();
        initAdaptiveChars = 1;
    }
    if (val256['<'] + val256['>'] + val256['/'] + val256['"'] +val256[':'] >= minCharCount)
    {
        // XML or HTML
        *textEncodingArray = adaptiveXMLEncoding;
        outVals[0] = 0x17; // indicate text mode with html/xml chars
        return 1;
    }
    else if (val256[','] + val256['='] + val256[';'] + val256['('] + val256[')'] + val256['/'] + val256['*'] >= minCharCount)
    {
        // C or similar
        *textEncodingArray = adaptiveCEncoding;
        outVals[0] = 0x27; // indicate text mode with C or similar chars
        return 2;
    }
    else
    {
        // standard text
        *textEncodingArray = extendedTextEncoding;
        outVals[0] = 0x7; // indicate standard text mode
        return 0;
    }
} // end setAdaptiveChars

int32_t encodeAdaptiveTextMode(const unsigned char *inVals, unsigned char *outVals, const uint32_t nValues, const unsigned char *val256, const uint32_t predefinedTextCharCnt, const uint32_t highBitclear, const uint32_t maxBytes)
{
    // Use these frequency-related bit encodings:
    // 101       value not in 23 text values, followed by 8-bit value
    // 011, 001  2 values of highest frequency
    // x111      2 values of next highest frequency
    // xxxx0     15 values of medium frequency, except 11110
    // xx11110   4 values of lowest frequency
    const unsigned char *pInVal=inVals;
    const unsigned char *pLastInValPlusOne=inVals+nValues;
    uint32_t inVal;
    uint32_t nextOutIx=1;
    uint32_t nextOutBit=0;
    uint32_t eVal;
    const uint32_t *textEncodingArray=extendedTextEncoding;
    const uint32_t output7or8=highBitclear ? 7 : 8;
    
    if (predefinedTextCharCnt)
        outVals[0] = 0x7; // default to standard text mode if predefined text char count is high enough
    else
        setAdaptiveChars(val256, outVals, nValues, &textEncodingArray);
    if (highBitclear)
        outVals[0] |= 128; // set high bit of info byte to indicate 7-bit values
    outVals[1] = 0; // init first value used by esmOutputBits
    while (pInVal < pLastInValPlusOne)
    {
        eVal=textEncodingArray[(inVal=(unsigned char)*(pInVal++))];
        if (eVal < MAX_PREDEFINED_FREQUENCY_CHAR_COUNT)
        {
            // encode predefined chars and adaptive chars
            esmOutputBits(outVals, textNBitsTable[eVal], textBitValTable[eVal], &nextOutIx, &nextOutBit);
        }
        else
        {
            // output char not predefined or adaptive
            if (nextOutIx > maxBytes)
                return 0; // requested compression not met
            esmOutputBits(outVals, 3, 0x5, &nextOutIx, &nextOutBit);
            esmOutputBits(outVals, output7or8, inVal, &nextOutIx, &nextOutBit); // output 7 bits if high bit clear, else 8
#ifdef TD64_TEST_MODE
            if (textEncodingArray == extendedTextEncoding)
                g_td64Text8bitCount++;
            else
                g_td64AdaptiveText8bitCount++;
#endif
        }
    }
    return nextOutIx * 8 + nextOutBit;
} // end encodeAdaptiveTextMode

int32_t encodeSingleValueMode(const unsigned char *inVals, unsigned char *outVals, const uint32_t nValues, int32_t singleValue, const uint32_t compressNSV)
{
    // generate control bit 1 if single value, otherwise 0 plus 8-bit value
    const unsigned char *pInVal=inVals;
    const unsigned char *pLastInValPlusOne=inVals+nValues;
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
    if (compressNSV)
    {
        uint32_t firstNonSingle=(nValues-1)/8+3; // skip single value itself
        uint32_t nNSV=nextOutVal-firstNonSingle+1;
        if (nNSV >= 16)
        {
            unsigned char outTemp[64];
            int32_t retBits;
            uint32_t nValuesOut;
#ifdef TD64_TEST_MODE
            g_td64CompressNSVcnt += nNSV;
            g_td64CompressNSVblocks++;
            g_td64nNSVcnt += nNSV;
#endif
            retBits = encodeExtendedStringMode(outVals+firstNonSingle, outTemp, nNSV, &nValuesOut);
            if (retBits < 0)
                return -28;
            if (retBits > (nNSV-2)*8 || retBits == 0)
            {
#ifdef TD64_TEST_MODE
                g_td64CompressNonSingleValues += nNSV*8; // not enough compression
                g_td64CompressNSVfailures++;
#endif
            }
            else
            {
                // non-string mode values compressed: set bit 4
                outVals[0] |= 8;
                outVals[firstNonSingle] = nNSV; // store number non-single values
                // copy compressed uniques in temp outvals to outvals
                uint32_t nBytes=retBits/8;
                if (retBits & 7)
                    nBytes++;
                // don't keep first byte that is 0x7f for extended string mode
                memcpy(outVals+firstNonSingle+1, outTemp+1, nBytes-1);
#ifdef TD64_TEST_MODE
                g_td64CompressNonSingleValues += retBits-8;
#endif
                return (firstNonSingle+1)*8 + retBits-8;
            }
        }
    }
    return (int32_t)nextOutVal * 8; // round up to full byte
} // end encodeSingleValueMode

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

#define STRING_LIMIT 9

int32_t encodeStringMode(const unsigned char *inVals, unsigned char *outVals, const uint32_t nValues, const uint32_t nUniquesIn, const uint32_t *uniqueOccurrence, const uint32_t highBitClear, const uint32_t maxBits)
{
    uint32_t twoValsPos[256]; // set to position+1 of first occurrence of value
    uint32_t nUniques; // first value is always a unique
    // if all inputs have high bit 0, compress the input values
    // set first bit of second value to 0 if not compressed, or 1 if compressed and followed by the compressed values
    // if not compressed, first bit of first output past uniques is high bit of first unique
    //uint32_t nextOutIx=nUniquesIn + 1; // start of encoding past uniques written from outer loop;
    uint32_t nextOutIx; // round for now
    uint32_t nextOutBit=1; // first bit indicates 1 or 2 uniques in first two input values
    const uint32_t maxBytes=maxBits/8; // compare against bytes out during loop
    
    if (nUniquesIn > 32 || nUniquesIn < MIN_STRING_MODE_UNIQUES)
        return -19;
    // use 7-bit encoding for uniques if all high bits set
    if (highBitClear)
    {
        uint32_t retBytes=encode7bitsInternal(outVals+1, outVals+1, nUniquesIn);
        nextOutIx = retBytes + 1;
        outVals[0] = 9 | (unsigned char)((nUniquesIn-17)<<4); // indicate string mode in first 3 bits, 1 for uniques compressed, then number original uniques - 17 (excess 16 as always 17+ values) in next 4 bits
    }
    else
    {
        nextOutIx=nUniquesIn+1;
        outVals[0] = 1 | (unsigned char)((nUniquesIn-17)<<4); // indicate string mode in first 3 bits, 0 for uniques uncompressed, then number uniques - 17 (excess 16 as always 17+ values) in next 4 bits
    }
    outVals[nextOutIx] = 0; // init for esmOutputBits
    // output two initial values
    // first unique assumed
    const unsigned char inVal0=inVals[0];
    if (inVal0 == inVals[1])
    {
        // first two values are the same
        nUniques = 1;
        // output 1 to indicate first unique value repeated
        outVals[nextOutIx] = 1; // 1=repeat for second value
        twoValsPos[inVal0] = 1;
    }
    else
    {
        // second val is a new unique
        nUniques = 2;
        // set up position of 2nd unique
        outVals[nextOutIx] = 0; // 0=uniques in first two values
        twoValsPos[inVal0] = 1;
        twoValsPos[inVals[1]] = 2;
    }
    uint32_t inPos=2; // start loop after init of first two values
    const uint32_t lastPos=nValues-1;
    uint32_t nextInVal = inVals[2];
    while (inPos < lastPos)
    {
        if (nextOutIx > maxBytes)
            return 0; // already failed compression minimum
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
            // don't take string past current input pos
            uint32_t strLimit=(inPos - tvPos < STRING_LIMIT) ? inPos - tvPos : STRING_LIMIT;
            if (nValues-strPos+2 < strLimit)
                strLimit = nValues - strPos + 2; // don't go past end of input
            tvPos++; // point to 2 past original unique
            uint32_t strCount=1;
            while(++strCount < strLimit && inVals[strPos] == inVals[tvPos])
            {
                strPos++;
                tvPos++;
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

int32_t td64(const unsigned char *inVals, unsigned char *outVals, const uint32_t nValues)
// td64: Compress nValues bytes. Return 0 if not compressible (no output bytes),
//    negative value if error; otherwise, number of bits written to outVals.
//    Management of whether compressible and number of input values must be maintained
//    by caller. Decdode requires number of input values and only accepts compressed data.
// Arguments:
//   inVals   input byte values
//   outVals  output byte values if compressed, max of inVals bytes
//   nValues  number of input byte values
// Returns number of bits compressed, 0 if not compressed, or negative value if error
{
    if (nValues <= 5)
        return td5(inVals, outVals, nValues);
    
    if (nValues > MAX_TD64_BYTES)
        return -1; // only values 1 to 64 supported
    
    uint32_t highBitCheck=0;
    uint32_t predefinedTextCharCnt=0; // count of text chars encountered
    uint32_t uniqueOccurrence[256]; // order of occurrence of uniques
    uint32_t nUniqueVals=0; // count of unique vals encountered
    unsigned char val256[256]={0}; // init characters found to 0
    const uint32_t uniqueLimit=uniqueLimits25[nValues]; // if exceeded, cannot use fixed bit coding

    // process enough input vals to eliminate most random data and to check for text mode
    // for fixed bit coding find and output the uniques starting at outVal[1]
    //    and saving the unique occurrence value to be used when values are output
    // for 7 bit mode OR every value
    // for text mode count number of predfined text characters
    // for single value mode accumulate frequency counts
    const uint32_t nValsInitLoop=nValues<24 ? nValues/2 : nValues*7/16; // 1-23 use 1/2 nValues, 24+ use 7/16 nValues; fewer values means faster execution but possibly lower compression
    uint32_t inPos=0;
    while (inPos < nValsInitLoop)
    {
        const uint32_t inVal=inVals[inPos++];
        predefinedTextCharCnt += predefinedBitTextChars[inVal]; // count text chars for text char mode
        if (val256[inVal]++ == 0)
        {
            // first occurrence of value, for fixed bit coding:
            uniqueOccurrence[inVal] = nUniqueVals; // save occurrence count for this unique
            outVals[++nUniqueVals] = (unsigned char)inVal; // store unique starting at second byte
            highBitCheck |= inVal; // keep watch on high bit of unique values
        }
    }
    if (nUniqueVals > nValsInitLoop - nValsInitLoop/8 - 1 && (highBitCheck & 0x80))
    {
        // unique values exceed usual count to be compressed and high bit across tested values is not 0
        outVals[0] = 0; // indicate random data failure in first check
        return 0;
    }
    if (nUniqueVals > uniqueLimit/2 && predefinedTextCharCnt > nValsInitLoop/2)
    {
        // encode in text mode if at least 11% compression expected
#ifdef TD64_TEST_MODE
        uint32_t save8bitCount=g_td64Text8bitCount;
        uint32_t saveAdaptive8bitCount=g_td64AdaptiveText8bitCount;
#endif
        const uint32_t useExtendedTextMode=predefinedTextCharCnt >= nValsInitLoop*7/8; // use extended text mode rather than adaptive text mode
        uint32_t highBitClear=0;
        if ((highBitCheck & 0x80) == 0)
        {
            // original values encoded with 7 bits if high bit is clear for all else 8 bits
            uint32_t inPos2=inPos;
            while (inPos2 < nValues)
                highBitCheck |= inVals[inPos2++]; // check for remaining values with high bit clear
            if ((highBitCheck & 0x80) == 0)
                highBitClear = 1;
        }
        // save uniques in outVals to recover on failure
        unsigned char saveUniques[MAX_TD64_BYTES];
        memcpy(saveUniques, outVals+1, nUniqueVals);
        int32_t retBits=encodeAdaptiveTextMode(inVals, outVals, nValues, val256, useExtendedTextMode, highBitClear, nValues-nValues/8);
        if (retBits != 0)
            return retBits;
        memcpy(outVals+1, saveUniques, nUniqueVals);
#ifdef TD64_TEST_MODE
        g_td64Text8bitCount = save8bitCount; // reset count before failure
        g_td64AdaptiveText8bitCount = saveAdaptive8bitCount; // reset
        g_td64FailedTextMode++;
#endif
    }
    // continue fixed bit loop with check for single value
    // perform this even when uniqueLimit is exceeded to do single value mode and string mode
    const uint32_t minRepeatsSingleValueMode=nValues<16 ? nValues/2 : nValues/4+2;
    int32_t singleValue=-1; // set to value if min repeats reached
    while (inPos < nValues)
    {
        // will always complete this loop unless single value count reached
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
    if (singleValue >= 0 && nUniqueVals > uniqueLimit)
    {
        // early opportunity for single value mode
        // single value mode is fast and set to get minimum 12% compression for 64 values
        // single value mode is not limited by MAX_STRING_MODE_UNIQUES
        const uint32_t compressNSV=0; // don't compress non-single values when unique limit exceeded
        return encodeSingleValueMode(inVals, outVals, nValues, singleValue, compressNSV);
    }
    if (nUniqueVals <= uniqueLimit)
    {
        // continue fixed bit loop with checks for high bit set and repeat counts, but without single value
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
            // always choose single value mode first
            const uint32_t compressNSV=0; // don't compress non-single values when unique limit exceeded
            return encodeSingleValueMode(inVals, outVals, nValues, singleValue, compressNSV);
        }
        const uint32_t nUniquesRandom=nValues*3/4 < MAX_STRING_MODE_UNIQUES ? nValues*3/4 : MAX_STRING_MODE_UNIQUES;
        const uint32_t checkHighBit=(highBitCheck & 0x80) == 0 && nValues >= MIN_VALUES_7_BIT_MODE;
        if (nUniqueVals > nUniquesRandom)
        {
            if (checkHighBit)
            {
                // compress based on high bit clear across all values
                // with this quantity of uniques, this mode should offer the best compression
                return encode7bits(inVals, outVals, nValues);
            }
            outVals[0] = 2; // indicate random data failure in second check
            return 0; // compression failed
        }
        if ((nValues >= MIN_VALUES_STRING_MODE))
        {
            // string mode for 32+ values with 17 to 32 uniques and fewer than MAX_STRING_MODE_UNIQUES
            // max bits set to 12% if high bit clear and enough input values, else 2%
            const uint32_t maxBits=checkHighBit ? nValues*8-nValues : nValues*8-nValues/4-1;
            int32_t retBits;
#ifdef TD64_TEST_MODE
            g_td64StringBlocks++;
#endif
            if (nUniqueVals >= MIN_STRING_MODE_UNIQUES)
            {
                if ((retBits=encodeStringMode(inVals, outVals, nValues, nUniqueVals, uniqueOccurrence, checkHighBit, maxBits)) != 0)
                return retBits;
            }
            else
            {
                uint32_t nValuesOut;
                if ((retBits=encodeExtendedStringMode(inVals, outVals, nValues, &nValuesOut)) < 0)
                    return retBits;
                if (retBits >= maxBits)
                    return retBits;
            }
#ifdef TD64_TEST_MODE
            g_td64FailedStringMode++;
#endif
        }
        if (checkHighBit)
        {
            // compress in high bit mode
            return encode7bits(inVals, outVals, nValues);
        }
        outVals[0] = 1; // indicate general failure to compress
        return 0; // unable to compress
    }
    else if (nUniqueVals > 4 && singleValue >= 0)
    {
        // favor single value over fixed 3- and 4-bit encoding
        const uint32_t compressNSV=1; // for small numbers of uniques, try to compress non-single values
        return encodeSingleValueMode(inVals, outVals, nValues, singleValue, compressNSV);
    }
    // process fixed bit coding inline
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

static uint32_t dtbmThisInVal; // current input value

static inline void dtbmPeekBits(const unsigned char *inVals, const uint32_t nBitsToPeak, uint32_t thisInValIx, uint32_t bitPos, uint32_t *theBits)
{
    *theBits = dtbmThisInVal >> bitPos;
    if ((bitPos += nBitsToPeak) < 8)
    {
        // all bits in current value with bits to spare
        *theBits &= 0xff >> (8-nBitsToPeak);
        return;
    }
    // next input: or in 0 to 7 bits
    bitPos -= 8;
    // when 0 bits, shifted past bits to get then anded off
    *theBits |= inVals[++thisInValIx] << (nBitsToPeak-bitPos);
    *theBits &= 0xff >> (8-nBitsToPeak);
} // end dtbmPeekBits

static inline void dtbmSkipBits(const unsigned char *inVals, const uint32_t nBitsToSkip, uint32_t *thisInValIx, uint32_t *bitPos)
{
    if ((*bitPos += nBitsToSkip) < 8)
    {
        // all bits in current value with bits to spare
        return;
    }
    // next input: or in 0 to 7 bits
    *bitPos -= 8;
    // when 0 bits, shifted past bits to get then anded off
    dtbmThisInVal = inVals[++(*thisInValIx)];
} // end dtbmSkipBits

static inline void dtbmGetBits(const unsigned char *inVals, const uint32_t nBitsToGet, uint32_t *thisInValIx, uint32_t *bitPos, uint32_t *theBits)
{
    // get 1 to 8 bits from inVals into theBits
    *theBits = dtbmThisInVal >> (*bitPos);
    if ((*bitPos += nBitsToGet) < 8)
    {
        // all bits in current value with bits to spare
        *theBits &= 0xff >> (8-nBitsToGet);
        return;
    }
    // next input: or in 0 to 7 bits
    dtbmThisInVal = inVals[++(*thisInValIx)];
    *bitPos -= 8;
    // when 0 bits, shifted past bits to get then anded off
    *theBits |= dtbmThisInVal << (nBitsToGet-*bitPos);
    *theBits &= 0xff >> (8-nBitsToGet);
} // end dtbmGetBits

static const uint32_t textDecodePos[128]={
    4,0,5,1,6,0,7,2,8,0,9,1,10,0,11,3,12,
    0,13,1,14,0,15,2,16,0,17,1,18,0,19,3,4,
    0,5,1,6,0,7,2,8,0,9,1,10,0,11,3,12,
    0,13,1,14,0,15,2,16,0,17,1,18,0,20,3,4,
    0,5,1,6,0,7,2,8,0,9,1,10,0,11,3,12,
    0,13,1,14,0,15,2,16,0,17,1,18,0,21,3,4,
    0,5,1,6,0,7,2,8,0,9,1,10,0,11,3,12,
    0,13,1,14,0,15,2,16,0,17,1,18,0,22,3
};
    
static const uint32_t textDecodeBits[128]={
    5,3,5,3,5,0,5,4,5,3,5,3,5,3,5,4,5,
    3,5,3,5,3,5,4,5,3,5,3,5,3,7,4,5,
    3,5,3,5,3,5,4,5,3,5,3,5,3,5,4,5,
    3,5,3,5,3,5,4,5,3,5,3,5,3,7,4,5,
    3,5,3,5,3,5,4,5,3,5,3,5,3,5,4,5,
    3,5,3,5,3,5,4,5,3,5,3,5,3,7,4,5,
    3,5,3,5,3,5,4,5,3,5,3,5,3,5,4,5,
    3,5,3,5,3,5,4,5,3,5,3,5,3,7,4
};

int32_t decodeAdaptiveTextMode(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed)
{
    // Use these frequency-related bit encodings:
    // 101       value not in 23 text values, followed by 8-bit value
    // 011, 001  2 values of highest frequency
    // x111      2 values of next highest frequency
    // xxxx0     15 values of medium frequency, except 11110
    // xx11110   4 values of lowest frequency
    uint32_t nextOutVal=0;
    uint32_t thisInValIx=1; // start past first info byte
    uint32_t bitPos=0;
    uint32_t theBits;
    const uint32_t *pTextChars; // points to text chars encoded with
    const uint32_t input7or8=(inVals[0] & 0x80) ? 7 : 8; // high bit of info bit indicates whether unreplaced values output as 7 or 8 bits
    
    if ((inVals[0] & 0x3f) == 0x17)
        pTextChars = XMLTextChars;
    else if ((inVals[0] & 0x3f) == 0x27)
        pTextChars = CTextChars;
    else
        pTextChars=extendedTextChars;
    dtbmThisInVal = inVals[thisInValIx]; // initialize to first input val to decode
    while (nextOutVal < nOriginalValues)
    {
        // peak at the next 7 bits to decide what to do
        dtbmPeekBits(inVals, 7, thisInValIx, bitPos, &theBits);
        if ((theBits & 7) == 5)
        {
            // skip three bits, get 7 or 8 more bits and output original value
            dtbmSkipBits(inVals, 3, &thisInValIx, &bitPos);
            dtbmGetBits(inVals, input7or8, &thisInValIx, &bitPos, &theBits);
            outVals[nextOutVal++] = (unsigned char)theBits;
        }
        else
        {
            // output the corresponding text char and skip corresponding number of bits
            outVals[nextOutVal++] = (unsigned char)pTextChars[textDecodePos[theBits]];
            dtbmSkipBits(inVals, textDecodeBits[theBits], &thisInValIx, &bitPos);
        }
    }
    *bytesProcessed = thisInValIx + (bitPos > 0);
    return nextOutVal;
} // end decodeAdaptiveTextMode

int32_t decodeSingleValueMode(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed)
{
    uint32_t nextInVal=(nOriginalValues-1)/8+2;
    uint32_t nextOutVal=0;
    uint64_t controlByte=0;
    uint64_t controlBit=1;
    unsigned char singleValue;
    const unsigned char *pNSVs;
    unsigned char uncompressedNSVs[MAX_TD64_BYTES];
    uint32_t compressedBytesProcessed=0; // when NSVs compressed use for bytesProcessed return value

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
    if (inVals[0] & 8)
    {
        // non-single values were compressed, byte at nextInVal is number NSVs
        uint32_t bytesProcessed;
        uint32_t nNSVs=inVals[nextInVal];
        // first byte in encoded extended string mode is 0x7f, which is not stored; value at nextInVal is not referenced
        if ((decodeExtendedStringMode(inVals+nextInVal, uncompressedNSVs, nNSVs, &bytesProcessed)) <= 0)
            return -27;
        pNSVs = uncompressedNSVs;
        compressedBytesProcessed = nextInVal + bytesProcessed; // byte for nNSVs replaces first byte of encoded data
        nextInVal = 0; // point to NSVs in local array
    }
    else
    {
        // NSVs are not compressed, point at input encoding
        pNSVs = inVals;
    }

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
            outVals[nextOutVal++] = pNSVs[nextInVal++];
        }
        controlBit <<= 1;
    }
    if (inVals[0] & 8)
        *bytesProcessed = compressedBytesProcessed;
    else
        *bytesProcessed = nextInVal;
    return (int32_t)nOriginalValues;
} // end decodeSingleValueMode

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

static uint32_t dsmThisVal;
static inline void dsmGetBits(const unsigned char *inVals, const uint32_t nBitsToGet, uint32_t *thisInValIx, uint32_t *bitPos, int32_t *theBits)
{
    // get 1 to 9 bits from inVals into theBits
    *theBits = dsmThisVal >> *bitPos;
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
        dsmThisVal = inVals[++(*thisInValIx)];
        return;
    }
    else if (*bitPos == 16)
    {
        *theBits |= (inVals[++(*thisInValIx)]) << 1; // must be 9 bits
        dsmThisVal = inVals[++(*thisInValIx)];
        *bitPos = 0;
        return;
    }
    // bits split between two values with bits left (bitPos > 0)
    *bitPos -= 8;
    dsmThisVal = inVals[++(*thisInValIx)];
    *theBits |= dsmThisVal << (nBitsToGet-*bitPos);
    *theBits &= bitMask[nBitsToGet];
} // end dsmGetBits

int32_t decodeStringMode(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed)
{
    uint32_t nextOutVal;
    uint32_t thisInValIx;
    uint32_t bitPos;
    int32_t theBits;
    uint32_t nUniques;
    uint32_t uPos[MAX_STRING_MODE_UNIQUES];
    const unsigned char *pUniques;
    unsigned char uncompressedUniques[MAX_TD64_BYTES];
    uint32_t firstByte=inVals[0];
    
    // process one of three encodings:
    // 0   new unique value
    // 01  repeated value by original unique count
    // 11  two or more values of a repeated string with string count and position of first occurrence
    if ((firstByte & 8) == 0)
    {
        // uniques are not compressed, point at input encoding
        pUniques = inVals+1;
        thisInValIx = (unsigned char)(firstByte >> 4) + 18; // number uniques, excess 16, plus 1 for first byte
    }
    else
    {
        // uniques are compressed, point at input vals
        pUniques = uncompressedUniques;
        uint32_t ret7bits;
        if ((ret7bits=decode7bitsInternal(inVals+1, uncompressedUniques, (firstByte >> 4) + 17, &thisInValIx) < 1))
            return ret7bits;
        thisInValIx++; // point past initial byte for encoded bytes
    }
    // first value is always the first unique
    outVals[0] = pUniques[0]; // first val is always first unique
    uPos[0] = 0; // unique position for first unique
    if (inVals[thisInValIx] & 1)
    {
        // second value matches first
        outVals[1] = outVals[0];
        bitPos = 1;
        nUniques = 1;
    }
    else
    {
        // second value is a unique
        outVals[1] = pUniques[1];
        bitPos = 1;
        nUniques = 2;
        uPos[1] = 1; // unique position for second unique
    }
    nextOutVal=2; // start with third output value
    const uint32_t nOrigMinus1=nOriginalValues-1;
    dsmThisVal = inVals[thisInValIx]; // init to first input val to decode
    while (nextOutVal < nOrigMinus1)
    {
        if ((dsmThisVal & (1 << bitPos)) == 0)
        {
            // new unique value
            uPos[nUniques] = nextOutVal; // unique position for first occurrence of unique
            if (bitPos == 7)
            {
                outVals[nextOutVal++] = pUniques[nUniques];
                dsmThisVal = inVals[++thisInValIx];
                bitPos = 0;
            }
            else
            {
                outVals[nextOutVal++] = pUniques[nUniques];
                bitPos++;
            }
            nUniques++;
        }
        else
        {
            // repeat or string
            if (++bitPos == 8)
            {
                dsmThisVal = inVals[++thisInValIx];
                bitPos = 0;
            }
            if ((dsmThisVal & (1 << (bitPos))) == 0)
            {
                // repeat: next number of bits, determined by nUniques, indicates repeated value
                if (++bitPos == 8)
                {
                    dsmThisVal = inVals[++thisInValIx];
                    bitPos = 0;
                }
                const uint32_t nUniqueBits = encodingBits[nUniques-1]; // current number uniques determines 1-5 bits used
                dsmGetBits(inVals, nUniqueBits, &thisInValIx, &bitPos, &theBits);
                outVals[nextOutVal++] = pUniques[theBits]; // get uniques from start of inVals
            }
            else
            {
                // string: 11 plus string length of 2 to 9 in 3 bits
                if (++bitPos == 8)
                {
                    dsmThisVal = inVals[++thisInValIx];
                    bitPos = 0;
                }
                // multi-character string: location of values in bits needed to code current pos
/*                uint32_t nPosBits = encodingBits[nextOutVal];*/
                const uint32_t nPosBits = encodingBits[nUniques-1];
                dsmGetBits(inVals, 3+nPosBits, &thisInValIx, &bitPos, &theBits);
                const uint32_t stringLen = (uint32_t)(theBits & 7) + 2;
                assert(stringLen <= STRING_LIMIT);
                const uint32_t stringPos=uPos[(uint32_t)theBits >> 3];
                if (stringPos+stringLen > nextOutVal)
                    return -41;
                assert((uint32_t)stringPos+stringLen <= nextOutVal);
                assert(nextOutVal+stringLen <= nOriginalValues);
                memcpy(outVals+nextOutVal, outVals+stringPos, stringLen);
                nextOutVal += stringLen;
            }
        }
    }
    if (bitPos > 0)
        thisInValIx++; // inc past partial input value
    if (nextOutVal == nOrigMinus1)
    {
        // output last byte in input when not ending with a string
        // string at end will catch last byte
        outVals[nOrigMinus1] = inVals[thisInValIx++];
    }
    *bytesProcessed = thisInValIx;
    return (int32_t)nOriginalValues;
} // end decodeStringMode

int32_t td64d(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed)
// decoding requires number of original values and encoded bytes
// uncompressed data is not acceppted
// encoding for 1 to 64 input values.
// 1 to 5 input values are handled separately.
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
    if (firstByte == 0x7f)
    {
        // string mode extended
        return decodeExtendedStringMode(inVals, outVals, nOriginalValues, bytesProcessed);
    }
    if ((firstByte & 7) == 0x01)
    {
        // string mode
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
        return decodeAdaptiveTextMode(inVals, outVals, nOriginalValues, bytesProcessed);
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
