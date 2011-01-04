/**
 * Note: this PNG manipulation code is mostly based off of examples from 
 * Greg Roelofs (http://www.vias.org/pngguide/chapter13_01.html)
 */
#include "pngutil.h"

#include <stdio.h>
#include <stdlib.h>

int readpng_init(FILE *infile, png_containerp ctx) {

  uint8_t sig[8];

  /* first do a quick check that the file really is a PNG image; could
    * have used slightly more general png_sig_cmp() function instead */

  fread(sig, 1, 8, infile);
  if (!png_check_sig(sig, 8))
    return -1;   /* bad signature */


  /* could pass pointers to user-defined error handlers instead of NULLs: */

  ctx->png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!ctx->png)
    return -1;   /* out of memory */

  ctx->info = png_create_info_struct(ctx->png);
  if (!ctx->info) {
    png_destroy_read_struct(&ctx->png, NULL, NULL);
    return -1;   /* out of memory */
  }


  /* we could create a second info struct here (end_info), but it's only
    * useful if we want to keep pre- and post-IDAT chunk info separated
    * (mainly for PNG-aware image editors and converters) */


  /* setjmp() must be called in every function that calls a PNG-reading
    * libpng function */

  if (setjmp(png_jmpbuf(ctx->png))) {
    png_destroy_read_struct(&ctx->png, &ctx->info, NULL);
    return -1;
  }


  png_init_io(ctx->png, infile);
  png_set_sig_bytes(ctx->png, 8);  /* we already read the 8 signature bytes */

  png_read_info(ctx->png, ctx->info);  /* read all PNG info up to image data */


  /* alternatively, could make separate calls to png_get_image_width(),
    * etc., but want bit_depth and color_type for later [don't care about
    * compression_type and filter_type => NULLs] */

  png_get_IHDR(ctx->png, ctx->info, &ctx->width, &ctx->height, &ctx->bit_depth, &ctx->color_type,
    NULL, NULL, NULL);

  /* OK, that's all we need for now; return happy */

  return 0;
}

int readpng_get_image(png_containerp ctx, uint32_t *pChannels, uint32_t *pRowBytes) {
  double display_exponent = 2.2; // sane default
  double  gamma;
  png_uint_32  i, rowbytes;
  png_bytepp  row_pointers = NULL;


  /* setjmp() must be called in every function that calls a PNG-reading
    * libpng function */

  if (setjmp(png_jmpbuf(ctx->png))) {
    png_destroy_read_struct(&ctx->png, &ctx->info, NULL);
    return -1;
  }


  /* expand palette images to RGB, low-bit-depth grayscale images to 8 bits,
    * transparency chunks to full alpha channel; strip 16-bit-per-sample
    * images to 8 bits per sample; and convert grayscale to RGB[A] */

  if (ctx->color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_expand(ctx->png);
  if (ctx->color_type == PNG_COLOR_TYPE_GRAY && ctx->bit_depth < 8)
    png_set_expand(ctx->png);
  if (png_get_valid(ctx->png, ctx->info, PNG_INFO_tRNS))
    png_set_expand(ctx->png);
  if (ctx->bit_depth == 16)
    png_set_strip_16(ctx->png);
  if (ctx->color_type == PNG_COLOR_TYPE_GRAY ||
    ctx->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(ctx->png);


  /* unlike the example in the libpng documentation, we have *no* idea where
    * this file may have come from--so if it doesn't have a file gamma, don't
    * do any correction ("do no harm") */

  if (png_get_gAMA(ctx->png, ctx->info, &gamma))
    png_set_gamma(ctx->png, display_exponent, gamma);


  /* all transformations have been registered; now update ctx->info data,
    * get rowbytes and channels, and allocate image memory */

  png_read_update_info(ctx->png, ctx->info);

  *pRowBytes = rowbytes = png_get_rowbytes(ctx->png, ctx->info);
  *pChannels = png_get_channels(ctx->png, ctx->info);

  if ((ctx->bytes = (uint8_t*) malloc(rowbytes*ctx->height)) == NULL) {
    png_destroy_read_struct(&ctx->png, &ctx->info, NULL);
    return -1;
  }
  if ((row_pointers = (png_bytepp)malloc(ctx->height*sizeof(png_bytep))) == NULL) {
    png_destroy_read_struct(&ctx->png, &ctx->info, NULL);
    free(ctx->bytes);
    ctx->bytes = NULL;
    return -1;
  }

  /* set the individual row_pointers to point at the correct offsets */

  for (i = 0;  i < ctx->height;  ++i)
    row_pointers[i] = ctx->bytes + i*rowbytes;


  /* now we can go ahead and just read the whole image */

  png_read_image(ctx->png, row_pointers);


  /* and we're done!  (png_read_end() can be omitted if no processing of
    * post-IDAT text/time/etc. is desired) */

  free(row_pointers);
  row_pointers = NULL;

  png_read_end(ctx->png, NULL);

  return 0;
}

void readpng_free(png_containerp ctx) {
  if (ctx->bytes) {
    free(ctx->bytes);
    ctx->bytes = NULL;
  }
  png_destroy_read_struct(ctx->png ? &ctx->png : NULL,
                          ctx->info ? &ctx->info : NULL, 
                          NULL);
  ctx->png = NULL;
  ctx->info = NULL;
}
