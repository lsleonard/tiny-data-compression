//
//  td64.h
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
#define MIN_STRING_MODE_UNIQUES 17 // string mode stores unique count excess 16
#define MIN_VALUES_7_BIT_MODE 16
#define MIN_VALUE_7_BIT_MODE_12_PERCENT 24 // min value where 7-bit mode expected to approach 12%, otherwise 6%
//#define TD64_TEST_MODE // enable this macro to collect some statistics with variables g_td64...

int32_t td5(const unsigned char *inVals, unsigned char *outVals, const uint32_t nValues);
int32_t td5d(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed);
int32_t td64(const unsigned char *inVals, unsigned char *outVals, const uint32_t nValues);
int32_t td64d(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed);
int32_t encodeAdaptiveTextMode(const unsigned char *inVals, unsigned char *outVals, const uint32_t nValues, const unsigned char *val256, const uint32_t predefinedTextCharCnt, const uint32_t highBitclear, const uint32_t maxBytes);
int32_t decodeAdaptiveTextMode(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed);

#endif /* td64_h */
