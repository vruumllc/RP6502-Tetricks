// ---------------------------------------------------------------------------
// bitmap_graphics.h
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

#ifndef BITMAP_GRAPHICS_H
#define BITMAP_GRAPHICS_H

#include <stdbool.h>
#include <stdint.h>
#include "colors.h"

#define swap(a, b) { int16_t t = a; a = b; b = t; }

// For writing text
#define TABSPACE 4 // number of spaces for a tab

// For accessing the font library
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))

void init_bitmap_graphics(uint16_t canvas_struct_address,
                          uint16_t canvas_data_address,
                          uint8_t  canvas_plane,
                          uint8_t  canvas_type,
                          uint16_t canvas_width,
                          uint16_t canvas_height,
                          uint8_t  bits_per_pixel);
uint16_t canvas_width(void);
uint16_t canvas_height(void);
uint8_t bits_per_pixel(void);

uint16_t random(uint16_t low_limit, uint16_t high_limit);

void erase_canvas(void);
void draw_pixel(uint16_t color, uint16_t x, uint16_t y);
void draw_vline(uint16_t color, uint16_t x, uint16_t y, uint16_t h);
void draw_hline(uint16_t color, uint16_t x, uint16_t y, uint16_t w);
void draw_line(uint16_t color, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void draw_rect(uint16_t color, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void fill_rect(uint16_t color, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void draw_circle(uint16_t color, uint16_t x0, uint16_t y0, uint16_t r);
void fill_circle(uint16_t color, uint16_t x0, uint16_t y0, uint16_t r);
void draw_rounded_rect(uint16_t color, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r);
void fill_rounded_rect(uint16_t color, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r);

void set_cursor(uint16_t x, uint16_t y);
void set_text_multiplier(uint8_t mult);
void set_text_color(uint16_t color); // transparent background
void set_text_colors(uint16_t color, uint16_t background);
void set_text_wrap(bool w);
void draw_char(char chr, uint16_t x, uint16_t y);
void draw_string(char * str);

#endif // BITMAP_GRAPHICS_H
