#ifndef img_img
#define img_img

#include <stdio.h>
#include <stdlib.h>

#include "zlib.hpp"

// Image error codes:
// -1 : Failed to open file
// -2 : Most likely not the expected filetype file, possibly corrupted.
// -3 : Appears to have been corrupted.
// -4 : Encountered unrecognized format or requested to perform unsupported actions.
// -5 : Encountered corrupted data (Usually caused by a failed checksum)

namespace img {
	class img {
	public:
		unsigned int width;
		unsigned int height;
		
		// Bytes per pixel, pitch, and size.
		unsigned char bpp;
		unsigned int pitch;
		unsigned int bsize;
		
		bool is_RGB;
		
		bool uses_palette;
		
		// 0: No Alpha, 1: True Alpha, 2: Indexed Alpha, 3: Binary Alpha
		// True alpha provides an alpha channel, Indexed Alpha specifies multiple values/palette entries as alpha, Binary alpha specifies one value/palette entry as alpha.
		unsigned char alpha_mode;
		
		unsigned char bit_depth;
		
		// Palette length is measured in number of entries, each entry may be either 3 channels or 1 channel wide, depending on whether the image is RGB or Grayscale.
		// Only byte-aligned bit depths (8, 16, 24, etc.) are supported for palettes.
		int palette_length;
		unsigned char* palette;
		
		// Raw decompressed image data.
		unsigned char* data;
		
		// Just sets pointers to NULL so the destructor knows not to try and free() them
		img();
		
		// Load a PNG image
		static img* load_png(char* fn, img& im, int verbose, int* errcd);
		
		~img();
	private:
		// Allows chunk names to be detected using 4-byte integer comparison
		enum png_chnk_type : unsigned int {IHDR = 0x49484452, PLTE = 0x504C5445, IDAT = 0x49444154, IEND = 0x49454E44, tEXt = 0x74455874};
		
		// Calculates and returns the 32 bit CRC for a buffer of data as used by the PNG specification
		// datan is the size of the data in bytes.
		static int png_crc(unsigned char* data, int datan);
		
		// Changes the endianness of the passed int.
		inline static void swap(unsigned char* a);
	};
}

#include "img.cpp"

#endif
