// ---------------------------------------------------------------------------
// colors.c
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

#include "colors.h"

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
uint16_t color_from_rgb5(uint8_t r, uint8_t g, uint8_t b)
{
    return (((uint16_t)b<<11)|((uint16_t)g<<6)|((uint16_t)r));
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
uint16_t color(uint8_t index, bool bpp16)
{
    if (bpp16) {
        uint16_t clr;
        if (index > BLACK && index <= WHITE) {
            switch(index) {
                case DARK_RED:     clr = color_from_rgb5(15, 0, 0); break;
                case DARK_GREEN:   clr = color_from_rgb5( 0,15, 0); break;
                case BROWN:        clr = color_from_rgb5(15,15, 0); break;
                case DARK_BLUE:    clr = color_from_rgb5( 0, 0,15); break;
                case DARK_MAGENTA: clr = color_from_rgb5(15, 0,15); break;
                case DARK_CYAN:    clr = color_from_rgb5( 0,15,15); break;
                case LIGHT_GRAY:   clr = color_from_rgb5(20,20,20); break;
                case DARK_GRAY:    clr = color_from_rgb5(15,15,15); break;
                case RED:          clr = color_from_rgb5(31, 0, 0); break;
                case GREEN:        clr = color_from_rgb5( 0,31, 0); break;
                case YELLOW:       clr = color_from_rgb5(31,31, 0); break;
                case BLUE:         clr = color_from_rgb5( 0, 0,31); break;
                case MAGENTA:      clr = color_from_rgb5(31, 0,31); break;
                case CYAN:         clr = color_from_rgb5( 0,31,31); break;
                case WHITE:        clr = color_from_rgb5(31,31,31); break;
            }
            return clr | COLOR_ALPHA_MASK; // opaque
        } else { // default to transparent BLACK
            return color_from_rgb5(0,0,0) & ~COLOR_ALPHA_MASK;
        }
    }
    return (index <= WHITE) ? index : BLACK;
}