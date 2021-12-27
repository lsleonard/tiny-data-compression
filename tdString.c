//
//  tdString.c
//  td512 calls this function to compress 65 to 512 values
//      but limited by 64 uniques
//
//  Created by Stevan Leonard on 12/8/21.
//

#include "tdString.h"
#include "td64_internal.h"

#define STRING_LIMIT 17
#define MAX_STRING_MODE_EXTENDED_VALUES 512

static inline void esmOutputBits(unsigned char *outValsT, const uint32_t nBits, const uint32_t bitVal, uint32_t *nextOutIx, uint32_t *nextOutBit)
{
    // output 1 to 8 bits
    outValsT[*nextOutIx] |= (unsigned char)(bitVal << *nextOutBit);
    *nextOutBit += nBits;
    if (*nextOutBit >= 8)
    {
        *nextOutBit -= 8;
        outValsT[++(*nextOutIx)] = (unsigned char)bitVal >> (nBits - *nextOutBit);
    }
} // end esmOutputBits

int32_t encodeStringModeExtended(const unsigned char *inVals, unsigned char *outVals, const uint32_t nValuesMax, uint32_t *nValuesOut)
{
    // encode strings and repeats in input for up to 64 unique values
    // then conclude processing and return if either number uniques
    // exceeds 64 or nValuesMax are processed
    uint32_t inPos; // current position in inVals
    uint32_t inVal;
    uint32_t nUniques; // first value is always a unique
    unsigned char val256[256]={0};
    uint32_t uniqueOccurrence[256]; // set to the count of the first occurrence of that value
    uint64_t twoVals[64]; // index is first unique val, with bit position of second unique value set to 1
    uint32_t twoValsPoss[64*64]; // position in input of first occurrence of corresponding two unique values of up to 64
    uint32_t twoValsPos;
    uint32_t nextOutIx;
    uint32_t nextOutBit=0; // start of encoding after first two inputs
    unsigned char outValsT[MAX_STRING_MODE_EXTENDED_VALUES];
    uint32_t maxUniquesExceeded=0;
    uint32_t highBitClear;
    
    if (nValuesMax > MAX_STRING_MODE_EXTENDED_VALUES)
        return -1;
    nextOutIx = 0; // start of encoding in outValsT
    outValsT[0] = 0; // init byte in process to 0
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
        outVals[1] = 64; // 1=repeat for second value
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
        outVals[1] = 0; // 0=encode first two uniques
    }
    inPos=2; // start loop after init of first two values
    const uint32_t lastPos=nValuesMax-1;
    while (inPos < lastPos)
    {
        if (nextOutIx+nUniques > nValuesMax-1)
            return 0;
        inVal = inVals[inPos++]; // inPos inc'd to next position
        uint32_t UOinVal=uniqueOccurrence[inVal];
        uint32_t UOinValsInPosP1;
        if (val256[inVal] == 0)
        {
            // set up for new unique in this position
            if (nUniques == MAX_UNIQUES_EXTENDED_STRING_MODE)
            {
                maxUniquesExceeded = inPos - 2;
                break;
            }
            uniqueOccurrence[inVal] = nUniques;
            UOinVal = nUniques;
            val256[inVal] = 1;
            if (val256[inVals[inPos]] == 0)
            {
                // new unique in next pos
                if (nUniques == MAX_UNIQUES_EXTENDED_STRING_MODE)
                {
                    maxUniquesExceeded = inPos - 2;
                    break;
                }
                UOinValsInPosP1 = nUniques + 1; // use but don't set up
            }
            else
            {
                UOinValsInPosP1 = uniqueOccurrence[inVals[inPos]];
            }
            twoVals[nUniques] = 1llu << UOinValsInPosP1;
            // set position of these two vals one past second value
            twoValsPoss[(nUniques<<6) | UOinValsInPosP1] = inPos + 1;
            nUniques++;
            highBitClear |= inVal;
            // output a 0 to indicate new unique
            if (++nextOutBit == 8)
            {
                // update out index and next out bit
                outValsT[++nextOutIx] = 0;
                nextOutBit = 0;
            }
            outVals[nUniques+1] = (unsigned char)inVal; // save unique in list at front of outVals starting in third position
            continue;
        }
        // this character occurred before: look for repetition of next character
        if (val256[inVals[inPos]] == 0)
        {
            // new unique in next pos so output a repeated value for current position
            if (nUniques == MAX_UNIQUES_EXTENDED_STRING_MODE)
            {
                maxUniquesExceeded = inPos - 2;
                break;
            }
            UOinValsInPosP1 = nUniques; // will be set to next unique on next loop
            // repeated value: 01
            // save new pair of this value and next
            twoVals[UOinVal] |= 1llu << UOinValsInPosP1;
            twoValsPoss[(UOinVal<<6) | UOinValsInPosP1] = inPos + 1;
            // output repeated value: 01 plus unique occurrence
            esmOutputBits(outValsT, 2 + encodingBits512[nUniques-1], 1|(UOinVal<<2), &nextOutIx, &nextOutBit);
            continue;
        }
        uint64_t TVuniqueOccurrence=twoVals[UOinVal];
        UOinValsInPosP1 = uniqueOccurrence[inVals[inPos]];
        if (TVuniqueOccurrence & (1llu << UOinValsInPosP1))
        {
            // found pair of matching values using next input value
            // look for continuation of matching characters
            twoValsPos = twoValsPoss[(UOinVal<<6) | UOinValsInPosP1];
            if (twoValsPos == inPos)
            {
                // two vals include first value so this is a repeat
                // output repeated value: 01 plus unique occurrence
                esmOutputBits(outValsT, 2+encodingBits512[nUniques-1], 1|(UOinVal<<2), &nextOutIx, &nextOutBit);
                continue;
            }
            
            uint32_t strPos=inPos+1;
            uint32_t strLimit = inPos - 1 - twoValsPos; // don't take string past current input pos
            if (nValuesMax-strPos < strLimit)
                strLimit = nValuesMax - strPos; // don't go past end of input
            if (strLimit > STRING_LIMIT-2)
                strLimit = STRING_LIMIT-2;
            
            uint32_t strCount=0;
            while(strCount++ < strLimit && inVals[strPos] == inVals[twoValsPos])
            {
                strPos++;
                twoValsPos++;
            }
            // output 11 plus 4 more bits for string length 2 to 17
            esmOutputBits(outValsT, 6, 3 | ((strCount-1)<<2), &nextOutIx, &nextOutBit);
            // output the position of string
            if (encodingBits512[inPos-1] > 8)
            {
                // output lowest bit and then remaining 8
                uint32_t outVal9=twoValsPos-strCount-1;
                esmOutputBits(outValsT, 1, outVal9&1, &nextOutIx, &nextOutBit);
                esmOutputBits(outValsT, 8, outVal9>>1, &nextOutIx, &nextOutBit);
            }
            else
            {
                esmOutputBits(outValsT, encodingBits512[inPos-1], twoValsPos-strCount-1, &nextOutIx, &nextOutBit);
            }
            inPos += strCount;
        }
        else
        {
            // repeated value: 01
            // save new pair of this value and next
            twoVals[UOinVal] |= 1llu << UOinValsInPosP1;
            twoValsPoss[(UOinVal<<6) | UOinValsInPosP1] = inPos + 1;
            // output repeated value: 01 plus unique occurrence
            esmOutputBits(outValsT, 2 + encodingBits512[nUniques-1], 1|(UOinVal<<2), &nextOutIx, &nextOutBit);
        }
    }
    
    // output final bits
    if (nextOutBit > 0)
        nextOutIx++; // index past final bits
    if (maxUniquesExceeded == 0 && inPos < nValuesMax)
    {
        outValsT[nextOutIx++] = inVals[lastPos]; // output last input byte
    }
    *nValuesOut = maxUniquesExceeded ? maxUniquesExceeded : nValuesMax;
    if (nextOutIx + nUniques > *nValuesOut - 1)
        return 0;
    // use 7-bit encoding on uniques if all high bits set
    int32_t uniqueOffset;
    if ((highBitClear & 0x80) == 0)
    {
        unsigned char compressedUniques[MAX_UNIQUES_EXTENDED_STRING_MODE];
        uniqueOffset = encode7bitsInternal(outVals+2, compressedUniques, nUniques) + 2;
        if (uniqueOffset > 2)
        {
            outVals[1] |= 128;
            memcpy(outVals+2, compressedUniques, nUniques);
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
    memcpy(outVals+uniqueOffset, outValsT, nextOutIx);
    outVals[0] = 0x7f; // indicate external string mode
    outVals[1] |= nUniques-1; // number uniques in first 6 bits then first encoding bit and finally compressed uniques bit

    return (int32_t)(nextOutIx+uniqueOffset) * 8;
} // end encodeStringModeExtended

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

int32_t decodeStringModeExtended(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed)
{
    uint32_t nextOutVal;
    uint32_t thisInVal; // position of encoded bits
    uint32_t bitPos=0; // start decoding from bit 0
    int32_t theBits;
    uint32_t nUniques; // running number of uniques
    const unsigned char *pUniques; // pointer to uniques
    unsigned char uncompressedUniques[MAX_UNIQUES_EXTENDED_STRING_MODE];\
    uint32_t secondByte=inVals[1]; // bits= 0:5 nUniques  6 first encoding bit  7 compressed or not
    uint32_t nUniquesIn=(secondByte & 0x3f) + 1;

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
        assert(ret7bits == nUniquesIn);
        thisInVal += 2; // point past initial byte for encoded bytes
    }
    // first value is always the first unique
    outVals[0] = pUniques[0]; // first val is always first unique
    // encoding bit for first two values in seventh bit of second byte
    if (secondByte & 64)
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
    nextOutVal=2; // start with third output value
    const uint32_t nOrigMinus1=nOriginalValues-1;
    uint32_t thisVal = inVals[thisInVal];
    while (nextOutVal < nOrigMinus1)
    {
        if ((thisVal & (1 << (bitPos))) == 0)
        {
            // new unique value
            if (bitPos == 7)
            {
                // unique from start of input values
                outVals[nextOutVal++] = pUniques[nUniques];
                thisVal = inVals[++thisInVal];
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
                uint32_t nUniqueBits = encodingBits512[nUniques-1]; // current number uniques determines 1-6 bits used
                dsmGetBits2(inVals, nUniqueBits, &thisInVal, &thisVal, &bitPos, &theBits);
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
                dsmGetBits2(inVals, 4, &thisInVal, &thisVal, &bitPos, &theBits);
                uint32_t stringLen = (uint32_t)theBits + 2;
                assert(stringLen <= STRING_LIMIT);
                uint32_t nPosBits = encodingBits512[nextOutVal];
                dsmGetBits2(inVals, nPosBits, &thisInVal, &thisVal, &bitPos, &theBits);
                uint32_t stringPos=(uint32_t)theBits;
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
} // end decodeStringModeExtended
