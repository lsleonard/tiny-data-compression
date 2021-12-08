//  td512.h
//  high-speed lossless tiny data compression of 1 to 512 bytes based on td64
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
 1. Added bit text mode that uses variable length encoding bits
    to maximize compression. td5 still uses the fixed bit text mode.
 2. Changed the random data metric to use number values init
    loop * 7/8 + 1 to be threshold for random data.
 3. Implemented a static global for decoding bit text mode and
    string mode to limit reads of input values.
*/
// Notes for version 1.1.5
/*
 1. Added adaptive text mode that looks for occurrences of characters
    that are common to a particular data type when fewer than 3/4 of
    the input values are matched by a predefined character. Defined
    XML and HTML based on '<', '>', '/' and '"'. Defined C or other
    code files based on '*', '=', ';' and '\t'. Eight characters
    common to the text type are defined in the last 8 characters of
    the characters encoded.
 2. Added compression of high bit in unique characters in string mode
    when the high bit is 0 for all values.
 3. Set the initial loop in td64 to 7/16 of input values for 24 or
    more inputs. This provides a better result for adaptive text mode.
 */
#ifndef td512_h
#define td512_h

#include "td64.h"
#include <unistd.h>

int32_t td512(unsigned char *inVals, unsigned char *outVals, const uint32_t nValues);
int32_t td512d(unsigned char *inVals, unsigned char *outVals, uint32_t *totalBytesProcessed);

#endif /* td512_h */
