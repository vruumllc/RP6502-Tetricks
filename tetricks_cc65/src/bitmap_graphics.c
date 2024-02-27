// ---------------------------------------------------------------------------
// bitmap_graphics.c
//
// This library was written by tonyvr to simplify bitmap graphics programming
// of the RP6502 picocomputer designed by Rumbledethumps.
//
// This code is an adaptation of the vga_graphics library written by V. Hunter Adams
// from Cornell University, for his excellent RP2040 microcontroller programming course.
//
// https://github.com/vha3/Hunter-Adams-RP2040-Demos/tree/master/VGA_Graphics/VGA_Graphics_Primitives
//
// There doesn't seem to be a copyright or a license associated with his code.
// I don't care what you do with my version either -- have fun!
// ---------------------------------------------------------------------------

#include <rp6502.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "font5x7.h"
#include "colors.h"
#include "bitmap_graphics.h"

// For drawing lines
static uint16_t canvas_struct = 0xFF00;
static uint16_t canvas_data = 0x0000;
static uint8_t  plane = 0;
static uint8_t  canvas_mode = 2;
static uint16_t canvas_w = 320;
static uint16_t canvas_h = 180;
static uint8_t  bpp_mode = 3;

// For drawing characters
static uint16_t cursor_y = 0;
static uint16_t cursor_x = 0;
static uint8_t textmultiplier = 1;
static uint16_t textcolor = 15;
static uint16_t textbgcolor = 15;
static bool wrap = true;

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
static uint8_t bpp_mode_to_bpp[] = {1, 2, 4, 8, 16};
static uint8_t bbp_to_bpp_mode(uint8_t bpp)
{
    switch(bpp) {
        case 1:  return 0;
        case 2:  return 1;
        case 4:  return 2;
        case 8:  return 3;
        case 16: return 4;
    }
    return 2; // default
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void init_bitmap_graphics(uint16_t canvas_struct_address,
                          uint16_t canvas_data_address,
                          uint8_t  canvas_plane,
                          uint8_t  canvas_type,
                          uint16_t canvas_width,
                          uint16_t canvas_height,
                          uint8_t  bits_per_pixel)
{
    uint8_t x_offset = 0;
    uint8_t y_offset = 0;

    // defaults
    canvas_struct = 0xFF00;
    canvas_data = 0x0000;
    plane = 0;
    canvas_mode = 2;
    canvas_w = 320;
    canvas_h = 180;
    bpp_mode = 3;

    // valid range check
    if (canvas_struct_address != 0) {
        canvas_struct = canvas_struct_address;
    }
    if (canvas_data_address != 0) {
        canvas_data = canvas_data_address;
    }
    if (/*canvas_plane >= 0 &&*/ canvas_plane <= 2) {
        plane = canvas_plane;
    }
    if (canvas_type > 0 && canvas_type <= 4) {
        canvas_mode = canvas_type;
    }
    if (canvas_width > 0 && canvas_width <= 640) {
        canvas_w = canvas_width;
    }
    if (canvas_height > 0 && canvas_height <= 480) {
        canvas_h = canvas_height;
    }
    if (bits_per_pixel == 1 ||
        bits_per_pixel == 2 ||
        bits_per_pixel == 4 ||
        bits_per_pixel == 8 ||
        bits_per_pixel == 16  ) {
        bpp_mode = bbp_to_bpp_mode(bits_per_pixel);
    }

    // additional contraints (due to memory limit of 64K)
    if (bpp_mode_to_bpp[bpp_mode] == 16) { // bits color
        canvas_mode = 2;
        canvas_w = 240; // max for 16-bit color
        canvas_h = 124; // max for 16-bit color
    } else if (bpp_mode_to_bpp[bpp_mode] == 8) { // bits color
        canvas_mode = 2;
        canvas_w = 320; // max for 8-bit color
        canvas_h = 180; // max for 8-bit color
    } else if (bpp_mode_to_bpp[bpp_mode] == 4) { // bits color
        canvas_w = 320; // max for 4-bit color
        if (canvas_mode > 2) {
            canvas_mode = 1;
            canvas_h = 240; // max for 4-bit color
        } else if (canvas_mode == 2) {
            canvas_h = 180; // max for canvas_mode 2
        }
    } else if (bpp_mode_to_bpp[bpp_mode] == 2) { // bits color
        if (canvas_mode == 4) {
            canvas_h = 360; // max for canvas_mode 4
        }
    }

    // center canvas if necessary
    if (bpp_mode_to_bpp[bpp_mode] == 16) {
        x_offset = 30; // (360 - 240)/4
        y_offset = 29; // (240 - 124)/4
    }

    if (canvas_struct_address != canvas_struct) {
        printf("Asked for canvas_struct_address of 0x%04X, but got 0x%04X\n", canvas_struct_address, canvas_struct);
    }
    if (canvas_data_address != canvas_data) {
        printf("Asked for canvas_data_address of 0x%04X, but got 0x%04X\n", canvas_struct_address, canvas_data);
    }
    if (canvas_type != canvas_mode) {
        printf("Asked for canvas_type of %u, but got %u\n", canvas_type, canvas_mode);
    }
    if (canvas_width != canvas_w) {
        printf("Asked for canvas_width of %u, but got %u\n", canvas_width, canvas_w);
    }
    if (canvas_height != canvas_h) {
        printf("Asked for canvas_height of %u, but got %u\n", canvas_height, canvas_h);
    }
    if (bits_per_pixel != bpp_mode_to_bpp[bpp_mode]) {
        printf("Asked for bits_per_pixel of %u, but got %u\n", bits_per_pixel, bpp_mode_to_bpp[bpp_mode]);
    }

    // initialize the canvas
    xreg_vga_canvas(canvas_mode);
    //xregn(1, 0, 0, 1, canvas_mode);

    xram0_struct_set(canvas_struct, vga_mode3_config_t, x_wrap, false);
    xram0_struct_set(canvas_struct, vga_mode3_config_t, y_wrap, false);
    xram0_struct_set(canvas_struct, vga_mode3_config_t, x_pos_px, x_offset);
    xram0_struct_set(canvas_struct, vga_mode3_config_t, y_pos_px, y_offset);
    xram0_struct_set(canvas_struct, vga_mode3_config_t, width_px, canvas_w);
    xram0_struct_set(canvas_struct, vga_mode3_config_t, height_px, canvas_h);
    xram0_struct_set(canvas_struct, vga_mode3_config_t, xram_data_ptr, canvas_data);
    xram0_struct_set(canvas_struct, vga_mode3_config_t, xram_palette_ptr, 0xFFFF);

    // initialize the bitmap video modes
    xreg_vga_mode(3, bpp_mode, canvas_struct, plane); // bitmap mode
    //xregn(1, 0, 1, 4, 3, bpp_mode, canvas_struct, plane);

    //xreg_vga_mode(0, 1); // console
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
uint16_t canvas_width(void)
{
    return canvas_w;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
uint16_t canvas_height(void)
{
    return canvas_h;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
uint8_t bits_per_pixel(void)
{
    return bpp_mode_to_bpp[bpp_mode];
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
uint16_t random(uint16_t low_limit, uint16_t high_limit)
{
    if (low_limit > high_limit) {
        swap(low_limit, high_limit);
    }

    return (uint16_t)((rand() % (high_limit-low_limit)) + low_limit);
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void erase_canvas(void)
{
    uint16_t i, num_bytes;

    if (bpp_mode == 4) { // 16bpp
        num_bytes = (canvas_w<<1) * canvas_h;
    } else if (bpp_mode == 3) { // 8bpp
        num_bytes = canvas_w * canvas_h;
    } else if (bpp_mode == 2) { // 4bpp
        num_bytes = (canvas_w>>1) * canvas_h;
    } else if (bpp_mode == 1) { //2bpp
        num_bytes = (canvas_w>>2) * canvas_h;
    } else if (bpp_mode == 0) { //1bpp
        num_bytes = (canvas_w>>3) * canvas_h;
    }

    RIA.addr0 = canvas_data;
    RIA.step0 = 1;
    for (i = 0; i < (num_bytes/16); i++) {
        // unrolled for speed
        RIA.rw0 = 0;
        RIA.rw0 = 0;
        RIA.rw0 = 0;
        RIA.rw0 = 0;
        RIA.rw0 = 0;
        RIA.rw0 = 0;
        RIA.rw0 = 0;
        RIA.rw0 = 0;
        RIA.rw0 = 0;
        RIA.rw0 = 0;
        RIA.rw0 = 0;
        RIA.rw0 = 0;
        RIA.rw0 = 0;
        RIA.rw0 = 0;
        RIA.rw0 = 0;
        RIA.rw0 = 0;
    }
}

// ---------------------------------------------------------------------------
// Draw a pixel on the RP6502, for all the various bpp modes.
// ---------------------------------------------------------------------------
void draw_pixel(uint16_t color, uint16_t x, uint16_t y)
{
    if (bpp_mode == 4) { // 16bpp
        RIA.addr0 = canvas_w*2 * y + x*2;
        RIA.step0 = 1;
        RIA.rw0 = color;
        RIA.rw0 = color >> 8;
    } else if (bpp_mode == 3) { // 8bpp
        RIA.addr0 = canvas_w * y + x;
        RIA.step0 = 1;
        RIA.rw0 = color;
    } else if (bpp_mode == 2) { // 4bpp
        uint8_t shift = 4 * (1 - (x & 1));
        RIA.addr0 = canvas_w/2 * y + x/2;
        RIA.step0 = 0;
        RIA.rw0 = (RIA.rw0 & ~(15 << shift)) | ((color & 15) << shift);
    } else if (bpp_mode == 1) { // 2bpp
        uint8_t shift = 2 * (3 - (x & 3));
        RIA.addr0 = canvas_w/4 * y + x/4;
        RIA.step0 = 0;
        if (color > 0 && (color % 4) == 0) {
            color = 1; // avoid 'accidental' black
        }
        RIA.rw0 = (RIA.rw0 & ~(3 << shift)) | ((color & 3) << shift);
    } else if (bpp_mode == 0) { // 1bpp
        uint8_t shift = 1 * (7 - (x & 7));
        RIA.addr0 = canvas_w/8 * y + x/8;
        RIA.step0 = 0;
        color = (color != 0) ? 1 : 0;
        RIA.rw0 = (RIA.rw0 & ~(1 << shift)) | ((color & 1) << shift);
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void draw_vline(uint16_t color, uint16_t x, uint16_t y, uint16_t h)
{
    uint16_t i;
    for (i=y; i<(y+h); i++) {
        draw_pixel(color, x, i);
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void draw_hline(uint16_t color, uint16_t x, uint16_t y, uint16_t w)
{
    uint16_t i;
    for (i=x; i<(x+w); i++) {
        draw_pixel(color, i, y);
    }
}

// ---------------------------------------------------------------------------
// Draw a straight line from (x0,y0) to (x1,y1) with given color
// using Bresenham's algorithm
// ---------------------------------------------------------------------------
void draw_line(uint16_t color, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    int16_t dx, dy;
    int16_t err;
    int16_t ystep;
    int16_t steep = abs((int16_t)y1 - (int16_t)y0) > abs((int16_t)x1 - (int16_t)x0);

    if (steep) {
        swap(x0, y0);
        swap(x1, y1);
    }

    if (x0 > x1) {
        swap(x0, x1);
        swap(y0, y1);
    }

    dx = x1 - x0;
    dy = abs((int16_t)y1 - (int16_t)y0);

    err = dx / 2;

    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }

    for (; x0<=x1; x0++) {
        if (steep) {
            draw_pixel(color, y0, x0);
        } else {
            draw_pixel(color, x0, y0);
        }

        err -= dy;

        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void draw_rect(uint16_t color, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    draw_hline(color, x, y, w);
    draw_hline(color, x, y+h-1, w);
    draw_vline(color, x, y, h);
    draw_vline(color, x+w-1, y, h);
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void fill_rect(uint16_t color, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint16_t i, j;
    for(i=x; i<(x+w); i++) {
        for(j=y; j<(y+h); j++) {
            draw_pixel(color, i, j);
        }
    }
}

// ---------------------------------------------------------------------------
// This seems to draw circle quadrants
// ---------------------------------------------------------------------------
static void draw_circle_helper(uint16_t color,
                               uint16_t x0, uint16_t y0, uint16_t r,
                               uint8_t cornername)
{
    int16_t f     = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x     = 0;
    int16_t y     = r;

    while (x<y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f     += ddF_y;
        }

        x++;
        ddF_x += 2;
        f     += ddF_x;

        if (cornername & 0x4) {
            draw_pixel(color, x0 + x, y0 + y);
            draw_pixel(color, x0 + y, y0 + x);
        }
        if (cornername & 0x2) {
            draw_pixel(color, x0 + x, y0 - y);
            draw_pixel(color, x0 + y, y0 - x);
        }
        if (cornername & 0x8) {
            draw_pixel(color, x0 - y, y0 + x);
            draw_pixel(color, x0 - x, y0 + y);
        }
        if (cornername & 0x1) {
            draw_pixel(color, x0 - y, y0 - x);
            draw_pixel(color, x0 - x, y0 - y);
        }
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void draw_circle(uint16_t color, uint16_t x0, uint16_t y0, uint16_t r)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    draw_pixel(color, x0  , y0+r);
    draw_pixel(color, x0  , y0-r);
    draw_pixel(color, x0+r, y0  );
    draw_pixel(color, x0-r, y0  );

    while (x<y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }

        x++;
        ddF_x += 2;
        f += ddF_x;

        draw_pixel(color, x0 + x, y0 + y);
        draw_pixel(color, x0 - x, y0 + y);
        draw_pixel(color, x0 + x, y0 - y);
        draw_pixel(color, x0 - x, y0 - y);
        draw_pixel(color, x0 + y, y0 + x);
        draw_pixel(color, x0 - y, y0 + x);
        draw_pixel(color, x0 + y, y0 - x);
        draw_pixel(color, x0 - y, y0 - x);
    }
}

// ---------------------------------------------------------------------------
// This seems to draw filled circle quadrants
// ---------------------------------------------------------------------------
static void fill_circle_helper(uint16_t color,
                               uint16_t x0, uint16_t y0, uint16_t r,
                               uint8_t cornername, uint16_t delta)
{
    int16_t f     = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x     = 0;
    int16_t y     = r;

    while (x<y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f     += ddF_y;
        }

        x++;
        ddF_x += 2;
        f     += ddF_x;

        if (cornername & 0x1) {
            draw_vline(color, x0+x, y0-y, 2*y+1+delta);
            draw_vline(color, x0+y, y0-x, 2*x+1+delta);
        }
        if (cornername & 0x2) {
            draw_vline(color, x0-x, y0-y, 2*y+1+delta);
            draw_vline(color, x0-y, y0-x, 2*x+1+delta);
        }
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void fill_circle(uint16_t color, uint16_t x0, uint16_t y0, uint16_t r)
{
    draw_vline(color, x0, y0-r, 2*r+1);
    fill_circle_helper(color, x0, y0, r, 3, 0);
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void draw_rounded_rect(uint16_t color,
                       uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r)
{
    draw_hline(color, x+r  , y    , w-2*r); // Top
    draw_hline(color, x+r  , y+h-1, w-2*r); // Bottom
    draw_vline(color, x    , y+r  , h-2*r); // Left
    draw_vline(color, x+w-1, y+r  , h-2*r); // Right

    // draw four corners
    draw_circle_helper(color, x+r    , y+r    , r, 1);
    draw_circle_helper(color, x+w-r-1, y+r    , r, 2);
    draw_circle_helper(color, x+w-r-1, y+h-r-1, r, 4);
    draw_circle_helper(color, x+r    , y+h-r-1, r, 8);
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void fill_rounded_rect(uint16_t color,
                       uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r)
{
    // smarter version
    fill_rect(color, x+r, y, w-2*r, h);

    // draw four corners
    fill_circle_helper(color, x+w-r-1, y+r, r, 1, h-2*r-1);
    fill_circle_helper(color, x+r    , y+r, r, 2, h-2*r-1);
}

// ---------------------------------------------------------------------------
// Set cursor for text to be printed
// ---------------------------------------------------------------------------
void set_cursor(uint16_t x, uint16_t y)
{
    cursor_x = x;
    cursor_y = y;
}

// ---------------------------------------------------------------------------
// Set multiplier of text to be displayed (1 for 5x7, 2 for 10x14, etc...)
// ---------------------------------------------------------------------------
void set_text_multiplier(uint8_t mult)
{
    textmultiplier = (mult > 0) ? mult : 1;
}

// ---------------------------------------------------------------------------
// Set colors of text to be displayed.
//     For 'transparent' background, we'll set the bg
//     to the same as fg instead of using a flag
// ---------------------------------------------------------------------------
void set_text_color(uint16_t color)
{
    textcolor = textbgcolor = color;
}

// ---------------------------------------------------------------------------
// Set colors of text to be displayed
//      color = color of text
//      background = color of text background
// ---------------------------------------------------------------------------
void set_text_colors(uint16_t color, uint16_t background)
{
    textcolor   = color;
    textbgcolor = background;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
void set_text_wrap(bool w)
{
    wrap = w;
}

// ---------------------------------------------------------------------------
// Draw a character at x, y
// ---------------------------------------------------------------------------
void draw_char(char chr, uint16_t x, uint16_t y)
{
    uint8_t i, j;

    if((x >= canvas_w) ||    // Clip right
       (y >= canvas_h)  ) { // Clip bottom
        return;
    }

    for (i=0; i<6; i++ ) {
        uint8_t line;

        if (i == 5) {
            line = 0x0;
        } else {
            line = pgm_read_byte(font+(chr*5)+i);
        }

        for ( j = 0; j<8; j++) {
            if (line & 0x1) {
                if (textmultiplier == 1) { // default size
                    draw_pixel(textcolor, x+i, y+j);
                } else {  // big size
                    fill_rect(textcolor, x+(i*textmultiplier), y+(j*textmultiplier), textmultiplier, textmultiplier);
                }
            } else if (textbgcolor != textcolor) {
                if (textmultiplier == 1) { // default size
                    draw_pixel(textbgcolor, x+i, y+j);
                } else {  // big size
                    fill_rect(textbgcolor, x+(i*textmultiplier), y+(j*textmultiplier), textmultiplier, textmultiplier);
                }
            }
            line >>= 1;
        }
    }
}

// ---------------------------------------------------------------------------
// Draw a character at cursor_x, cursor_y, then advance the cursor.
// ---------------------------------------------------------------------------
static void draw_char_at_cursor(char chr)
{
    if (chr == '\n') {
        cursor_y += textmultiplier*8;
        cursor_x  = 0;
    } else if (chr == '\r') {
        // skip em
    } else if (chr == '\t') {
        uint16_t new_x = cursor_x + TABSPACE;

        if (new_x < canvas_w) {
            cursor_x = new_x;
        }
    } else {
        draw_char(chr, cursor_x, cursor_y);
        cursor_x += textmultiplier*6;

        if (wrap && (cursor_x > (canvas_w - textmultiplier*6))) {
            cursor_y += textmultiplier*8;
            cursor_x = 0;
        }
    }
}

// ---------------------------------------------------------------------------
// Draw a zero-terminated string at cursor_x, cursor_y, then advance the cursor.
// ---------------------------------------------------------------------------
void draw_string(char * str)
{
    while (*str) {
        draw_char_at_cursor(*str++);
    }
}
