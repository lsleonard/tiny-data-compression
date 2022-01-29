//
//  td512.c
//  high-speed lossless tiny data compression of 1 to 512 bytes
//
//  td512 uses td64 to compress 16 to 512 bytes. 1 to 15 values are output without compression. td512 outputs all data necessary to decode the original values with td512d.
//
//  Created by L. Stevan Leonard on 10/30/21.
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

#include "td512.h"
#include "string.h"

#ifdef TD512_TEST_MODE
uint32_t gExtendedStringCnt=0;
uint32_t gExtendedTextCnt=0;
uint32_t gtd64Cnt=0;
#endif


// use textChars to eliminate non-text values that have text chars
const uint32_t textChars[256]={
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, // 0xA 0xD
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, // space ! " comma .
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, // ?
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // A...O
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, // P...Z
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // a...o
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, // p...z
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

uint32_t checkSingleValueMode(const unsigned char *inVals, unsigned char *tempOutVals)
{
    unsigned char val256[256]={0};
    uint32_t count[MAX_TD64_BYTES]={0};
    uint32_t i=0;
    
    while (i < 64)
    {
        const uint32_t inVal=inVals[i++];
        count[val256[inVal]++]++;
    }
    if (count[0] > 40)
        return 1; // more uniques than usually compress
    if (count[0] <= 2)
        return 1; // all one or two values: fixed bit coding compresses well
    if (count[35])
    {
        return 1; // fine line between choosing td64 and extended string mode as shown between files mr and nci, for high counts of a repeated value
    }
    const uint32_t minRepeatsSingleValueMode=18;
    if (count[minRepeatsSingleValueMode] && count[0] >  MIN_UNIQUES_SINGLE_VALUE_MODE_CHECK)
    {
        // single value mode works for up to 46 uniques, but files with repeating values, such as paper-100k.pdf, compress better using string mode
        int32_t retBits;
        int32_t retBitstd64;
        uint32_t nValuesRead;
        retBits = encodeExtendedStringMode(inVals, tempOutVals, 64, &nValuesRead);
        if (retBits <= 0)
            return 1; // process this block with td64
        if (retBits > (retBitstd64=td64(inVals, tempOutVals, 64)))
            return retBitstd64; // pick td64 if it compresses better and return compressed values
    }
    return 0;
} // end checkSingleValueMode

uint32_t checkTextMode(const unsigned char *inVals, uint32_t nValues, uint32_t *highBitCheck)
{
    // try to determine whether this data should be compressed with extended text mode
    if (nValues < 128)
        return 0; // expecting at last 128 values
    uint32_t charCount=0;
    uint32_t predefinedCharCount=0;
    uint32_t thisHighBitCheck=0;
    uint32_t i=0;
    
    while (i<16)
    {
        const uint32_t inVal=inVals[i++];
        if (inVal & 0x80)
            return 0; // return as quickly as possible if not mostly 7-bit values
        charCount += textChars[inVal];
        predefinedCharCount += predefinedBitTextChars[inVal];
    }
    // need more than 64 values to make a good determination about data
    while (i<96)
    {
        const uint32_t inVal=inVals[i++];
        charCount += textChars[inVal];
        predefinedCharCount += predefinedBitTextChars[inVal];
        thisHighBitCheck |= inVal;
    }
    if (charCount < 90)
        return 0; // too few text chars
    if (predefinedCharCount < 72)
        return 0; // too few predefined chars
    if ((thisHighBitCheck & 0x80) == 0)
        while (i<nValues)
            thisHighBitCheck |= inVals[i++];
    *highBitCheck = (thisHighBitCheck & 0x80) == 0;
    return 1;
} // end checkTextMode

int32_t td512(const unsigned char *inVals, unsigned char *outVals, const uint32_t nValues)
{
    // set initial bits according to number of values
    //  0 1 to 64 values plus 1 pass/fail
    // 01 65 to 320 values plus 5 pass/fail (requires a second byte)
    // 11 321 to 512 values plus 8 pass/fail (may require a third byte)
    // returns number of bytes output
    int32_t retBits;
    int32_t retBytes;
    if ((nValues == 0) || (nValues > 512))
    {
        return -128; // number of input values not supported
    }
    if (nValues <= 64)
    {
        // process 1 to 64 values
        outVals[0] = (unsigned char)((nValues-1) << 1); // first bit indicates <= 64 vals, followed by number values
        if (nValues < MIN_VALUES_TO_COMPRESS)
        {
            // output values without compression
            memcpy(outVals+1, inVals, nValues);
            return (int32_t)nValues + 1;
        }
        if ((retBits=td64(inVals, outVals+1, nValues)) < 0)
            return retBits; // error occurred
        if (retBits == 0)
        {
            // failure leaves high-order pass/fail bit 0
            memcpy(outVals+1, inVals, nValues);
            return (int32_t)nValues + 1;
        }
        else
        {
            // pass sets high-order pass/fail bit to 1
            outVals[0] |= 128;
        }
        retBytes = retBits / 8 + 1; // 1 info byte
        if (retBits & 7)
            retBytes++;
        return retBytes;
    } // nValues <= 64
    
    // process 65 to 512 values
    uint32_t nBytesRemaining=nValues;
    uint32_t inputOffset=0;
    uint32_t outputOffset;
    uint32_t passFail=0;
    uint32_t passFailBit=1;
    uint32_t bytesProcessed;
    uint32_t nBlockBytes;
    uint32_t td64on=0;
    uint32_t extendedMode=0; // 0=td64  1,2=extended mode
    outVals[1] = 0;
    if (nValues <= 256)
    {
        outputOffset = 2;
        retBytes = 2; // 2 info bytes for 1 to 256 values
    }
    else
    {
        outputOffset = 3;
        retBytes = 3; // 3 info bytes for 257 to 512 values
    }
    
    while (nBytesRemaining >= MIN_VALUES_TO_COMPRESS)
    {
        //uint32_t infoByte=0;
        if (nBytesRemaining < MIN_VALUES_EXTENDED_MODE || td64on)
        {
            nBlockBytes = nBytesRemaining <=MAX_TD64_BYTES ? nBytesRemaining : MAX_TD64_BYTES;
            if ((retBits=td64(inVals+inputOffset, outVals+outputOffset, nBlockBytes)) < 0)
                return retBits; // error occurred
            if (retBits == 0)
            {
                // failure leaves pass/fail bit 0
                memcpy(outVals+outputOffset, inVals+inputOffset, nBlockBytes);
                bytesProcessed = nBlockBytes;
                retBytes += nBlockBytes;
            }
            else
            {
                // pass sets pass/fail bit to 1
                passFail |= passFailBit;
                bytesProcessed = (uint32_t)retBits / 8;
                if (retBits & 7)
                    bytesProcessed++;
                retBytes += bytesProcessed;
                //infoByte = *(outVals+outputOffset);
            }
            nBytesRemaining -= nBlockBytes;
            passFailBit <<= 1;
            inputOffset += nBlockBytes;
            outputOffset += bytesProcessed;
            continue;
        } // end td64 processing
        else
        {
            // -------check if all input values can be handled together--------
            uint32_t highBitCheck;
            if (checkTextMode(inVals, nBytesRemaining, &highBitCheck))
            {
                // process in extended text mode
                unsigned char val256[256];
#ifdef TD512_TEST_MODE
                gExtendedTextCnt++;
#endif
                extendedMode = 1; // text compression mode called directly
                retBits = encodeAdaptiveTextMode(inVals+inputOffset, outVals+outputOffset, nBytesRemaining, val256, 1, highBitCheck, nBytesRemaining-16);
                if (retBits < 0)
                    return retBits;
                if (retBits == 0)
                {
                    // too many non-predefined chars to compress
                    // fail the first 128 chars and continue with td64
                    // 128 is an arbitrary value, but enough failure to find new data
                    passFailBit <<= 1;
                    memcpy(outVals+outputOffset, inVals+inputOffset, 128);
                    retBytes += 128;
                    nBytesRemaining -= 128;
                    outputOffset += 128;
                    inputOffset += 128;
                    td64on = 1; // process remaining values with td64
                    continue;
                }
                // pass sets pass/fail bit to 1
                passFail |= passFailBit;
                bytesProcessed = (uint32_t)retBits / 8;
                if (retBits & 7)
                    bytesProcessed++;
                retBytes += bytesProcessed; // add byte for count
                outputOffset += bytesProcessed;
                inputOffset += nBytesRemaining;
                nBytesRemaining = 0; // all values processed
                continue;
            } // end text mode processing
            unsigned char tempOutVals[MAX_TD64_BYTES];
            if ((retBits=checkSingleValueMode(inVals, tempOutVals)))
            {
                // determine data best handled by td64
#ifdef TD512_TEST_MODE
                gtd64Cnt++;
#endif
                if (retBits > 1)
                {
                    // use the values from check to td64
                    passFail |= passFailBit;
                    bytesProcessed = (uint32_t)retBits / 8;
                    if (retBits & 7)
                        bytesProcessed++;
                    memcpy(outVals+outputOffset, tempOutVals, bytesProcessed);
                    retBytes += bytesProcessed;
                    nBytesRemaining -= MAX_TD64_BYTES;
                    passFailBit <<= 1;
                    inputOffset += MAX_TD64_BYTES;
                    outputOffset += bytesProcessed;
                }
                td64on = 1;
                continue;
            }
            // use extended string mode
            uint32_t nValuesRead;
#ifdef TD512_TEST_MODE
            gExtendedStringCnt++;
#endif
            extendedMode = 2; // extended string mode called directly
            // add 1 byte for number values read as extended string mode stops after 64 uniques encountered
            outputOffset++;
            retBytes++;
            retBits = encodeExtendedStringMode(inVals+inputOffset, outVals+outputOffset, nValues, &nValuesRead);
            if (nValues < nValuesRead)
                return -44; // debug check
            if (retBits < 0)
                return retBits;
            if (retBits == 0)
            {
                // no compression
                passFailBit <<= 1;
                memcpy(outVals+outputOffset, inVals+inputOffset, nValuesRead);
                bytesProcessed = nValuesRead;
            }
            else
            {
                // compression
                passFail |= passFailBit;
                passFailBit <<= 1;
                bytesProcessed = retBits / 8;
                if (retBits & 7)
                    bytesProcessed++;
            }
            if (nValues <= 256)
                outVals[outputOffset-1] = (unsigned char)(nValuesRead-1); // string mode count
            else
            {
                outVals[outputOffset-1] = (unsigned char)(nValuesRead-1); // lower 8 bits of count
                outVals[1] |= (unsigned char)((nValuesRead-1)>>4)&0x10; // save upper bit in info byte 1 above extended mode bits
            }
            outputOffset += bytesProcessed;
            inputOffset += nValuesRead;
            retBytes += bytesProcessed;
            nBytesRemaining -= nValuesRead;
            td64on = 1; // process remaining values with td64
            continue;
        } // end string mode processing
    }
    if (nBytesRemaining > 0)
    {
        // final block is < MIN_VALUES_TO_COMPRESS
        // copy bytes without compression
        // pass/fail bit is 0
        memcpy(outVals+outputOffset, inVals+inputOffset, nBytesRemaining);
        retBytes += nBytesRemaining;
    }
    // --------------- END OF COMPRESSION ---------------
    // process info bytes
    if (nValues <= 320)
    {
        // nValues 65 to 320, excess 65 value
        outVals[0] = (unsigned char)((nValues-65) << 2) | 1; // indicator of 65 to 320 values, and lower 6 bits of 8-bit excess 65 value
        outVals[1] |= (unsigned char)((nValues-65)>>6) & 3; // upper 2 bits of value
    }
    else
    {
        // nValues 321 to 512, excess 321 value
        outVals[0] = (unsigned char)((nValues-321) << 2) | 3; // indicator of 321 to 512 values, and lower 6 bits of 8-bit excess 321 value
        outVals[1] |= (unsigned char)((nValues-321)>>6) & 3; // upper 2 bits of value; string mode uses third bit for 9th bit of string count; for <=256 values, upper four bits are pass/fail
    }
    // two bits indicates how compression starts; extended modes for >= 128 values continue with td64 for any remaining values
    // 0 td64
    // 1 extended text mode
    // 2 extended string mode
    outVals[1] |= extendedMode << 2; // used for >= 128 values
    if (nValues <= 256)
    {
        outVals[1] |= passFail << 4; // use upper four bits of second info byte
    }
    else
    {
        outVals[2] = passFail; // use third info byte
    }
    return retBytes;
} // end td512

int32_t td512d(const unsigned char *inVals, unsigned char *outVals, uint32_t *totalBytesProcessed)
{
    // decompress td512 compressed data
    // first bit or two indicate number of values from 1 to 512
    //  0 1 to 64 values plus 1 pass/fail bit
    // 01 65 to 320 values: excess 65 value, second byte holds upper two bits value
    // 11 321 to 512 values: excess 321 second byte holds upper two bits value
    // for 65 to 512 values: all modes bits in second byte and pass/fail in third byte
    // return number of bytes output
    int32_t retBytes=0;
    uint32_t nValues;
    uint32_t bytesProcessed;
    uint32_t passFail;
    uint32_t firstByte=inVals[0];
    
    if ((firstByte & 1) == 0)
    {
        // decompress 1 to 64 values
        nValues = ((firstByte >> 1) & 0x3f) + 1;
        if (firstByte & 128)
        {
            retBytes = td64d(inVals+1, outVals, nValues, &bytesProcessed);
            *totalBytesProcessed = bytesProcessed + 1;
            return retBytes;
        }
        // uncompressed
        memcpy(outVals, inVals+1, nValues);
        *totalBytesProcessed = nValues + 1;
        return (int32_t)nValues;
    }
    
    uint32_t secondByte=inVals[1];
    uint32_t thirdByte=inVals[2];
    uint32_t inputOffset=2; // point past first two info bytes for <= 256 values
    uint32_t outputOffset=0;
    uint32_t nBytesRemaining;
    int32_t blockRetBytes;
    // decompress 65 to 512 values
    if ((firstByte & 3) == 1)
    {
        // 65 to 320 values
        nValues = ((firstByte >> 2) | (secondByte & 3) << 6) + 65;
    }
    else
    {
        // 321 to 512 values
        nValues = ((firstByte >> 2) | (secondByte & 3) << 6) + 321;
    }
    if (nValues != 512)
        nValues = nValues;
    nBytesRemaining = nValues;
    uint32_t extendedMode = (secondByte >> 2) & 3;
    if (nValues <= 256)
    {
        passFail = secondByte >> 4;
    }
    else
    {
        // pass fail in third byte for > 256 values
        passFail = thirdByte;
        inputOffset++;
    }
    if (extendedMode > 2)
        return -22;
    if (nValues >= MIN_VALUES_EXTENDED_MODE)
    {
        if (extendedMode == 1)
        {
            // process text mode
            if (passFail & 1)
            {
                // decompress
                blockRetBytes = decodeAdaptiveTextMode(inVals+inputOffset, outVals, nValues, &bytesProcessed);
                nBytesRemaining = 0;
            }
            else if (passFail == 0)
            {
                // all tests failed, copy all original values to output
                memcpy(outVals, inVals+inputOffset, nValues);
                *totalBytesProcessed = inputOffset + nValues;
                return (int32_t)nValues;
            }
            else
            {
                // no compression for this call: first 128 values are originals
                memcpy(outVals, inVals+inputOffset, 128); //
                bytesProcessed = 128;
                blockRetBytes = 128;
                nBytesRemaining = nValues - 128; // continue with td64
            }
            passFail >>= 1;
            inputOffset += bytesProcessed;
            outputOffset += blockRetBytes;
            retBytes = blockRetBytes;
        } // end process text mode
        else if (extendedMode == 2)
        {
            // process string mode
            uint32_t nValuesThisCall=inVals[inputOffset++]; // lower 8 bits of values read in first byte
            if (nValues <= 256)
                nValuesThisCall++; // 8-bit value
            else
            {
                nValuesThisCall |= (inVals[1]&0x10) << 4; // 9th bit
                nValuesThisCall++; // 9-bit value
            }
            if (passFail & 1)
            {
                blockRetBytes=decodeExtendedStringMode(inVals+inputOffset, outVals+outputOffset, nValuesThisCall, &bytesProcessed);
                if (blockRetBytes < 0)
                    return blockRetBytes; // error return
            }
            else if (passFail == 0)
            {
                // all tests failed, copy all original values to output
                memcpy(outVals, inVals+inputOffset, nValues);
                *totalBytesProcessed = inputOffset + nValues;
                return (int32_t)nValues;
            }
            else
            {
                // no compression for this block
                memcpy(outVals, inVals+inputOffset, nValuesThisCall);
                bytesProcessed = nValuesThisCall;
                blockRetBytes = nValuesThisCall;
            }
            inputOffset += bytesProcessed;
            outputOffset += nValuesThisCall;
            passFail >>= 1;
            retBytes = blockRetBytes;
            nBytesRemaining -= blockRetBytes; // continue with td64
        } // end process string mode
    }
    // td64 processing
    while (nBytesRemaining > 0)
    {
        if (passFail == 0)
        {
            // all remaining blocks failed to compress: copy remaining values to output
            memcpy(outVals+outputOffset, inVals+inputOffset, nBytesRemaining);
            *totalBytesProcessed = inputOffset + nBytesRemaining;
            return (int32_t)nValues;
        }
        // decompress next block
        // must provide the number of original values, which will be 64 for all but the last block
        uint32_t nBlockVals= nBytesRemaining >= 64 ? 64 : nBytesRemaining;
        if (passFail & 1)
        {
            // decode compressed values
            assert(nBytesRemaining >= MIN_VALUES_TO_COMPRESS); // error in compressed count
            blockRetBytes = td64d(inVals+inputOffset, outVals+outputOffset, nBlockVals, &bytesProcessed);
            if (blockRetBytes < 0)
                return blockRetBytes;
        }
        else
        {
            // get uncompressed values
            memcpy(outVals+outputOffset, inVals+inputOffset, nBlockVals);
            bytesProcessed = nBlockVals;
            blockRetBytes = (int32_t)nBlockVals;
        }
        nBytesRemaining -= nBlockVals;
        passFail >>= 1;
        inputOffset += bytesProcessed;
        outputOffset += (uint32_t)blockRetBytes;
        retBytes += nBlockVals;
    }
    *totalBytesProcessed = inputOffset;
    if (retBytes != (int32_t)nValues)
        return -129;
    return (int32_t)nValues;
}
