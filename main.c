//
//  main.c
//  tiny data compression for 1 to 512 bytes based on td512
//  version 1.1
//
//  file-based test bed outputs .td512 file with encoded values
//  then reads in that file and generates .td512d file with
//  original values.
//
//  Created by Stevan Leonard on 10/30/21.
//  Copyright © 2021 Oxford House Software. All rights reserved.
//
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
#include "td512.h" // td512 functions

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

//#define BENCHMARK_LOOP_COUNT // // special loop count for benchmarking

int main(int argc, char* argv[])
{
    FILE *ifile, *ofile;
    char ofileName[256];
    unsigned char *src, *dst;
    size_t len, len2, len3;
    clock_t begin, end;
    double timeSpent;
    double minTimeSpent=600;
    int32_t nBytesRemaining;
    int32_t nCompressedBytes;
    uint32_t totalCompressedBytes;
    uint32_t bytesProcessed;
    uint32_t totalOutBytes;
    uint32_t srcBlockOffset;
    uint32_t dstBlockOffset;
    int loopNum;
    int loopCnt; // argv[4] option: default is 1
    uint32_t betterCompression=0; // argv[3] option: default is fastest execution

    printf("tiny data compression td512 v1.1\n");
    if (argc < 2)
    {
        printf("td512 error: input file must be specified\n");
        return 14;
    }
    ifile = fopen(argv[1], "rb");
    if (!ifile)
    {
        printf("td512 error: file not found: %s\n", argv[1]);
        return 9;
    }
    printf("   file=%s\n", argv[1]);
    strcpy(ofileName, argv[1]);
    ofile = fopen(strcat(ofileName, ".td512"), "wb");

    // allocate source buffer and read file
    fseek(ifile, 0, SEEK_END);
    len = (size_t)ftell(ifile);
    fseek(ifile, 0, SEEK_SET);
    src = (unsigned char*) malloc(len);
    fread(src, 1, len, ifile);
    fclose(ifile);
    
    // allocate "uncompressed size" + 3 bytes per block for the destination buffer
    dst = (unsigned char*) malloc(len + 3 * (len / 512 + 1));
    if (argc >= 3)
    {
        sscanf(argv[2], "%d", &betterCompression);
        if (betterCompression == 0)
            printf("    better compression is disabled");
        else
        {
            printf("    better compression is enabled");
            betterCompression = 1; // can be only 0 or 1
        }
    }
    if (argc >= 4)
    {
        sscanf(argv[3], "%d", &loopCnt);
        if (loopCnt < 1 || loopCnt > 1000)
            loopCnt = 1;
    }
    else
    {
        loopCnt = 1;
    }
#ifdef BENCHMARK_LOOP_COUNT // // special loop count for benchmarking
    loopCnt = 20000000 / len;
    loopCnt = (loopCnt < 10) ? 10 : loopCnt;
    loopCnt = (loopCnt > 1000) ? 1000 : loopCnt;
#endif
    loopNum = 0;
    printf("  loop count=%d\n", loopCnt);

COMPRESS_LOOP:
    nBytesRemaining=(int32_t)len;
    nCompressedBytes=0;
    totalCompressedBytes=0;
    srcBlockOffset=0;
    dstBlockOffset=0;

    // compress and write result
    begin = clock();
    while (nBytesRemaining > 0)
    {
        uint32_t nBlockBytes=(uint32_t)nBytesRemaining>=512 ? 512 : (uint32_t)nBytesRemaining;
            nCompressedBytes = td512(src+srcBlockOffset, dst+dstBlockOffset, nBlockBytes, betterCompression);
        if (nCompressedBytes < 0)
            exit(nCompressedBytes); // error occurred
        nBytesRemaining -= nBlockBytes;
        totalCompressedBytes += (uint32_t)nCompressedBytes;
        srcBlockOffset += nBlockBytes;
        dstBlockOffset += (uint32_t)nCompressedBytes;
    }
    end = clock();
    timeSpent = (double)(end - begin) / (double)CLOCKS_PER_SEC;
    if (timeSpent < minTimeSpent)
        minTimeSpent = timeSpent;
    if (++loopNum < loopCnt)
    {
        usleep(10); // sleep 10 us
        goto COMPRESS_LOOP;
    }
    timeSpent = minTimeSpent;
    printf("compression=%.02f%%, %.00f bytes per second inbytes=%lu outbytes=%u\n", (float)100*(1.0-((float)totalCompressedBytes/(float)len)), (float)len/(float)timeSpent, len, totalCompressedBytes);

    fwrite(dst, totalCompressedBytes, 1, ofile);
    fclose(ofile);
    free(src);
    free(dst);
    
    // decompress
    ifile = fopen(ofileName, "rb");
    strcpy(ofileName, argv[1]);
    ofile = fopen(strcat(ofileName, ".td512d"), "wb");

    // allocate source buffer
    fseek(ifile, 0, SEEK_END);
    len3 = (size_t)ftell(ifile);
    fseek(ifile, 0, SEEK_SET);
    src = (unsigned char*) malloc(len3);
    
    // read file and allocate destination buffer
    fread(src, 1, len3, ifile);
    len2 = len; // output==input
    dst = (unsigned char*) malloc(len2);
    
    minTimeSpent=600;
    loopNum = 0;
DECOMPRESS_LOOP:
    // decompress and write result
    totalOutBytes = 0;
    nBytesRemaining = (int32_t)len3;
    srcBlockOffset = 0;
    dstBlockOffset = 0;
    begin = clock();
    while (nBytesRemaining > 0)
    {
        int32_t nRetBytes;
        nRetBytes = td512d(src+srcBlockOffset, dst+dstBlockOffset, &bytesProcessed);
        if (nRetBytes < 0)
            return nRetBytes;
        nBytesRemaining -= bytesProcessed;
        totalOutBytes += (uint32_t)nRetBytes;
        srcBlockOffset += bytesProcessed;
        dstBlockOffset += (uint32_t)nRetBytes;
    }
    end = clock();
    timeSpent = (double)(end - begin) / (double)CLOCKS_PER_SEC;
    if (timeSpent < minTimeSpent)
        minTimeSpent = timeSpent;
    if (++loopNum < loopCnt)
    {
        usleep(10); // sleep 10 us
        goto DECOMPRESS_LOOP;
    }
    timeSpent = minTimeSpent;
    printf("decompression=%.00f bytes per second inbytes=%lu outbytes=%lu\n", (float)len/(float)timeSpent, len3, len);
    fwrite(dst, len, 1, ofile);
    fclose(ifile);
    fclose(ofile);
    return 0;
}
