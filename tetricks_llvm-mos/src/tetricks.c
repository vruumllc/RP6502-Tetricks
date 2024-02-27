// ---------------------------------------------------------------------------
// tetricks.c
//
// This code is influenced by the Blocks.bas basic program,
// written by Geoff Graham for the MMBasic on the PicoMiteVGA.
// https://geoffg.net/picomitevga.html
//
// There doesn't seem to be a copyright or a license associated with that code.
// I don't care what you do with my version either -- have fun!
//
// tonyvr
// ---------------------------------------------------------------------------

#include <rp6502.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "usb_hid_keys.h"
#include "colors.h"
#include "bitmap_graphics.h"

#define CANVAS_W 320
#define CANVAS_H 240 // 180 or 240

// XRAM locations
#define KEYBOARD_INPUT 0xFF10 // KEYBOARD_BYTES of bitmask data

// 256 bytes HID code max, stored in 32 uint8
#define KEYBOARD_BYTES 32
uint8_t keystates[KEYBOARD_BYTES] = {0};

// keystates[code>>3] gets contents from correct byte in array
// 1 << (code&7) moves a 1 into proper position to mask with byte contents
// final & gives 1 if key is pressed, 0 if not
#define key(code) (keystates[code >> 3] & (1 << (code & 7)))

#define BLOCK_SIZE 8 // block width and height in pixels
#define BLOCKS_W  12 // playing field width, in blocks
#if (CANVAS_H == 180)
    #define BLOCKS_H  22 // playing field height, in blocks
#else
    #define BLOCKS_H  28 // playing field height, in blocks
#endif

// field, in pixels, calculated from constants above
const int16_t field_x = ((CANVAS_W/2)-(BLOCKS_W/2)*BLOCK_SIZE);
const int16_t field_w = (BLOCKS_W*BLOCK_SIZE);
const int16_t field_y = ((CANVAS_H/2)-(BLOCKS_H/2)*BLOCK_SIZE);
const int16_t field_h = (BLOCKS_H*BLOCK_SIZE);

// saves color of block, if any, at field[x][y]
static uint16_t field[BLOCKS_W][BLOCKS_H];

// where to draw stuff
const uint8_t keys_x = CANVAS_W/8;
const uint8_t keys_y = CANVAS_H/5;
const uint8_t next_x = (3*CANVAS_W/4)+BLOCK_SIZE;
const uint8_t next_y = CANVAS_H/8;
const uint8_t level_x = (3*CANVAS_W/4)+BLOCK_SIZE;
const uint8_t level_y = 2*CANVAS_H/4;
const uint8_t score_x = (3*CANVAS_W/4)+BLOCK_SIZE;
const uint8_t score_y = 3*CANVAS_H/4;

// Each bit of each blocks element indicates box drawn or not,
// assuming 4x4 array of possible boxes per shape.
// Array elements specify shape at 0, 90, 180, 270 degrees.
typedef struct {
    uint16_t color;
    uint16_t blocks[4];
} shape;

static shape shapes[] = {
    {WHITE, // 0: white bar
        {0b0010001000100010,  //   0
         0b0000111100000000,  //  90
         0b0010001000100010,  // 180
         0b0000111100000000}},// 270
    {MAGENTA, // 1: magenta tee
        {0b0100110001000000,  //   0
         0b0100111000000000,  //  90
         0b0100011001000000,  // 180
         0b0000111001000000}},// 270
    {BLUE, // 2: blue backward-L
        {0b0100010011000000,  //   0
         0b1000111000000000,  //  90
         0b0110010001000000,  // 180
         0b0000111000100000}},// 270
    {CYAN, // 3: cyan forward-L
        {0b1000100011000000,  //   0
         0b1110100000000000,  //  90
         0b0110001000100000,  // 180
         0b0000001011100000}},// 270
    {YELLOW, // 4: yellow cube
        {0b0110011000000000,  //   0
         0b0110011000000000,  //  90
         0b0110011000000000,  // 180
         0b0110011000000000}},// 270
    {RED, // 5: red forward-Z
        {0b0100110010000000,  //   0
         0b1100011000000000,  //  90
         0b0010011001000000,  // 180
         0b0000110001100000}},// 270
    {GREEN, // 6: green backward-Z
        {0b1000110001000000,  //   0
         0b0110110000000000,  //  90
         0b0100011000100000,  // 180
         0b0000011011000000}} // 270
};

typedef enum {AXIS_X, AXIS_Y, AXIS_Z} move_axis;

static uint8_t next_shape = 0;
static uint8_t current_shape = 0;
static uint8_t current_rotation = 0;
static uint8_t current_x = 0;
static uint8_t current_y = 0;

// level is increased every 10 points
static uint16_t current_level = 1;
static uint16_t current_score = 0;

// timer_threshold = 70-10*level
// start dropping rate at once per second
static uint8_t timer_threshold = 60;
static bool paused = false;
static bool game_over = false;

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static void update_level()
{
    char str_level[10] = {0};
    set_text_multiplier(1);
    set_text_color(CYAN);
    set_cursor(level_x+5*BLOCK_SIZE, level_y);
    fill_rect(BLACK, level_x+5*BLOCK_SIZE, level_y, 4*6, 8);
    snprintf(str_level, 10, "%u", current_level);
    draw_string(str_level);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static void update_score()
{
    uint8_t new_level = current_level;
    char str_score[10] = {0};
    set_text_multiplier(1);
    set_text_color(CYAN);
    set_cursor(score_x+5*BLOCK_SIZE, score_y);
    fill_rect(BLACK, score_x+5*BLOCK_SIZE, score_y, 4*6, 8);
    snprintf(str_score, 10, "%u", current_score);
    draw_string(str_score);

    new_level = current_score/10;
    if (new_level < 7 && current_level < new_level) {
        timer_threshold = 70 - 10*new_level;
        current_level = new_level;
        update_level();
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static void update_paused()
{
    if (paused) {
        set_text_multiplier(1);
        set_text_colors(YELLOW, RED);
        set_cursor(0, canvas_height()-8);
        if (game_over) {
            draw_string(" !! GAME OVER !! ");
        } else {
            draw_string(" !!! PAUSED !!!  ");
        }
    } else {
        fill_rect(BLACK, 0, canvas_height()-8, 17*6, 8);
    }
}

// ----------------------------------------------------------------------------
// Draw the background static elements (only once).
// ----------------------------------------------------------------------------
static void draw_background()
{
    uint8_t i, j;

    // draw title
    set_text_multiplier(2);
    set_text_colors(YELLOW, DARK_RED);
    set_cursor(0, 0);
    draw_string("Tetricks");

    // draw field boundry, with 1 pixel margin
    draw_rect(DARK_GRAY, field_x-2, field_y-2, 2+field_w+1, 2+field_h+1);

    // and draw the grid
    for (i = 0; i < BLOCKS_W; i++) {
        for (j = 0; j < BLOCKS_H; j++) {
            field[i][j] = 0; // clear field array
            draw_pixel(DARK_GRAY,
                       field_x + i*BLOCK_SIZE + (BLOCK_SIZE/2) - 1,
                       field_y + j*BLOCK_SIZE + (BLOCK_SIZE/2) - 1);
        }
    }

    // draw help text
    set_text_multiplier(1);
    set_text_color(WHITE);
    set_cursor(keys_x-BLOCK_SIZE, keys_y-(BLOCK_SIZE/2));
    draw_string("KEYS:\n\n");
    draw_string(" Left    'Left'\n\n");
    draw_string(" Right   'Right'\n\n");
    draw_string(" Rotate  'Up'\n\n");
    draw_string(" Drop    'Down'\n\n\n");
    draw_string(" Pause   'p'\n\n");
    draw_string(" Restart 'r'\n\n");
    draw_string(" Quit    'Esc'");

    // draw next shape text
    set_text_multiplier(1);
    set_text_color(WHITE);
    set_cursor(next_x, next_y);
    draw_string("NEXT:");

    // draw level text
    set_text_multiplier(1);
    set_text_color(WHITE);
    set_cursor(level_x, level_y);
    draw_string("LEVEL:");
    update_level();

    // draw score text
    set_text_multiplier(1);
    set_text_color(WHITE);
    set_cursor(score_x, score_y);
    draw_string("SCORE:");
    update_score();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static void draw_shape(uint8_t shape, uint8_t rotation, uint16_t x, uint16_t y)
{
    uint8_t i;
    for (i = 0; i < 16; i++) {
        if (1<<i & shapes[shape].blocks[rotation]) {
            uint8_t row = ((i>11)?1:0) + ((i>7)?1:0) + ((i>3)?1:0);
            draw_rect(shapes[shape].color,
                      x+(i%4)*BLOCK_SIZE,
                      y+row*BLOCK_SIZE,
                      BLOCK_SIZE-1,
                      BLOCK_SIZE-1);
        }
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static void erase_shape(uint8_t shape, uint8_t rotation, uint16_t x, uint16_t y)
{
    uint8_t i;
    for (i = 0; i < 16; i++) {
        if (1<<i & shapes[shape].blocks[rotation]) {
            uint8_t row = ((i>11)?1:0) + ((i>7)?1:0) + ((i>3)?1:0);
            draw_rect(BLACK,
                      x+(i%4)*BLOCK_SIZE,
                      y+row*BLOCK_SIZE,
                      BLOCK_SIZE-1,
                      BLOCK_SIZE-1);
        }
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void restart_game()
{
    uint8_t i, j;
    // clear the screen of blocks
    for (i = 0; i < BLOCKS_H; i++) {
        for (j = 0;j < BLOCKS_W; j++) {
            draw_rect(BLACK,
                      field_x + j*BLOCK_SIZE,
                      field_y + i*BLOCK_SIZE,
                      BLOCK_SIZE-1,
                      BLOCK_SIZE-1);
            field[j][i] = 0;
        }
    }
    erase_shape(next_shape, 1, next_x, next_y);

    // reset state variables to starting values
    next_shape = lrand()%7;
    current_shape = lrand()%7;
    current_rotation = 1; // 90
    current_x = field_x + (field_w/2) - 2*BLOCK_SIZE;
    current_y = field_y;
    timer_threshold = 60;
    current_level = 1;
    current_score = 0;
    update_level();
    update_score();

    // restart the game
    draw_shape(current_shape, current_rotation, current_x, current_y);
    draw_shape(next_shape, 1, next_x, next_y);
    paused = false;
    game_over = false;
    update_paused();
}

// ----------------------------------------------------------------------------
// Returns true if no collision between field borders or other pieces.
// ----------------------------------------------------------------------------
static bool validate_move(uint8_t new_rotation, uint16_t new_x, uint16_t new_y)
{
    uint8_t i;
    // for each possible block in shape
    for (i = 0; i < 16; i++) {
        if (1<<i & shapes[current_shape].blocks[new_rotation]) {
            // OK, we have to test this block against field
            uint8_t col = i%4;
            uint8_t row = ((i>11)?1:0) + ((i>7)?1:0) + ((i>3)?1:0);
            int16_t field_col = col + ((int16_t)new_x-field_x)/BLOCK_SIZE;
            int16_t field_row = row + ((int16_t)new_y-field_y)/BLOCK_SIZE;
            if ((field_col < 0) ||                  // collision with field left border
                (field_col > (BLOCKS_W-1)) ||       // collision with field right border
                (field_row < 0) ||                  // collision with field top border
                (field_row > (BLOCKS_H-1)) ||       // collision with field bottom border
                (field[field_col][field_row] != 0)  // collision with another block on field
               ) {
                return false;
            }
        }
    }
    return true;
}

// ----------------------------------------------------------------------------
// If allowed, erases shape in old position and redraws it in new position.
// Returns true if validation succeeded, else false.
// ----------------------------------------------------------------------------
static bool move_shape(move_axis axis, uint8_t new_rotation, uint16_t new_x, uint16_t new_y)
{
    if (validate_move(new_rotation, new_x, new_y)) {
        erase_shape(current_shape, current_rotation, current_x, current_y);
        switch (axis) {
            case AXIS_X:
                current_x = new_x;
                break;
            case AXIS_Y:
                current_y = new_y;
                break;
            case AXIS_Z:
                current_rotation = new_rotation;
                break;
        }
        draw_shape(current_shape, current_rotation, current_x, current_y);
        return true;
    }
    return false;
}

// ----------------------------------------------------------------------------
// Update the field array with color and position of new shape
// ----------------------------------------------------------------------------
static void save_shape_to_field()
{
    uint8_t i;
    // for each possible block in shape
    for (i = 0; i < 16; i++) {
        if (1<<i & shapes[current_shape].blocks[current_rotation]) {
            // OK, block is not 0 (black)
            uint8_t col = i%4;
            uint8_t row = ((i>11)?1:0) + ((i>7)?1:0) + ((i>3)?1:0);
            int16_t field_col = col + ((int16_t)current_x-field_x)/BLOCK_SIZE;
            int16_t field_row = row + ((int16_t)current_y-field_y)/BLOCK_SIZE;
            field[field_col][field_row] = shapes[current_shape].color;
        }
    }
}

// ----------------------------------------------------------------------------
// Recursively called to copy all non-empty rows down one row.
// ----------------------------------------------------------------------------
static void copy_row_above(uint8_t row)
{
    if (row > 0) {
        uint8_t col;
        bool row_above_not_blank = false;
        for (col = 0; col < BLOCKS_W; col++) {
            if (!row_above_not_blank && field[col][row-1] != 0) {
                row_above_not_blank = true;
            }
            field[col][row] = field[col][row-1];
            draw_rect(field[col][row],
                      field_x + col*BLOCK_SIZE,
                      field_y + row*BLOCK_SIZE,
                      BLOCK_SIZE-1,
                      BLOCK_SIZE-1);
        }
        if (row_above_not_blank) {
            copy_row_above(row-1); // recurse
        }
    }
}

// ----------------------------------------------------------------------------
// Looks for completely filled 'scoring' rows.
// Recursively called for all non-empty rows,
// Note that finding more than one scoring row results in scoring bonus!
// ----------------------------------------------------------------------------
static void check_for_scoring_row(uint8_t row, uint8_t *p_num_scoring_rows)
{
    uint8_t col;
    if (row > 0) {
        bool scoring_row = true;
        bool row_above_not_blank = false;
        for (col = 0; col < BLOCKS_W; col++) {
            if (field[col][row] == 0) {
                scoring_row = false;
            } else if (!row_above_not_blank) {
                row_above_not_blank = true;
            }
        }
        if (scoring_row) {
            *p_num_scoring_rows += 1;
            current_score += *p_num_scoring_rows; // +1, +2, +3, ...
            copy_row_above(row); // recursive routine results in all rows above moving down
        }
        if (row_above_not_blank) {
            // if scoring row, it is now gone, so we need to test the same row again
            check_for_scoring_row(row - (scoring_row?0:1), p_num_scoring_rows); // recurse
        }
    }
}

// ----------------------------------------------------------------------------
// Kicks off recursive scoring check, and recursive scoring row removal
// ----------------------------------------------------------------------------
static void check_for_scoring_rows()
{
    uint8_t num_scoring_rows = 0;
    check_for_scoring_row(BLOCKS_H-1, &num_scoring_rows);
    update_score();
}

// ----------------------------------------------------------------------------
// Deal with the consequences of a dropped shape
// ----------------------------------------------------------------------------
static void process_drop()
{
    // Shape has dropped as far as possible,
    // so update field and see if we scored
    save_shape_to_field();
    check_for_scoring_rows();

    // Try to add a new shape at top
    erase_shape(next_shape, 1, next_x, next_y);
    current_shape = next_shape;
    next_shape = lrand()%7;
    current_rotation = 1; // 90
    current_x = field_x + (field_w/2) - 2*BLOCK_SIZE;
    current_y = field_y;
    if (validate_move(current_rotation, current_x, current_y)) {
        draw_shape(next_shape, 1, next_x, next_y);
        draw_shape(current_shape, current_rotation, current_x, current_y);
    } else { // can't add new shape at top either, so...game over!
        paused = true;
        game_over = true;
        update_paused();
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
int main()
{
    uint8_t v; // vsync counter, incements every 1/60 second, rolls over every 256
    uint16_t timer = 0; // counts longer before rolling over, and we can reset it
    bool handled_key = false;

    // plane=0, canvas=1, w=320, h=240, bpp4
#if (CANVAS_H == 180)
    init_bitmap_graphics(0xFF00, 0x0000, 0, 2, CANVAS_W, CANVAS_H, 4);
#else
    init_bitmap_graphics(0xFF00, 0x0000, 0, 1, CANVAS_W, CANVAS_H, 4);
#endif

    // Erase display
    erase_canvas();
    //printf("\f"); // clear console

    //xreg_vga_mode(0, 1); // console

    printf("Hello, from Tetricks!\n");

    draw_background();
    restart_game();

    // initialize keyboard
    xregn( 0, 0, 0, 1, KEYBOARD_INPUT);
    RIA.addr0 = KEYBOARD_INPUT;
    RIA.step0 = 0;

    // vsync loop
    v = RIA.vsync;
    while (1) {
        uint8_t i;

        // we will use the RIA.vsync counter for our drop timer
        if (v == RIA.vsync) {
            continue; // wait until vsync is incremented
        } else {
            v = RIA.vsync; // new value for v
        }
        timer++; // use this instead of v, because we can reset this

        // Try to drop current_shape one row every time timer exceeds timer_threshold
        if (!paused && timer > timer_threshold) {
            timer = 0; // reset it

            // drop the current_shape if possible
            if (!move_shape(AXIS_Y, current_rotation, current_x, current_y+BLOCK_SIZE)) {
                process_drop();
            }
        }

        // fill the keystates bitmask array
        for (i = 0; i < KEYBOARD_BYTES; i++) {
            uint8_t new_keys;
            RIA.addr0 = KEYBOARD_INPUT + i;
            new_keys = RIA.rw0;
/*
            // check for change in any and all keys
            {
                uint8_t j;
                for (j = 0; j < 8; j++) {
                    uint8_t new_key = (new_keys & (1<<j));
                    if ((((i<<3)+j)>3) && (new_key != (keystates[i] & (1<<j)))) {
                        printf( "key %d %s\n", ((i<<3)+j), (new_key ? "pressed" : "released"));
                    }
                }
            }
*/
            keystates[i] = new_keys;
        }

        // check for a key down
        if (!(keystates[0] & 1)) {
            if (!handled_key) { // handle only once per single keypress
                // handle the keystrokes
                if (!paused && key(KEY_RIGHT)) { // try to move shape right
                    move_shape(AXIS_X, current_rotation, current_x+BLOCK_SIZE, current_y);
                } else if (!paused && key(KEY_LEFT)) { // try to move shape left
                    move_shape(AXIS_X, current_rotation, current_x-BLOCK_SIZE, current_y);
                } else if (!paused && key(KEY_UP)) { // try to rotate shape
                    move_shape(AXIS_Z, (current_rotation+1)%4, current_x, current_y);
                } else if (!paused && key(KEY_DOWN)) { // drop the shape as far as possible
                    while(move_shape(AXIS_Y, current_rotation, current_x, current_y+BLOCK_SIZE)){;}
                    process_drop();
                }  else if (key(KEY_P)) { // pause
                    paused = !paused;
                    update_paused();
                } else if (key(KEY_R)) { // restart game
                    restart_game();
                } else if (key(KEY_ESC)) { // exit game
                    break;
                }
                handled_key = true;
            }
        } else { // no keys down
            handled_key = false;
        }
    }
    //exit
    printf("Goodbye!\n");
}
