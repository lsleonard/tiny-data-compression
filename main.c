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

#define BENCHMARK_LOOP_COUNT // // special loop count for benchmarking
//#define TEST_TD512 // invokes test_td512_1to512

int32_t test_td512_1to512(void)
{
    // generate data then run through compress and decompress and compare for 1 to 512 values
    unsigned char textData[512]={"it over afterwards, it occurred to her that she ought to have wondered at this, but at the time it all seemed quite natural); but when the Rabbit actually TOOK A WATCH OUT OF ITS WAISTCOAT- POCKET, and looked at it, and then hurried on, Alice started to her feet, for it flashed across her mind that she had never before seen a rabbit with either a waistcoat-pocket, or a watch to take out of it, and burning with curiosity, she ran across the field after it, and fortunately was just in time to see it positive"};
    unsigned char textOut[515];
    unsigned char textOrig[512];
    uint32_t bytesProcessed;
    int32_t retVal;
    int i;
    int j;
RUN_512:
    for (i=1; i<=512; i++)
    {
        retVal = td512(textData, textOut, i);
        if (retVal < 0)
            return i;
        retVal = td512d(textOut, textOrig, &bytesProcessed);
        if (retVal < 0)
            return -i;
        for (j=0; j<i; j++)
        {
            if (textData[j] != textOrig[j])
                return 1000+j;
            textOut[j] = '0';
            textOrig[j] = '1';
        }
    }
    if (textData[0] == 'i')
    {
        // set all values to 0x83 and run again
        memset(textData, 0x91, sizeof(textData));
        goto RUN_512;
    }
    return 0;
}

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
    printf("tiny data compression td512 v1.1\n");
#ifdef TEST_TD512
    int32_t retVal;
    if ((retVal=test_td512_1to512()) != 0) // do check of 1 to 512 values
    {
        printf("error from test_td512_1to512=%d\n", retVal);
        return -83;
    }
#endif
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
        sscanf(argv[2], "%d", &loopCnt);
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
    printf("   loop count=%d\n", loopCnt);

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
            nCompressedBytes = td512(src+srcBlockOffset, dst+dstBlockOffset, nBlockBytes);
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
    printf("compression=%.02f%%  %.00f bytes per second inbytes=%lu outbytes=%u\n", (float)100*(1.0-((float)totalCompressedBytes/(float)len)), (float)len/(float)timeSpent, len, totalCompressedBytes);

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
