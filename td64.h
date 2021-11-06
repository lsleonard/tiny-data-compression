//
//  td64.h
//  tiny data compression of 2 to 64 bytes based on td64
#ifndef td64_h
#define td64_h

#include <stdint.h>

int32_t td5(const unsigned char *inVals, unsigned char *outVals, const uint32_t nValues);
int32_t td5d(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed);
int32_t td64(unsigned char *inVals, unsigned char *outVals, const uint32_t nValues, const uint32_t betterCompression);
int32_t td64d(const unsigned char *inVals, unsigned char *outVals, const uint32_t nOriginalValues, uint32_t *bytesProcessed);

#endif /* td64_h */
