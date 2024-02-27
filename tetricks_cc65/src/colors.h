// ---------------------------------------------------------------------------
// colors.h
//
// This little library simplifies color selection for the RP6502.
//
// If your application uses multiple bits/pixel, you can select 16 colors
// for 4bpp, 8bpp, and 16bpp using the color() function.
//
// You can specify any color for 16bpp using color_from_rgb5(r,g,b).
//
// If your app is just written for 4bpp or 8bpp, you can simply use the macros.
//
// Written by tonyvr, and I don't care what you do with this code. ENJOY!
// ---------------------------------------------------------------------------

#ifndef COLORS_H
#define COLORS_H

#include <stdint.h>
#include <stdbool.h>

#define COLOR_ALPHA_MASK (1u<<5)

#define BLACK 0
#define DARK_RED 1
#define DARK_GREEN 2
#define BROWN 3
#define DARK_BLUE 4
#define DARK_MAGENTA 5
#define DARK_CYAN 6
#define LIGHT_GRAY 7
#define DARK_GRAY 8
#define RED 9
#define GREEN 10
#define YELLOW 11
#define BLUE 12
#define MAGENTA 13
#define CYAN 14
#define WHITE 15

uint16_t color_from_rgb5(uint8_t r, uint8_t g, uint8_t b);
uint16_t color(uint8_t index, bool bpp16);

#endif // COLORS_H