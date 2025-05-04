#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>

#include "get_image.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <assert.h>

extern void split_pairs(const uint8_t* bytes, size_t length, uint8_t** pitch_bytes, uint8_t** yaw_bytes);
extern void gen_palette_and_indexes(int start_x, int start_y, int stride, const uint8_t* bytes, uint8_t* palette, uint8_t* indexes);
extern void block_encode(int start_x, int start_y, int stride, const uint8_t* r_bytes, const uint8_t* g_bytes,
                  uint8_t* r_palette, uint8_t* r_indexes, uint8_t* g_palette, uint8_t* g_indexes);
extern void pack_block(const uint8_t* rpal, const uint8_t* ridxs, const uint8_t* gpal, const uint8_t* gidxs, uint8_t* result);
extern uint8_t* compress_normalmap(const uint8_t* norm_bytes, int width, int height);
void encode_hemi_data(uint8_t *raw_xyz_data, int w, int h, uint8_t *raw_hemi_data);

#define ERROR(...) { fprintf(stderr, __VA_ARGS__); goto cleanup; }

static void printUsage(void)
{
	printf("pvr_bc5enc - Dreamcast hemisphere normal map compressor v1.0\n");
	printf("Copyright (c) 2024, 2025 jnmartin84, erik5249\n");
	printf("usage:\n");
	printf("\tpvr_bc5enc <infile.png/.jpg> <outfile.comp>\n");
	printf("Outputs:\n\tbc5-compressed polar-encoded normal map to <outfile.comp>.\n");
}

static int isPowerOfTwo(unsigned x)
{
	return x && !(x & (x-1));
}

int main(int argc, char **argv)
{
	uint8_t *raw_hemi_data = NULL;

	FILE *fp = NULL;
	image_t img = {0};

	if (argc < 2) {
		printUsage();
		return 0;
	}

	if (get_image(argv[1], &img) < 0) {
		ERROR("Cannot open %s\n", argv[1]);
	}

	char *cfn;
	if (argc == 3) {
		cfn = argv[2];
	} else {
		printUsage();
		ERROR("\nNo compressed output filename provided!\n");
	}

	raw_hemi_data = malloc(2 * img.w * img.h);
	if (NULL == raw_hemi_data) {
		ERROR("Cannot allocate memory for image data!\n");
	}

	if (!isPowerOfTwo(img.w) || !isPowerOfTwo(img.h)) {
		ERROR("Image dimensions %ux%u are not a power of two!\n", img.w, img.h);
	}

	encode_hemi_data(img.data, img.w, img.h, raw_hemi_data);

	uint8_t *compd_img = compress_normalmap(raw_hemi_data, img.w, img.h);
	FILE *cfp = fopen(cfn, "wb");
	if (NULL == cfp) {
		ERROR("Cannot open file for compressed normal map.\n");
	}
	fwrite(compd_img, img.w*img.h, 1, cfp);
	fclose(cfp);

cleanup:
	if (fp) fclose(fp);
	if (raw_hemi_data) free(raw_hemi_data);
	if (img.data) free(img.data);

	return 0;
}

#define rescale(x) ((((double)(x) / 255.0) * 2.0) - 1.0)
#define zrescale(x) ((double)(x) / 255.0)

void encode_hemi_data(uint8_t *raw_xyz_data, int w, int h, uint8_t *raw_hemi_data)
{
	double x, y, z;
	double a, e;
	int outa, oute;
	double lenxy2;

	int dest = 0;
	int source = 1;
	int ih;
	int iw;
	for (ih = 0; ih < h; ih++) {
		for (iw = 0; iw < w; iw++, source += 4) {
			x = rescale(raw_xyz_data[source    ]);
			y = rescale(raw_xyz_data[source + 1]);
			z = rescale(raw_xyz_data[source + 2]);

			// flip y
			y = -y;

			lenxy2 = sqrt((x*x) + (y*y));

			// compute the azimuth angle
			a = atan2( y , x );

			// compute the elevation angle
			e = atan2( lenxy2 , z );

			// a 0 - 2PI
			// e 0 - PI/2

			while (a < 0.0) {
				a += (M_PI * 2.0);
			}

			if (e < 0.0) e = 0.0;
			if (e > (M_PI / 2.0)) e = M_PI / 2.0;

			outa = (int)(255 * (a / (M_PI * 2.0)));
			oute = (int)(255 * (1.0 - (e / (M_PI / 2.0))));

			raw_hemi_data[dest] = outa; dest += 1;
			raw_hemi_data[dest] = oute; dest += 1;
		}
	}
}
