namespace util {
	/* binp_stream */
	
	binp_stream::binp_stream() : in(NULL), b(0), numbits(0), eof(false) {}
	binp_stream::binp_stream(FILE* in) : in(in), b(0), numbits(0), eof(false) {}
	
	unsigned char binp_stream::read_1() {
		// If there are no more bits in the current byte, get the next byte.
		if (numbits == 0) {
			if (!fread(&b, 1, 1, in)) {
				eof = true;
				return 0;
			}
			numbits = 8;
		}
		
		// Get the least significant bit and shift b out.
		numbits--;
		unsigned char bit = b & 0x80;
		b <<= 1;
		return bit;
	}
	
	unsigned char binp_stream::read_8() {
		// Discard unread bits
		numbits = 0;
		// Read value
		if (!fread(&b, 1, 1, in)) {
			eof = true;
			return 0;
		}
		return b;
	}
	
	unsigned short binp_stream::read_16() {
		// Discard unread bits
		numbits = 0;
		// Read value
		unsigned short out;
		if (!fread(&out, 2, 1, in)) {
			eof = true;
			return 0;
		}
		return out;
	}
	
	unsigned int binp_stream::read_32() {
		// Discard unread bits
		numbits = 0;
		// Read value
		unsigned int out;
		if (!fread(&out, 4, 1, in)) {
			eof = true;
			return 0;
		}
		return out;
	}
	
	unsigned int binp_stream::read_bits(unsigned char n) {
		unsigned int o = 0;
		for (int i = 0; i < n; i++) {
			o |= read_1() << i;
			if (eof) {
				return 0;
			}
		}
		return o;
	}
	
	/* bout_stream */
	
	bout_stream::bout_stream() : out(NULL), b(0), numbits(0) {}
	bout_stream::bout_stream(FILE* out) : out(out), b(0), numbits(0) {}
	
	void bout_stream::write_8(unsigned char n) {
		fwrite(&n, 1, 1, out);
	}
	
	/* huffman_node */
	huffman_node::huffman_node() : symbol('\0'), left(NULL), right(NULL) {}
	
	huffman_node::~huffman_node() {
		if (left != NULL) {
			delete left;
		}
		if (right != NULL) {
			delete right;
		}
	}
	
	/* huffman_tree */
	huffman_tree::huffman_tree() : root() {}
	
	// This is the structure that made me switch from malloc() to new. The function originally malloc'd space and called the huffman_node constructor in that space.
	// When it went out of scope, the destructor would be called (having no effect at all, since the left and right fields would still both be NULL, and of course the space remained malloc'd.)
	// This had no obvious glitches. The program would perform normally as the compiler still allowed me to get/set the fields of a destructed object.
	// This is presumably because the C++ devs wanted people to still be able to build objects in memory by hand, without constructors, the way you'd build a struct.
	// However what it did mean is that when the memory was freed / the program ended, since the memory contained no constructed objects, their destructors would not be called.
	// This caused memory leaks.
	// In other words, if you want to build objects by hand, you need to destroy them by hand, which kind of defeats the whole point. So, use new. It doesn't just save you a sizeof(), turns out.
	bool huffman_tree::insert(unsigned short codeword, unsigned char n, unsigned char symbol) {
		unsigned char b;
		
		huffman_node* node = &root;
		huffman_node* next_node;
		for (int i = n-1; i >= 0; i--) {
			b = codeword & (1 << i);
			if (b) {
				next_node = node->right;
				if (!next_node) {
					node->right = new huffman_node();
					next_node = node->right;
				}
			} else {
				next_node = node->left;
				if (!next_node) {
					node->left = new huffman_node();
					next_node = node->left;
				}
			}
			node = next_node;
		}
		node->symbol = symbol;
		
		return true;
	}
	
	/* zlib_stream */
	
	zlib_stream::zlib_stream() : in(), out(), zhead(0), BTYPE(3), BFINAL(1) {}
	zlib_stream::zlib_stream(FILE* in, FILE* out) : in(in), out(out), zhead(0), BTYPE(3), BFINAL(1) {}
	
	void zlib_stream::set_in(FILE* fp) {in.in = fp;}
	void zlib_stream::set_out(FILE* fp) {out.out = fp;}
	
	bool zlib_stream::deflate(unsigned int bytes) {printf("Deflate not yet implemented.\n");}
	
	bool zlib_stream::inflate(unsigned int bytes) {
		// Read zlib stream header
		if (zhead == 0) {
			zhead = in.read_8() << 8 | in.read_8();
			
			CM = (zhead & 0x0F00) >> 8;
			CINFO = (zhead & 0xF000) >> 12;
			FDICT = (zhead & 0x0020) >> 5;
			FLEVEL = (zhead & 0x0070) >> 6;
			
			// Check for invalid zhead values
			if (CINFO > 7 || CM != 8 || zhead % 31 != 0 || FDICT) return false;
			
			unsigned short window_size = 1 << (CINFO + 8);
		}
		
		unsigned int bytes_left = bytes;
		
		// Decompress stream
		while (!BFINAL & !in.eof) {
			// Read deflate block header
			BFINAL = in.read_1();
			BTYPE = in.read_bits(2);
			
			if (BTYPE == 0) {
				inflate_block_none(bytes);
			}
			else if (BTYPE == 1) {
				inflate_block_fixed(bytes);
			}
			else if (BTYPE == 2) {
				inflate_block_dynamic(bytes);
			}
			else {
				return false;
			}
		}
		
		return true;
	}
	
	void zlib_stream::close_in() {
		fclose(in.in);
	}
	
	void zlib_stream::close_out() {
		fclose(out.out);
	}
	
	bool zlib_stream::inflate_block_none(unsigned int bytes) {
		unsigned short len = in.read_16();
		unsigned short nlen = in.read_16();
		
		unsigned int a = 1;
		unsigned int b = 0;
		
		unsigned char c;
		
		// Copy uncompressed data to output stream
		for (int i = 0; i < len; i++) {
			c = in.read_8();
			a = (a+c) % 65521;
			b = (b+a) % 65521;
			out.write_8(c);
		}
		
		unsigned int adler32_c = (a << 16) | b;
		unsigned int adler32_r = (in.read_8() << 8) | in.read_8() | (in.read_8() << 24) | (in.read_8() << 16);
		
		if (adler32_c != adler32_r) return false;
		
		return true;
	}
	
	bool zlib_stream::inflate_block_fixed(unsigned int bytes) {}
	
	bool zlib_stream::inflate_block_dynamic(unsigned int bytes) {}
	
	unsigned char zlib_stream::decode_symbol(const huffman_tree& ht) {
		unsigned char b;
		
		const huffman_node* node = &ht.root;
		while (node->left || node->right) {
			b = in.read_1();
			if (b) {
				node = node->right;
			} else {
				node = node->left;
			}
		}
		return node->symbol;
	}
}











