#include <stdio.h>
#include <stdlib.h>

#include "zlib.hpp"

using namespace util;

int main() {
	printf("Hello zlib!\n");
	
	FILE* fi = fopen("compressed.bin", "rb");
	FILE* fo = fopen("decompressed.txt", "wb+");
	
	zlib_stream zs(fi, fo);
	
	zs.inflate(100);
	
	fclose(fi);
	fclose(fo);
	
	huffman_tree ht;
	ht.insert(0x01, 2, 'A');
	ht.insert(0x01, 1, 'B');
	ht.insert(0x00, 3, 'C');
	ht.insert(0x01, 3, 'D');
	
	unsigned char* buf = (unsigned char*) malloc(2);
	
	fi = fopen("decodable.bin", "rb");
	fo = fopen("decoded.txt", "wb+");
	
	zlib_stream zs2(fi, fo);
	
	for (int i = 0; i < 6; i++) {
		printf("%c", zs2.decode_symbol(ht));
	}
	printf("\n");
	
	fclose(fi);
	fclose(fo);
	
	free(buf);
	
	return 0;
}







