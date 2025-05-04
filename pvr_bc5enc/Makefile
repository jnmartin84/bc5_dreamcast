
# Makefile stolen from the kmgenc program and then the dcbumpgen program.

CFLAGS = -g -Og -Wall -DINLINE=inline -I/usr/local/include
LDFLAGS = -s -lpng -ljpeg -lm -lz -L/usr/local/lib

all: pvr_bc5enc

pvr_bc5enc: pvr_bc5enc.o bc5_encoder.o get_image.o get_image_jpg.o get_image_png.o readpng.o
	$(CC) -o $@ $+ $(LDFLAGS)

clean:
	rm -f pvr_bc5enc *.o
