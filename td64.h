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
// Notes for version 1.1.0:
/*
 1. Main program reads a file into memory that is compressed by
    calling td512 repeatedly. When complete, the compressed data is
    written to a file and read for decompression by calling td512d.
      td512 filename [loopCount]]
        filename is required argument 1.
        loopCount is optional argument 2 (default: 1). Looping is performed over the entire input file.
 */
// Notes for version 1.1.1:
/*
 1. Updated some descriptive comments.
 */
// Notes for version 1.1.2:
/*
 1. Moved 7-bit mode defines to td64.h because they are used
    outside of the 7-bit mode.
 2. When fewer than minimum values to use 7-bit mode of 16, don't
    accumulate high bit when reasonable. Main loop keeps this in because
    time required is minimal.
 3. When fewer than 24 input values, but greater than or equal to minimum
    values of 16 to use 7-bit mode, use 6% as minimum compression for
    compression modes used prior to 7-bit mode.
 */
// Notes for version 1.1.3:
/*
 1. Fixed bugs in td5 and td5d functions.
 2. Recognize random data starting at 16 input values.
 */
// Notes for version 1.1.4:
/*
 1. Added bit text mode that uses variable length encodig bits
    to maximize compression. td5 still uses the fixed bit text mode.
 2. Changed the random data metric to use number values init
    loop * 7/8 + 1 to be threshold for random data.
 3. Implemented a static global for decoding bit text mode and
    string mode to limit reads of input values.
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
#define MIN_VALUES_7_BIT_MODE 16
#define MIN_VALUE_7_BIT_MODE_12_PERCENT 24 // min value where 7-bit mode expected to approach 12%, otherwise 6%

int32_t td5(const unsigned char *inVals, unsigned char *outVals, const uint32_t nValues);
int32_t td5d(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed);
int32_t td64(unsigned char *inVals, unsigned char *outVals, const uint32_t nValues);
int32_t td64d(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed);

#endif /* td64_h */
