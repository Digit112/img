#include <stdio.h>

#include "img.hpp"

int main() {
	int errcd;
	
	img::img im;
	img::img::load_png((char*) "fish.png", im, 3, &errcd);
	printf("Done! errcd: %d\n", errcd);
	
	printf("size: %dx%d, depth: %d, type: %d / %d / %d\n", im.width, im.height, im.bit_depth, im.is_RGB, im.uses_palette, im.alpha_mode);
	return 0;
}
