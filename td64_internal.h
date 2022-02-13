//
//  td64_internal.h
//  td512
//
//  Created by L. Stevan Leonard on 12/9/21.
//  Copyright © 2021-2022 L. Stevan Leonard. All rights reserved.
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
//

#ifndef td64_internal_h
#define td64_internal_h

#define MIN_STRING_MODE_EXTENDED_VALUES 16

static const uint32_t encodingBits[64]={1,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6};
static const uint32_t bitMask[]={0,1,3,7,15,31,63,127,255,511};

static int32_t encode7bitsInternal(const unsigned char *inVals, unsigned char *outVals, const uint32_t nValues)
{
    // for internal use: output 7 bytes for each 8-byte group, then remaining bytes
    uint32_t nextOutVal=0;
    uint32_t nextInVal=0;
    uint32_t val1;
    uint32_t val2;
    
    if (nValues < 8)
        return 0;
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
    return (int32_t)nextOutVal;
} // end encode7bitsInternal

static int32_t decode7bitsInternal(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed)
{
    // decode values directly into outVals
    uint32_t nextOutVal=0;
    uint32_t nextInVal=0;
    uint32_t val1;
    uint32_t val2;
    
    if (nOriginalValues < 8)
        return -33; // must have at least one group of 8 bytes to compress
    
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
} // end decode7bitsInternal

#endif /* td64_internal_h */
