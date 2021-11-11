# tiny-data-compression
High-speed lossless tiny data compression of 1 to 512 bytes

td512 filename [loopCount]
   
	filename is the name of the file to compress. The compressed output is written to filename.td512 and the decompressed output to filename.td512d.
	
	loopCount (default 1) is the loop count to use for performance testing. Also see BENCHMARK_LOOP_COUNT macro in main.c.

Tiny data compression is not usually supported by compression programs. Now with td512 you can compress data from 6 to 512 bytes. Although the algorithm emphasizes speed, the minimum compression supported within each 64-byte block is 10%, with a goal of 25% or greater when possible. Although Huffman coding, with its optimal compression using frequency analysis of values, has been used effectively for many applications, for small datasets the compression modes used in td512 approach or exceed the results of using the Huffman algorithm. And with a focus on speed of execution, Huffman and arithmetic coding are not practical algorithms for applications of tiny data. Two areas where high-speed compression using td512 might be applied are small message text and programmatic objects.

You can call the td512 and td512d functions to compress and decompress 1 to 512 bytes. The td512 interface performs compression of 6 to 512 bytes, but accepts 1 to 5 bytes and stores them without compression. td512 acts as a wrapper that uses the td64 interface to compress blocks of 64 bytes until the final block of 64 or fewer bytes is compressed. Along with the number of bytes processed, a pass/fail bit is stored for each 64-byte (or smaller) block compressed, and the compressed or uncompressed data is output.

With td64, you can call the td5 and td5d functions to compress and decompress 2 to 5 values. This interface is not used by td512 because the number of bytes generated is often more than the number of values to compress. Or you can call td64 and td64d functions to compress and decompress 6 to 64 values. The td64 interface returns pass (number of compressed bits) or fail (0) and outputs only compressed values. Decompression requires input of the number of original values.
