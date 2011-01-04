#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <ncurses.h>

#include "pngutil.h"

#define PATCH_SIZE_W   5
#define PATCH_SIZE_H   9

/**
 * Calls endwin() first, then calls exit(code)
 */
static void ncurses_exit(int code) {
  endwin();
  exit(code);
}

/**
 * Computes euclidean distance between color and r,g,b
 */
static float distance(short color, uint8_t r, uint8_t g, uint8_t b) {
  short color_r, color_g, color_b;
  color_content(color, &color_r, &color_g, &color_b);
  #define SCALE(c) (((float)c)/1000.f*255.f)
  float term_r = SCALE(color_r) - ((float)r);
  float term_g = SCALE(color_g) - ((float)g);
  float term_b = SCALE(color_b) - ((float)b);
  return sqrt(term_r * term_r + term_g * term_g + term_b * term_b);  
}

/**
 * Finds best color match in terms of euclidean distance
 */
static int best_color(uint8_t r, uint8_t g, uint8_t b) {
  short bestSoFar = 0;
  float bestValueSoFar = distance(0, r, g, b);
  short i;
  float test;
  for (i = 1; i < 8; i++) {
    test = distance(i, r, g, b);
    if (test < bestValueSoFar) {
      bestSoFar = i;
      bestValueSoFar = test;
    }
  }
  return bestSoFar;
}

int main(int argc, char **argv) {

  if (argc < 2) {
    fprintf(stderr, "[USAGE] %s filename.png\n", argv[0]);
    exit(1);
  }

  char *filename = argv[1];
  FILE* fp = fopen(filename, "rb");
  if (!fp) {
    fprintf(stderr, "Error in opening file: %s\n", filename);
    exit(1);
  }

  initscr();

  if (has_colors() == FALSE) { 
    fprintf(stderr, "Your terminal does not support color\n");
    ncurses_exit(1);
  }

  start_color();

  if (COLORS < 8 || COLOR_PAIRS < 8) {
    fprintf(stderr, "Your terminal does not support at least 8 colors and color pairs\n");
    ncurses_exit(1);
  }

  int c;
  for (c = 0; c < 8; c++) 
    init_pair(c + 1, c, COLOR_BLACK);

  struct png_container ctx;

  if (readpng_init(fp, &ctx)) {
    fprintf(stderr, "Error in opening file as a PNG file\n");
    ncurses_exit(1);
  }

  uint32_t pChannels, pRowBytes;
  if (readpng_get_image(&ctx, &pChannels, &pRowBytes)) {
    fprintf(stderr, "Error in reading data from PNG file\n");
    ncurses_exit(1);
  }

  int x, y, i, j, clr;
  for (y = 0; y < ctx.height; y += PATCH_SIZE_H) {
    for (x = 0; x < ctx.width; x += PATCH_SIZE_W) {

      uint64_t r_buf = 0;
      uint64_t g_buf = 0;
      uint64_t b_buf = 0;

      #define MIN(a, b) ((a < b) ? a : b)
      int patch_height = MIN(ctx.height - y, PATCH_SIZE_H);
      int patch_width = MIN(ctx.width - x, PATCH_SIZE_W);
      int num_elems = patch_height * patch_width;

      for (j = 0; j < patch_height; j++) {
        for (i = 0; i < patch_width; i++) {
          uint8_t* p = ctx.bytes + (y + j) * pRowBytes + (x + i) * pChannels;
          r_buf += p[0];
          g_buf += p[1];
          b_buf += p[2];
        }
      }

      clr = best_color(r_buf / num_elems, g_buf / num_elems, b_buf / num_elems);
      attron(COLOR_PAIR(clr + 1));
      addch(ACS_BLOCK);
      attroff(COLOR_PAIR(clr + 1));

    }
    addch('\n');
  }

  refresh();
  getch();
  endwin();

  readpng_free(&ctx);
  return 0;
}
