// td512.h

#ifndef td512_h
#define td512_h

#include <unistd.h>

int32_t td512(unsigned char *inVals, unsigned char *outVals, const uint32_t nValues, const uint32_t betterCompression);
int32_t td512d(unsigned char *inVals, unsigned char *outVals, uint32_t *totalBytesProcessed);

#endif /* td512_h */
