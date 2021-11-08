//
//  td64.h
//  tiny data compression of 2 to 64 bytes based on td64
#ifndef td64_h
#define td64_h

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
//#define NDEBUG // disable asserts
#include <assert.h>

#define MAX_TD64_BYTES 64  // max input vals supported
#define MIN_TD64_BYTES 2  // min input vals supported
#define MAX_UNIQUES 16 // max uniques supported in input
#define MAX_STRING_MODE_UNIQUES 32 // max uniques supported by string mode
#define MIN_VALUES_STRING_MODE 32 // min values to use string mode


int32_t td5(const unsigned char *inVals, unsigned char *outVals, const uint32_t nValues);
int32_t td5d(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed);
int32_t td64(unsigned char *inVals, unsigned char *outVals, const uint32_t nValues, const uint32_t betterCompression);
int32_t td64d(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed);

#endif /* td64_h */
