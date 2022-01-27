//  td512.h
//  high-speed lossless tiny data compression of 1 to 512 bytes
//
//  Created by L. Stevan Leonard on 10/31/21.
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
// Notes for version 2.1.1:
/*
 1. In td512.c, for 128 and more values, text and extended string modes
    are called for checked data. For other data, and for any remaining values
    from calls for 128 or more values, td64 is called.
    A minimum of 16 characters are compressed.
 2. In tdString.c, extended string mode was modified to stop on the 65th
    unique value. This value is the last value output and the number
    of values at that point is returned.
 3. In main.c, after decompression, the input file is verified against
    the decompressed output file.
*/
// Note for version 2.1.2:
/*
 1. In tdString.c, make the definition of string length and associated
    number of bits based on number of input values. For <= 64 values,
    length is 9 and bits are 3. For > 64 values, length is 17 and
    bits are 4. Longer strings are likely to be found in larger data sets.
 */
#ifndef td512_h
#define td512_h

#include "td64.h"
#include "tdString.h"
#include <unistd.h>

#define TD512_VERSION "v2.1.2"
#define MIN_VALUES_EXTENDED_MODE 128
#define MIN_UNIQUES_SINGLE_VALUE_MODE_CHECK 14
#define MIN_VALUES_TO_COMPRESS 16
//#define TD512_TEST_MODE // enable this macro to generate statistics

extern const uint32_t predefinedBitTextChars[256];

int32_t td512(const unsigned char *inVals, unsigned char *outVals, const uint32_t nValues);
int32_t td512d(const unsigned char *inVals, unsigned char *outVals, uint32_t *totalBytesProcessed);

#endif /* td512_h */
