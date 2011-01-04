/**
 * Note: this PNG manipulation code is mostly based off of examples from 
 * Greg Roelofs (http://www.vias.org/pngguide/chapter13_01.html)
 */
#ifndef __PNGUTIL_H__
#define __PNGUTIL_H__

#include <stdio.h>
#include <stdint.h>

#include <png.h>

struct png_container {
  png_structp png;
  png_infop   info;
  uint32_t    width;
  uint32_t    height;
  int         bit_depth;
  int         color_type;
  uint8_t*    bytes;
};

typedef struct png_container* png_containerp;

/**
 * Returns 0 on success, -1 on error
 */
extern int readpng_init(FILE *infile, png_containerp ctx);

/**
 * Returns 0 on success, -1 on error 
 */
extern int readpng_get_image(png_containerp ctx, uint32_t *pChannels, uint32_t *pRowBytes);

extern void readpng_free(png_containerp ctx);

#endif /* __PNGUTIL_H__ */
