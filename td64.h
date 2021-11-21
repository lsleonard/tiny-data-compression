//
//  td64.h
//  high-speed lossless tiny data compression of 1 to 64 bytes based on td64
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
// Notes for version 1.1:
/*  1. Tiny data compression integrates several compression modes: fixed
    bit, text mode, single value, string mode, and 7-bit encoding and
    decoding. td64 returns the number of compressed bytes or 0 if
    compression failed or a negative value if an error occurred. Values
    are returned only if compression succeeds. Decoding of the td64 values
    requires the caller to supply the number of original bytes.
*/
#ifndef td64_h
#define td64_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#define NDEBUG // disable asserts
#include <assert.h>

#define MAX_TD64_BYTES 64  // max input vals supported
#define MIN_TD64_BYTES 1  // min input vals supported
#define MAX_UNIQUES 16 // max uniques supported in input
#define MAX_STRING_MODE_UNIQUES 32 // max uniques supported by string mode
#define MIN_VALUES_STRING_MODE 32 // min values to use string mode

int32_t td5(const unsigned char *inVals, unsigned char *outVals, const uint32_t nValues);
int32_t td5d(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed);
int32_t td64(unsigned char *inVals, unsigned char *outVals, const uint32_t nValues);
int32_t td64d(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed);

#endif /* td64_h */
