//
//  td512.c
//  high-speed lossless tiny data compression of 1 to 512 bytes based on td64
//  version 1.1
//
//  td512 uses td64 to compress 6 to 512 bytes. 1 to 5 values are output without compression. td512 outputs all data necessary to decode the original values with td512d.
//
//  Created by Stevan Leonard on 10/30/21.
//  Copyright © 2021 L. Stevan Leonard. All rights reserved.
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
 along with this program.  If not, see <https://www.gnu.org/licenses/>.//
 */

#include "td64.h"
#include "string.h"
int32_t td512(unsigned char *inVals, unsigned char *outVals, const uint32_t nValues)
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
        if (nValues < 6)
        {
            // output 1 to 5 values without compression
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
    uint32_t outputOffset=2; // first two bytes for info
    uint32_t passFail=0;
    uint32_t passFailBit=1;
    uint32_t bytesProcessed;
    uint32_t nBlockBytes;
    retBytes = 2;
    while (nBytesRemaining >= 6)
    {
        nBlockBytes = nBytesRemaining <=64 ? nBytesRemaining : 64;
        if ((retBits=td64(inVals+inputOffset, outVals+outputOffset, nBlockBytes)) < 0)
            return retBits; // error occurred
        if (retBits == 0)
        {
            // failure leaves pass/fail bit 0
            memcpy(outVals+outputOffset, inVals+inputOffset, 64);
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
        }
        nBytesRemaining -= nBlockBytes;
        passFailBit <<= 1;
        inputOffset += nBlockBytes;
        outputOffset += bytesProcessed;
    }
    if (nBytesRemaining > 0)
    {
        // final block is < 6 values
        // copy 1 to 5 bytes without compression
        // pass/fail bit is 0
        memcpy(outVals+outputOffset, inVals+inputOffset, nBytesRemaining);
        retBytes += nBytesRemaining;
        nBytesRemaining = 0;
    }
    
    if (nValues <= 320)
    {
        // nValues 65 to 320
        outVals[0] = (unsigned char)((nValues-65) << 2) | 1; // indicator of 65 to 320 values, and lower 6 bitrs of 8-bit excess 65 value
        outVals[1] = (unsigned char)((nValues-65)>>6) & 3; // upper 2 bits of 8-bit value
        outVals[1] |= (unsigned char)(passFail << 2); // next 5 bits record pass/fail status for up to 5 compressed blocks
    }
    else
    {
        // nValues 321 to 512
        outVals[0] = (unsigned char)((nValues-321) << 2) | 3; // indicator of 321 to 512 values, and lower 6 bits of 8-bit excess 321 value
        outVals[1] = (unsigned char)((nValues-321)>>6) & 3; // upper 2 bits of 8-bit value
        if (nValues <= 384)
        {
            // only 6 pass/fail bits required
            outVals[1] |= (unsigned char)(passFail << 2); // next 6 bits record pass/fail status for 6 compressed blocks
        }
        else
        {
            // 7 or 8 pass/fail bits
            // try to compress pass/fail bits to fit in 2 bytes
            if ((passFail & 0xf) == 0 || (passFail & 0xf) == 0xf)
            {
                // passFail compressed to 6 values: first bit 1 indicates compressed, followed by 0 (fail) or 1 (pass) of first four pass/fail bits
                passFail = (passFail >> 2) | 1;
                outVals[1] |= (unsigned char)(passFail << 2);
            }
            else
            {
                // store pass/fail byte in third byte of output: requires move of compressed bytes by 1 byte
                // third bit of outVals[1] is 0 (no compression)
                memmove(outVals+3, outVals+2, retBytes-2);
                outVals[2] = (unsigned char)passFail;
                retBytes++;
            }
        }
    }
    return retBytes;
} // end td512

int32_t td512d(unsigned char *inVals, unsigned char *outVals, uint32_t *totalBytesProcessed)
{
    // decompress td512 compressed data
    // first bit or two indicate number of values from 1 to 512
    //  0 1 to 64 values plus 1 pass/fail
    // 01 65 to 320 values plus 5 pass/fail (requires a second byte)
    // 11 321 to 512 values plus 8 pass/fail (may require a third byte)
    // return number of bytes output
    int32_t retBytes=0;
    uint32_t nValues;
    uint32_t bytesProcessed;
    uint32_t passFail;
    uint32_t passFailBit=1;
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
    uint32_t inputOffset=2; // point past first two info bytes
    uint32_t outputOffset=0;
    uint32_t nBytesRemaining;
    int32_t blockRetBytes;
    // decompress 65 to 512 values
    if ((firstByte & 3) == 1)
    {
        // 65 to 320 values
        nValues = ((firstByte >> 2) | (secondByte & 3) << 6) + 65;
        passFail = secondByte >> 2;
    }
    else
    {
        // 321 to 512 values
        nValues = ((firstByte >> 2) | (secondByte & 3) << 6) + 321;
        if (nValues <= 384)
        {
            // 6 pass/fail bits in second byte
            passFail = secondByte >> 2;
        }
        else
        {
            if (secondByte & 4)
            {
                // pass/fail bits compressed in upper 6 bits second byte
                passFail = secondByte & 0xf0; // upper four bits of pass/fail
                if (secondByte & 8)
                    passFail |= 0xf; // first four pass
                // else first four fail (0s)
            }
            else
            {
                // pass/fail in third byte
                passFail = inVals[2];
                inputOffset = 3; // point past first three bytes
            }
        }
    }
    nBytesRemaining = nValues;
    uint32_t nBlocks=nValues/64;
    if (nValues & 63)
        nBlocks++;
    uint32_t thisBlock=1;
    while (nBytesRemaining > 0)
    {
        // decompress next block
        // must provide the number of original values, which will be 64 for all but the last block
        uint32_t nBlockVals= thisBlock++ == nBlocks ? nBytesRemaining : 64;
        if (passFail & passFailBit)
        {
            // decode compressed values
            assert(nBytesRemaining != 1); // error in compressed count
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
        passFailBit <<= 1;
        inputOffset += bytesProcessed;
        outputOffset += (uint32_t)blockRetBytes;
        retBytes += nBlockVals;
    }
    *totalBytesProcessed = inputOffset;
    if (retBytes != (int32_t)nValues)
        return -129;
    return (int32_t)nValues;
}
