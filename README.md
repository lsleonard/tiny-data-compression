# tiny-data-compression
High-speed lossless tiny data compression of 1 to 512 bytes

td512 filename [loopCount]
   
	filename is the name of the file to compress. The compressed output is written to filename.td512 and the decompressed output to filename.td512d.
	
	loopCount (default 1) is the loop count to use for performance testing. Also see BENCHMARK_LOOP_COUNT macro in main.c.

Tiny data compression is not supported by standard compression programs. Now with td512 you can reasonably compress data from 16 to 512 bytes. td512 is available under the GPL-3.0 License at https://github.com/lsleonard/tiny-data-compression. Compared with QuickLZ, a fast compression program that is designed to compress smaller data sets, td512 gets as good or better compression for 512-byte blocks of most data types. QuickLZ steadily declines in compression ratio as the number of bytes decreases to 128, and at 64 bytes, produces no compression. td512 has good compression at 64 bytes with the td64 interface. td512 combines extended text and string modes for 128 to 512 bytes with the td64 interface to compress any remaining bytes in the input. The td512 algorithm emphasizes speed, and based on data in this paper, gets 31.6% average compression for 512-byte blocks at 250 Mbytes per second on the Squash benchmark test data (see https://quixdb.github.io/squash-benchmark/#) running on a 2 GHz quad-core processor. For 64-byte blocks on this benchmark data, td512 gets 25.3% average compression at 250 MBytes per second.

You can call the td512 and td512d functions to compress and decompress 1 to 512 bytes. The td512 interface performs compression of 16 to 512 bytes, but accepts 1 to 15 bytes and stores them without compression. Along with its extended text and string modes, td512 acts as a wrapper that uses the td64 interface to compress blocks of 64 bytes until the final block of 64 or fewer bytes is compressed. Along with the number of bytes processed, a pass/fail bit is stored for each block compressed, and the compressed or uncompressed data is output.

With td64, you can call the td5 and td5d functions to compress and decompress 1 to 5 values. This interface is not used by td512 because the number of bytes generated is often more than the number of values to compress. Or you can call td64 and td64d functions to compress and decompress 6 to 64 values. The td64 interface returns pass (number of compressed bits) or fail (0) and outputs only compressed values. Decompression requires input of the number of original values.

For more information, see Tiny Data Compression with td512.docx.
