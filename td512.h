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
// Notes for version 1.1:
/*  1. Main program reads a file into memory that is compressed by
    calling td512 repeatedly. When complete, the compressed data is
    written to a file and read for decompression by calling td512d.
      td512 filename [loopCount]]
        filename is required argument 1.
        loopCount is optional argument 2 (default: 1). Looping is performed over the entire input file.
 

*/

#ifndef td512_h
#define td512_h

#include <unistd.h>

int32_t td512(unsigned char *inVals, unsigned char *outVals, const uint32_t nValues);
int32_t td512d(unsigned char *inVals, unsigned char *outVals, uint32_t *totalBytesProcessed);

#endif /* td512_h */
