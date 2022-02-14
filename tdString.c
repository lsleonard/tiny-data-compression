//
//  tdString.c
//  Encode and decode up to MAX_STRING_MODE_EXTENDED_VALUES input values.
//  If the 65th unique value is encountered, output it and return the
//  number of values processed.
//
//  Created by L. Stevan Leonard on 12/8/21.
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

#include "tdString.h"
#include "td64_internal.h"

#define MAX_STRING_MODE_EXTENDED_VALUES 512

int32_t encodeExtendedStringMode(const unsigned char *inVals, unsigned char *outVals, const uint32_t nValuesMax, uint32_t *nValuesOut)
{
    // Encode repeated strings and values in input until the 65th unique value,
    // then conclude processing and return the number of values.
    // If the number of encoded values exceeds the number of input values,
    // return 0.
    uint32_t inPos; // current position in inVals
    uint32_t inVal;
    uint32_t nUniques; // first value is always a unique
    uint8_t val256[256]={0};
    uint32_t uniqueOccurrence[256]; // set to the count of the first occurrence of that value
    uint64_t twoVals[64]; // index is first unique val, with bit position of second unique value set to 1
    uint32_t twoValsPoss[64*64]; // position in input of first occurrence of corresponding two unique values of up to 64
    uint32_t twoValsPos;
    uint32_t thisOutIx;
    uint32_t nextOutBit=1; // start of encoding after first two inputs
    unsigned char outValsT[MAX_STRING_MODE_EXTENDED_VALUES];
    uint32_t maxUniquesExceeded=0;
    uint32_t highBitClear;
    uint64_t outBits; // accumulate 64 bits before output
    uint32_t nUniqueBitsPlus2=3; // bits to encode current number of uniques (1 or 2) plus 2 control bits
    uint32_t nextInVal=inVals[2];
    
    if (nValuesMax > MAX_STRING_MODE_EXTENDED_VALUES || nValuesMax < MIN_STRING_MODE_EXTENDED_VALUES)
        return -100;
    outVals[1] = 0; // init second info byte
    thisOutIx = 0; // start of encoding in outValsT
    // output encoding of first two values in outVals starting at third bit in second byte
    //    first bit is last bit of unique count, second is whether
    //    uniques are compressed
    inVal=inVals[0];
    highBitClear = inVal;
    outVals[2] = inVal;
    if (inVal == inVals[1])
    {
        // first two values are the same
        nUniques = 1;
        val256[inVal] = 1; // indicate encountered
        uniqueOccurrence[inVal] = 0;
        twoVals[0] = 1;  // 1 << next unique val
        twoValsPoss[0] = 2; // set position (0,0): unique << 6 | next unique, to pos first unique + 2
        if (inVal != inVals[2])
        {
            // set up for new unique in third position
            twoVals[0] |= 2; // (0,1)
            twoValsPoss[1] = 3; // set position to two past second value
        }
        // output 1 to indicate first unique value repeated
        outBits = 1; // 1 for first encoding bit
    }
    else
    {
        // second val is a new unique
        // set twoValsPos for first position (0,1)
        nUniques = 2;
        val256[inVal] = 1;
        uniqueOccurrence[inVal] = 0;
        inVal = inVals[1]; // inVal is now second value
        highBitClear |= inVal;
        outVals[3] = inVal;
        val256[inVal] = 1;
        uniqueOccurrence[inVal] = 1;
        twoVals[0] = 2; // 1 << next unique val
        twoValsPoss[1] = 2; // set position (0,1): unique << 6 | next unique, to pos first unique + 2
        inVal = inVals[2]; // inval is now value in third position
        uint32_t UOinPos2=uniqueOccurrence[inVal];
        if (val256[inVal] == 0)
        {
            // new unique in third position
            UOinPos2 = 2;
        }
        // set up new two value in 2nd position
        twoVals[1] = 1 << UOinPos2;
        twoValsPoss[64 | UOinPos2] = 3; // set position to two past second value
        outBits = 0; // for first encoding bit
    }
    // smaller values compress slightly better with string limit of 9 versus 17
    const uint32_t string_limit=nValuesMax<=64 ? 9 : 17;
    const uint32_t extended_string_length_bits=nValuesMax<=64 ? 3 : 4;
    const uint32_t stringBits=2+extended_string_length_bits;
    const uint32_t stringEndPos=nValuesMax-string_limit+3; // used with inPos+4 to end one before last value
    const uint32_t maxCompressedPos=nValuesMax-nValuesMax/16;
    const uint32_t lastPos=nValuesMax-1;
    
    inPos=2; // start loop after init of first two values
    while (inPos < lastPos)
    {
        inVal = nextInVal; // set this val already retrieved value
        nextInVal = inVals[++inPos]; // inPos inc'd to next position
        if (val256[inVal] == 0)
        {
            // set up for new unique in this position
            // uniques > 64 are output as uniques but are not considered for processing
            if (thisOutIx+nUniques > maxCompressedPos)
            {
                // getting less than 6% compression: fail
                *nValuesOut = inPos - 1; // processed through last inPos
                return 0;
            }
            if (nUniques < MAX_UNIQUES_EXTENDED_STRING_MODE)
            {
                uint32_t UOinValsInPosP1;
                uniqueOccurrence[inVal] = nUniques;
                val256[inVal] = 1;
                nUniqueBitsPlus2 = encodingBits512[nUniques]+2;
                if (val256[nextInVal] == 0)
                {
                    if (nUniques < MAX_UNIQUES_EXTENDED_STRING_MODE-1)
                    {
                        // set up for all except last unique
                        UOinValsInPosP1 = nUniques + 1; // use but don't set up
                        twoVals[nUniques] = 1llu << UOinValsInPosP1;
                        // set position of these two vals one past second value
                        twoValsPoss[(nUniques<<6) | UOinValsInPosP1] = inPos + 1;
                    }
                    else
                    {
                        twoVals[nUniques] = 0; // init without assigning a next value for last unique
                    }
                }
                else
                {
                    // set up pair of this value and next
                    UOinValsInPosP1 = uniqueOccurrence[nextInVal];
                    twoVals[nUniques] = 1llu << UOinValsInPosP1;
                    twoValsPoss[(nUniques<<6) | UOinValsInPosP1] = inPos + 1;
                }
            }
            else if (nUniques == 128)
            {
                maxUniquesExceeded = inPos; // 129th unique to be output
                break;
            }
            nUniques++;
            highBitClear |= inVal;
            // output a 0 to indicate new unique
            if (++nextOutBit == 64)
            {
                // output outBits and init for next output
                esmOutputOutBits(outValsT, &thisOutIx, &outBits);
                outBits = 0;
                nextOutBit = 0;
            }
            outVals[nUniques+1] = (unsigned char)inVal; // save unique or any value encountered beyond 64 uniques in list at front of outVals starting in third position
            continue;
        }
        // this character occurred before: look for repetition of next character
        const uint32_t UOinVal=uniqueOccurrence[inVal];
        if (val256[nextInVal] == 0)
        {
            // new unique in next pos so output a repeated value for current position
            if (nUniques < MAX_UNIQUES_EXTENDED_STRING_MODE)
            {
                // set up pair with this unique and next value
                const uint32_t UOinValsInPosP1 = nUniques; // will be set to next unique on next loop
                twoVals[UOinVal] |= 1llu << UOinValsInPosP1;
                twoValsPoss[(UOinVal<<6) | UOinValsInPosP1] = inPos + 1;
            }
            // output repeated value: 01 plus unique occurrence
            thisOutIx2(outValsT, nUniqueBitsPlus2, (uint64_t)(1|(UOinVal<<2)), &thisOutIx, &nextOutBit, &outBits);
            continue;
        }
        const uint64_t TVuniqueOccurrence=twoVals[UOinVal];
        const uint32_t UOinValsInPosP1 = uniqueOccurrence[nextInVal];
        if (TVuniqueOccurrence & (1llu << UOinValsInPosP1))
        {
            // found pair of matching values using next input value
            // look for continuation of matching characters
            twoValsPos = twoValsPoss[(UOinVal<<6) | UOinValsInPosP1];
            // !!! check for end of string could be avoided by stopping loop 3 earlier
            if (twoValsPos+2 >= inPos || nValuesMax-inPos < 4)
            {
                // two vals + 2 include first value so overlap with second to fourth values; only check once for strings from 2 to 4
                // output repeated value: 01 plus unique occurrence
                thisOutIx2(outValsT, nUniqueBitsPlus2, (uint64_t)(1|(UOinVal<<2)), &thisOutIx, &nextOutBit, &outBits);
                continue;
            }
            const unsigned char *matchPos=inVals+twoValsPos;
            if (inVals[++inPos] != *matchPos++) // three-character match?
            {
                // no, output two-character string
                // output 11 plus string length bit then position of string
                thisOutIx2(outValsT, stringBits+encodingBits512[inPos-2], (3 | ((twoValsPos-2)<<stringBits)), &thisOutIx, &nextOutBit, &outBits);
                nextInVal = inVals[inPos];
                continue;
            }
            if (inVals[++inPos] != *(matchPos++)) // four-character match?
            {
                // no, output three-character string
                // output 11 plus string length bit then position of string
                thisOutIx2(outValsT, stringBits+encodingBits512[inPos-3], (7 | ((twoValsPos-2)<<stringBits)), &thisOutIx, &nextOutBit, &outBits);
                nextInVal = inVals[inPos];
                continue;
            }
            inPos++; // inPos now pointing at 5th char relative to start inPos
            // check for 5+ character string
            uint32_t strLimit=string_limit; // limit based on extended_string_length_bits
            if (inPos > stringEndPos)
                strLimit = lastPos - inPos + 4; // don't go past end of input
            if (inPos-twoValsPos-2 < strLimit)
                strLimit = inPos - twoValsPos - 2; // don't take string past current input pos
            uint32_t strCount=4;
            while(++strCount < strLimit && inVals[inPos] == *matchPos)
            {
                inPos++;
                matchPos++;
            }
            assert(inPos<=nValuesMax);
            assert(twoValsPos+strCount<=inPos-1);
            // output 11 plus string length bit then position of string
            // strCount is 1 greater than its actual length and the output length is 1 less than actual length
            thisOutIx2(outValsT, stringBits+encodingBits512[inPos+1-strCount], (3 | ((strCount-3)<<2)) | ((twoValsPos-2)<<stringBits), &thisOutIx, &nextOutBit, &outBits);
            nextInVal = inVals[inPos];
        }
        else
        {
            // repeated value: 01
            // save new pair of this value and next
            twoVals[UOinVal] |= 1llu << UOinValsInPosP1;
            twoValsPoss[(UOinVal<<6) | UOinValsInPosP1] = inPos + 1;
            // output repeated value: 01 plus unique occurrence
            thisOutIx2(outValsT, nUniqueBitsPlus2, (uint64_t)(1|(UOinVal<<2)), &thisOutIx, &nextOutBit, &outBits);
        }
    }
    // output final bits
    if (inPos < nValuesMax)
    {
        // occurs for both end of input on last pos -1 and for max uniques exceeded
        if (maxUniquesExceeded)
            thisOutIx2(outValsT, 8, (uint64_t)inVals[maxUniquesExceeded-1], &thisOutIx, &nextOutBit, &outBits);
        else
            thisOutIx2(outValsT, 8, (uint64_t)inVals[lastPos], &thisOutIx, &nextOutBit, &outBits);
    }
    if (nextOutBit > 0)
    {
        esmOutputRemainder(outValsT, &thisOutIx, &nextOutBit, &outBits); // index past final bits
    }
    *nValuesOut = maxUniquesExceeded ? maxUniquesExceeded : nValuesMax;
    if (thisOutIx + nUniques > *nValuesOut - 1)
        return 0;
    // use 7-bit encoding on uniques if all high bits set
    int32_t uniqueOffset;
    if ((highBitClear & 0x80) == 0 && *nValuesOut >= 16)
    {
        unsigned char compressedUniques[MAX_TOTAL_UNIQUES_EXTENDED_STRING_MODE];
        uniqueOffset = encode7bitsInternal(outVals+2, compressedUniques, nUniques);
        if (uniqueOffset > 0)
        {
            outVals[1] |= 128;
            memcpy(outVals+2, compressedUniques, uniqueOffset);
            uniqueOffset += 2;
        }
        else
        {
            // uniques did not compress
            uniqueOffset = nUniques + 2;
        }
    }
    else
    {
        uniqueOffset = nUniques + 2;
    }
    memcpy(outVals+uniqueOffset, outValsT, thisOutIx);
    outVals[0] = 0x7f; // indicate external string mode
    outVals[1] |= nUniques-1; // number uniques in first 7 bits then compressed uniques bit
    return (int32_t)(thisOutIx+uniqueOffset) * 8;
} // end encodeExtendedStringMode

static inline void dsmGetBits(const unsigned char *inVals, const uint32_t nBitsToGet, uint32_t *thisInVal, uint32_t *thisVal, uint32_t *bitPos, int32_t *theBits)
{
    // get 1 to 8 bits from inVals into theBits
    *theBits = (int32_t)*thisVal >> (*bitPos);
    if ((*bitPos += nBitsToGet) < 8)
    {
        // all bits in current value with bits to spare
        *theBits &= bitMask[nBitsToGet];
        return;
    }
    // bits split between two values with bits left (bitPos > 0)
    *bitPos -= 8;
    *thisVal = inVals[++(*thisInVal)];
    *theBits |= (*thisVal) << (nBitsToGet-*bitPos);
    *theBits &= bitMask[nBitsToGet];
    return;
} // end dsmGetBits

static inline void dsmGetBits2(const unsigned char *inVals, const uint32_t nBitsToGet, uint32_t *thisInVal, uint32_t *thisVal, uint32_t *bitPos, int32_t *theBits)
{
    // get 1 to 9 bits from inVals into theBits
    *theBits = (int32_t)*thisVal >> (*bitPos);
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
        *thisVal = inVals[++(*thisInVal)];
        return;
    }
    if (*bitPos == 16)
    {
        *theBits |= (inVals[++(*thisInVal)]) << 1; // must be 9 bits
        *thisVal = inVals[++(*thisInVal)];
        *bitPos = 0;
        return;
    }
    // bits split between two values with bits left (bitPos > 0)
    *bitPos -= 8;
    *thisVal = inVals[++(*thisInVal)];
    *theBits |= (*thisVal) << (nBitsToGet-*bitPos);
    *theBits &= bitMask[nBitsToGet];
    return;
} // end dsmGetBits2

int32_t decodeExtendedStringMode(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed)
{
    uint32_t nextOutVal;
    uint32_t thisInVal; // position of encoded bits
    uint32_t bitPos=1; // start decoding from bit 0, but first bit 0 used to indicate whether first unique is repeated or not
    int32_t theBits;
    uint32_t nUniques; // running number of uniques
    const unsigned char *pUniques; // pointer to uniques
    unsigned char uncompressedUniques[MAX_TOTAL_UNIQUES_EXTENDED_STRING_MODE];
    uint32_t secondByte=inVals[1]; // bits= 0:6 nUniques 7 compressed or not
    uint32_t nUniquesIn=(secondByte & 0x7f) + 1;
    // smaller values compress slightly better with string limit of 9 versus 17
    const uint32_t extended_string_length_bits=nOriginalValues<=64 ? 3 : 4;

    // process one of three encodings:
    // 0   new unique value
    // 01  repeated value by original unique count
    // 11  two or more values of a repeated string with string count and position of first occurrence
    // number uniques from first byte bits 3-7 plus first bit of first encoding byte
    // second bit of first encoding byte reserved for indicating whether uniques are compressed or not
    
    if ((secondByte & 128) == 0)
    {
        // uniques are not compressed, point at input encoding
        pUniques = inVals+2;
        thisInVal = nUniquesIn + 2;
    }
    else
    {
        // uniques are compressed, point at input vals
        pUniques = uncompressedUniques;
        uint32_t ret7bits;
        if ((ret7bits=decode7bitsInternal(inVals+2, uncompressedUniques, nUniquesIn, &thisInVal)) < 1)
            return -21;
        if (ret7bits != nUniquesIn)
            return -21;
        thisInVal += 2; // point past initial byte for encoded bytes
    }
    uint32_t thisVal = inVals[thisInVal]; // current coded byte
    // first value is always the first unique
    outVals[0] = pUniques[0]; // first val is always first unique
    // encoding bit for first two values in first bit of encoded bytes
    if (thisVal & 1)
    {
        // second value matches first
        outVals[1] = outVals[0];
        nUniques = 1;
    }
    else
    {
        // second value is a unique
        outVals[1] = pUniques[1];
        nUniques = 2;
    }
    uint32_t nUniqueBits = 1; // current number uniques determines 1-6 bits used: 1 bit for 1 or 2 uniques to start;
    nextOutVal=2; // start with third output value
    const uint32_t nOrigMinus1=nOriginalValues-1;
    while (nextOutVal < nOrigMinus1)
    {
        if ((thisVal & (1 << (bitPos))) == 0)
        {
            // new unique value
            outVals[nextOutVal++] = pUniques[nUniques];
            if (++bitPos == 8)
            {
                // unique from start of input values
                thisVal = inVals[++thisInVal];
                bitPos = 0;
            }
            if (nUniques < MAX_UNIQUES_EXTENDED_STRING_MODE)
                nUniqueBits = encodingBits512[nUniques]; // current number uniques determines 1-6 bits used
            nUniques++;
        }
        else
        {
            // repeat or string
            if (++bitPos == 8)
            {
                thisVal = inVals[++thisInVal];
                bitPos = 0;
            }
            if ((thisVal & (1 << (bitPos))) == 0)
            {
                // repeat: next number of bits, determined by nUniques, indicates repeated value
                if (++bitPos == 8)
                {
                    thisVal = inVals[++thisInVal];
                    bitPos = 0;
                }
                dsmGetBits(inVals, nUniqueBits, &thisInVal, &thisVal, &bitPos, &theBits);
                outVals[nextOutVal++] = pUniques[theBits]; // get uniques from start of inVals
            }
            else
            {
                // string: 11 plus string length of 2 to STRING_LENGTH
                if (++bitPos == 8)
                {
                    thisVal = inVals[++thisInVal];
                    bitPos = 0;
                }
                // multi-character string: length, then location of values in bits needed to code current pos
                dsmGetBits(inVals, extended_string_length_bits, &thisInVal, &thisVal, &bitPos, &theBits);
                uint32_t stringLen = (uint32_t)theBits + 2;
                assert(stringLen <= (extended_string_length_bits==3 ? 9:17));
                if (nextOutVal < 256)
                {
                    const uint32_t nPosBits = encodingBits512[nextOutVal];
                    dsmGetBits(inVals, nPosBits, &thisInVal, &thisVal, &bitPos, &theBits); // 8 bits
                }
                else
                {
                    dsmGetBits2(inVals, 9, &thisInVal, &thisVal, &bitPos, &theBits); // 9 bits
                }
                uint32_t stringPos=(uint32_t)theBits;
                assert((uint32_t)stringPos+stringLen <= nextOutVal);
                assert(nextOutVal+stringLen <= nOriginalValues);
                memcpy(outVals+nextOutVal, outVals+stringPos, stringLen);
                nextOutVal += stringLen;
            }
        }
    }
    if (nextOutVal == nOrigMinus1)
    {
        // input last byte in input when not ending with a string
        // string at end will catch last byte
        dsmGetBits(inVals, 8, &thisInVal, &thisVal, &bitPos, &theBits);
        outVals[nOrigMinus1] = (unsigned char)theBits;
    }
    if (bitPos > 0)
        thisInVal++; // inc past partial input value
    *bytesProcessed = thisInVal;
    return (int32_t)nOriginalValues;
} // end decodeExtendedStringMode
