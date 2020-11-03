/* https://pyokagan.name/blog/2019-10-18-zlibinflate/ */

#ifndef util_zlib
#define util_zlib

#include <stdio.h>

namespace util {
	// This class allows the reading of individual bits and bytes from a file stream.
	class binp_stream {
	public:
		binp_stream();
		binp_stream(FILE* in);
		
		FILE* in;
		
		// Whether EOF has been found.
		bool eof;
		
		// Read this many bits from stream.
		// read_1() Reads a bit from the currently stored byte, reading a new byte when necessary. If the current byte contains unread bits, read_8/16/32 will discard those unread bits.
		unsigned char  read_1();
		unsigned char  read_8();
		unsigned short read_16();
		unsigned int   read_32();
		
		// Read n bits. If n>32, n-32 bits are discarded, and the return value will contain only the left-most 32 bits read in the stream.
		unsigned int read_bits(unsigned char n);
		
	private:
		unsigned char b;
		unsigned char numbits;
	};
	
	// This class allows the writing of individual bits and bytes to a file stream.
	class bout_stream {
	public:
		bout_stream();
		bout_stream(FILE* out);
		
		FILE* out;
		
		// Write this many bits to stream.
		// write_1() writes the least significant bit of "in" to the next open bit of an internally stored byte, and writes that whole byte to the output stream when it is filled.
		// write_8/16/32 will write the currently stored byte to stream if it has been written to, with 0s in bits that have not been written to, before writing the given byte(s) to stream.
		// Thus, data written to stream with write_8/16/32 will always start on byte boundaries.
		void write_1( unsigned char  in);
		void write_8( unsigned char  in);
		void write_16(unsigned short in);
		void write_32(unsigned int   in);
		
		// Write n bits to stream.
		void write_bits(unsigned int in, unsigned char n);
		
	private:
		unsigned char b;
		unsigned char numbits;
	};
	
	// This class is used in the huffman_tree class
	class huffman_node {
	public:
		huffman_node();
		
		unsigned char symbol;
		
		huffman_node* left;
		huffman_node* right;
		
		~huffman_node();
	};
	
	// This class allows Huffman trees to be represented in memory, required for zlib implementation.
	class huffman_tree {
	public:
		huffman_tree();
		
		huffman_node root;
		
		// Inserts a codeword
		bool insert(unsigned short codeword, unsigned char n, unsigned char symbol);
	};
	
	// This class keeps track of the internal state of a zlib stream, allowing the user to decode or encode streams as they become available.
	// deflate() and inflate() will continue encoding the stream from wherever the cursor happens to be and from wherever they left off last time they were called.
	class zlib_stream {
	public:
		zlib_stream();
		zlib_stream(FILE* in, FILE* out);
		
		void set_in(FILE* in);
		void set_out(FILE* out);
		
		bool deflate(unsigned int bytes);
		bool inflate(unsigned int bytes);
		
		// Close streams.
		void close_in();
		void close_out();
		
unsigned char decode_symbol(const huffman_tree& ht);
		
	private:
		// Input and output bit streams
		binp_stream in;
		bout_stream out;
		
		// zlib stream header
		unsigned short zhead;
		
		unsigned char CM;
		unsigned char CINFO;
		unsigned char FDICT;
		unsigned char FLEVEL;
		
		// deflate block header
		// BTYPE = 3 means a block header is expected.
		bool BFINAL;
		unsigned char BTYPE;
		
		// Functions to handle individual deflate blocks.
		// Functions will decode up to "bytes" bytes, or until they reach the end of the block, or until EOF.
		bool inflate_block_none(unsigned int bytes);
		bool inflate_block_fixed(unsigned int bytes);
		bool inflate_block_dynamic(unsigned int bytes);
		
		// reads the next huffman code and returns the associated symbol
//		unsigned char decode_symbol(const huffman_tree& ht);
	};
}

#include "zlib.cpp"
#endif
