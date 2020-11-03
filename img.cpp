#define IMG_IDAT_STREAM_SIZE 8192

namespace img {
	img::img() : palette(NULL), data(NULL) {}
	
	img* img::load_png(char* fn, img& im, int verbose, int* errcd) {
		*errcd = 0;
		
		// Open png file
		FILE* fp = fopen(fn, "rb");
		if (fp == NULL) {
			if (verbose >= 3) printf("Error Loading \"%s\": Failed to open file.\n", fn);
			*errcd = -1; return NULL;
		}
		
		// 4 bytes for lenght, 4 bytes for type, null terminating byte, 4 bytes for CRC
		unsigned char* chnk_meta = (unsigned char*) malloc(13);
		unsigned char* chnk_data;
		
		// Test for signature
		fread(chnk_meta, 1, 8, fp);
		if (chnk_meta[0] != 0x89 || chnk_meta[1] != 0x50 || chnk_meta[2] != 0x4E || chnk_meta[3] != 0x47) {
			if (verbose >= 3) printf("Error While Loading \"%s\": File does not appear to be a PNG file.\n", fn);
			*errcd = -2; return NULL;
		}
		if (chnk_meta[4] != 0x0D || chnk_meta[5] != 0x0A) {
			if (verbose >= 3) printf("Error While Loading \"%s\": This PNG file has likely been corrupted while being transmitted onto a Unix system.\n", fn);
			*errcd = -3; return NULL;
		}
		if (chnk_meta[6] != 0x1A) {
			if (verbose >= 3) printf("Error While Loading \"%s\": This PNG file appears to be corrupted.\n", fn);
			*errcd = -3; return NULL;
		}
		if (chnk_meta[7] != 0x0A) {
			if (verbose >= 3) printf("Error While Loading \"%s\": This PNG file has likely been corrupted while being transmitted onto a Windows/DOS system.\n", fn);
			*errcd = -3; return NULL;
		}
		
		// Begin loading chunks
		unsigned int* len = (unsigned int*) chnk_meta;
		char* type = (char*) chnk_meta+4;
		unsigned char* data;
		unsigned int* crc = (unsigned int*) (chnk_meta+9);
		unsigned int c_crc;
		
		// Set Null terminator in chunk metadata buffer.
		chnk_meta[8] = 0;
		
		// Used to detect EOF
		int read_ret;
		
		// Detect CPU endianness
//		int t = 1;
//		bool isBE = *((char*) &t) == 0;
		
		unsigned char temp;
		
		bool found_IHDR = false;
		bool found_PLTE = false;
		bool critical;
		unsigned char interlacing;
		
		unsigned int idat_start;
		
		bool ret;
		
		// Holds Decompressed data, before it gets filtered and sent to im.data
		unsigned char* buf = (unsigned char*) malloc(IMG_IDAT_STREAM_SIZE);
		
		// Create a zlib_stream for decompressing image data.
		util::zlib_stream idat(fp, fmemopen(buf, IMG_IDAT_STREAM_SIZE, (const char*) "w"));
		
		while (true) {
			// Read length and chunk type
			fread(len, 4, 1, fp);
			fread(type, 1, 4, fp);
			
			// Convert endianness of length
			swap(chnk_meta);
			
			// chnk_data consists of the 4-byte chunk name, the chunk raw data, and a NULL-terminating character at the end in case the data is text data
			chnk_data = (unsigned char*) malloc(*len + 5);
			data = chnk_data + 4;
			
			// Add NULL terminator and type to chnk_data
			data[*len] = 0;
			chnk_data[0] = type[0]; chnk_data[1] = type[1]; chnk_data[2] = type[2]; chnk_data[3] = type[3];
			
			idat_start = ftell(fp);
			
			// Read chunk data content
			if (*len > 0) fread(data, 1, *len, fp);
			
			// Read CRC
			read_ret = fread(crc, 4, 1, fp);
			
			// Read critical bit flag.
			critical = !(type[0] & 0x20);
			
			// Convert endianness of the CRC
			swap(chnk_meta+9);
			
//			printf("Found chunk \"%s\"\n", type);
			
			if (type[0] == 'I' && type[1] == 'E' && type[2] == 'N' && type[3] == 'D') {
				printf("IEND chunk found, exiting.\n");
				free(chnk_data);
				break;
			}
			
			if (read_ret == 0) {
				if (verbose >= 3) printf("Error While Loading \"%s\": Encountered End Of File before finding an IEND chunk.\n", fn);
				*errcd = -3;
				break;
			}
			
			// Calculate and check the CRC
			c_crc = png_crc(chnk_data, *len+4);
			if (*crc != c_crc) {
				if (critical) {
					if (verbose >= 3) printf("Error While Loading \"%s\": CRC Check failed on critical chunk \"%s\".\n", fn, type);
					*errcd = -5;
					return NULL;
				} else {
					if (verbose >= 2) printf("Warning While Loading \"%s\": CRC Check failed on ancillary chunk \"%s\". Skipping chunk.\n", fn, type);
					continue;
				}
			}
			
			// From this point on, the type of a chunk is recognized via 4-byte integer comparison to constants. This is simpler than actual string comparison.
			// The constants are defined in the img.hpp header file.
			swap((unsigned char*) type);
			
			if (!found_IHDR) {
				if (*((unsigned int*) type) == IHDR) {
					found_IHDR = true;
					
					swap(data);
					swap(data+4);
					
					im.width = *((unsigned int*) data);
					im.height = *((unsigned int*) (data+4));
					
					im.bit_depth = data[8];
					
					switch (data[9]) {
						case 0:
							im.is_RGB = false;
							im.uses_palette = false;
							im.alpha_mode = 0;
							break;
						case 2:
							im.is_RGB = true;
							im.uses_palette = false;
							im.alpha_mode = 0;
							break;
						case 3:
							im.is_RGB = true;
							im.uses_palette = true;
							im.alpha_mode = 0;
							break;
						case 4:
							im.is_RGB = false;
							im.uses_palette = false;
							im.alpha_mode = 1;
							break;
						case 6:
							im.is_RGB = true;
							im.uses_palette = false;
							im.alpha_mode = 1;
					}
					
					if (data[10] != 0) {
						if (verbose >= 3) printf("Error While Loading \"%s\": PNG Header requests the use of an unsupported compression method.\n", fn);
						*errcd = -4;
						return NULL;
					}
					
					if (data[11] != 0) {
						if (verbose >= 3) printf("Error While Loading \"%s\": PNG Header requests the use of an unsupported filtering method.\n", fn);
						*errcd = -4;
						return NULL;
					}
					
					interlacing = data[12];
					if (interlacing > 1) {
						if (verbose >= 3) printf("Error While Loading \"%s\": PNG Header requests the use of an unsupported interlacing method.\n", fn);
						*errcd = -4;
						return NULL;
					}
					else if (interlacing == 1) {
						if (verbose >= 3) printf("Error While Loading \"%s\": PNG Header requests the use of an unsupported interlacing method Adam-7.\n", fn);
						*errcd = -4;
						return NULL;
					}
					
					// Allocate space for output stream.
					if (im.is_RGB) {
						im.bpp = 3;
					} else {
						im.bpp = 1;
					}
					if (im.alpha_mode) {
						im.bpp++;
					}
					
					im.pitch = im.bpp*im.width;
					im.bsize = im.pitch*im.height;
					im.data = (unsigned char*) malloc(im.bsize);
					
				} else {
					if (verbose >= 3) printf("Error While Loading \"%s\": File is missing an IHDR chunk.\n", fn);
					*errcd = -5;
					return NULL;
				}
			}
				
			if (*((unsigned int*) type) == IDAT) {
				fseek(fp, idat_start, SEEK_SET);
				ret = idat.inflate(*len);
				if (!ret) {
					if (verbose >= 3) printf("Error While Loading \"%s\": Invalid zlib stream.\n", fn);
					*errcd = -5;
					return NULL;
				}
			}
			else if (*((unsigned int*) type) == PLTE) {
				found_PLTE = true;
				
				if (*len % 3 != 0) {
					if (verbose >= 3) printf("Error While Loading \"%s\": PNG PLTE chunk is of an invalid length.\n", fn);
					*errcd = -5;
					return NULL;
				}
				
				im.palette_length = *len / 3;
				im.palette = (unsigned char*) malloc(*len);
				for (int i = 0; i < *len; i++) {
					im.palette[i] = data[i];
				}
			}
			else if (*((unsigned int*) type) == tEXt) {
				char* txtdata;
				for (txtdata = (char*) data; *txtdata != '\0'; txtdata++); txtdata++;
				if (verbose >= 0) printf("Note While Loading %s: %s, %s\n", fn, data, txtdata);
			}
			
			free(chnk_data);
		}
		
		fclose(fp);
		idat.close_out();
		free(chnk_meta);
		free(buf);
		
		return NULL;
	}
	
	img::~img() {
		if (palette != NULL) {
			free(palette);
		}
		if (data != NULL) {
			free(data);
		}
	}
	
	// I can't say I really understand this function fully. But I do know that it gave me too much grief to be worth looking into further.
	// Turns out the bit shift operator is undefined for negative integers, so crc has to be unsigned. Who could've guessed? Also, remember to use "%08x" for hexadecimal, not just "%x"
	int img::png_crc(unsigned char* data, int datan) {
		// Build CRC table
		unsigned int* crc_table = (unsigned int*) malloc(256 * sizeof(unsigned int));
		for (int n = 0; n < 256; n++) {
			unsigned int c = (unsigned int) n;
			for (int k = 0; k < 8; k++) {
				if (c & 1) {
					c = 0xEDB88320 ^ (c >> 1);
				} else {
					c = c >> 1;
				}
			}
			crc_table[n] = c;
		}
		
		unsigned int crc = 0xFFFFFFFF;
		
		for (int i = 0; i < datan; i++) {
			crc = crc_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
		}
		
		free(crc_table);
		
		return crc ^ 0xFFFFFFFF;
	}
	
	void img::swap(unsigned char* a) {
		unsigned char temp;
		temp = a[0]; a[0] = a[3]; a[3] = temp;
		temp = a[1]; a[1] = a[2]; a[2] = temp;
	}
}




















