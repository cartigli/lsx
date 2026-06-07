#include <ncurses.h>
#include <stdio.h>

#include "colors.h"
#include "error.h"

// RGB codes for color scheme (hexadecimal values)

const ColorCode COLOR_CODES[] = {
    {
        "f6",
        "aa",
        "d3",
    }, // pink
    {
        "23",
        "ad",
        "61",
    }, // green
    {
        "c3",
        "91",
        "ed",
    }, // purple
    {
        "87",
        "ce",
        "eb",
    }, // cyan
    {
        "b3",
        "b3",
        "b3",
    }, // gray
    {
        "fa",
        "f1",
        "87",
    }, // yellow
    {
        "e6",
        "67",
        "6b",
    }, // reddish
    {
        "27",
        "f5",
        "be",
    }, // teal
    {
        "e9",
        "97",
        "3f",
    }, // orange
    {
        "53",
        "df",
        "dd",
    }, // py_cyan
    {
        "a8",
        "82",
        "ff",
    }, // py_purple
    {
        "44",
        "cf",
        "6e",
    }, // py_green
    {
        "b3",
        "b3",
        "b3",
    }, // gray duplicate
    {
        "ff",
        "ff",
        "ff",
    }, // white
    {
        "ef",
        "ef",
        "d8",
    }, // offwhite
};

static inline int hex_compr(const char hex[])
{
    int ccode;
    sscanf(hex, "%x", &ccode);
    return (int)((ccode * 1000) / 255.0);
}

int load_colors(void)
{
    if (!has_colors()) {
        // print_err(hlte_src, "terminal does not support colors", 2);
        LOG_WARN("terminal does not support colors");
        return 1;
    }
    start_color();

    short pink   = 14; // pink
    short green  = 15; // green
    short purple = 16; // purple
    short cyan   = 17; // cyan
    short gray   = 18; // light gray
    short yellow = 19; // yellow
    short redsh  = 20; // reddish
    short teal   = 21; // teal
    short orange = 22; // orange
    short py_cya = 23; // python cyan
    short py_pur = 24; // python purple
    short py_gre = 25; // python green
    short white  = 26; // white
    short fwhite = 27; // off-white

    init_color(pink, hex_compr(COLOR_CODES[0].r), hex_compr(COLOR_CODES[0].g),
        hex_compr(COLOR_CODES[0].b));
    init_color(green, hex_compr(COLOR_CODES[1].r), hex_compr(COLOR_CODES[1].g),
        hex_compr(COLOR_CODES[1].b));
    init_color(purple, hex_compr(COLOR_CODES[2].r), hex_compr(COLOR_CODES[2].g),
        hex_compr(COLOR_CODES[2].b));
    init_color(cyan, hex_compr(COLOR_CODES[3].r), hex_compr(COLOR_CODES[3].g),
        hex_compr(COLOR_CODES[3].b));
    init_color(gray, hex_compr(COLOR_CODES[4].r), hex_compr(COLOR_CODES[4].g),
        hex_compr(COLOR_CODES[4].b));
    init_color(yellow, hex_compr(COLOR_CODES[5].r), hex_compr(COLOR_CODES[5].g),
        hex_compr(COLOR_CODES[5].b));
    init_color(redsh, hex_compr(COLOR_CODES[6].r), hex_compr(COLOR_CODES[6].g),
        hex_compr(COLOR_CODES[6].b));
    init_color(teal, hex_compr(COLOR_CODES[7].r), hex_compr(COLOR_CODES[7].g),
        hex_compr(COLOR_CODES[7].b));
    init_color(orange, hex_compr(COLOR_CODES[8].r), hex_compr(COLOR_CODES[8].g),
        hex_compr(COLOR_CODES[8].b));
    init_color(py_cya, hex_compr(COLOR_CODES[9].r), hex_compr(COLOR_CODES[9].g),
        hex_compr(COLOR_CODES[9].b));
    init_color(py_pur, hex_compr(COLOR_CODES[10].r),
        hex_compr(COLOR_CODES[10].g), hex_compr(COLOR_CODES[10].b));
    init_color(py_gre, hex_compr(COLOR_CODES[11].r),
        hex_compr(COLOR_CODES[11].g), hex_compr(COLOR_CODES[11].b));
    init_color(white, hex_compr(COLOR_CODES[12].r),
        hex_compr(COLOR_CODES[12].g), hex_compr(COLOR_CODES[12].b));
    init_color(fwhite, hex_compr(COLOR_CODES[13].r),
        hex_compr(COLOR_CODES[13].g), hex_compr(COLOR_CODES[13].b));

    // args: int: pair_no, fg color, bg color
    init_pair(1, pink, COLOR_BLACK);
    init_pair(2, green, COLOR_BLACK);
    init_pair(3, purple, COLOR_BLACK);
    init_pair(4, cyan, COLOR_BLACK);
    init_pair(5, gray, COLOR_BLACK);
    init_pair(6, yellow, COLOR_BLACK);
    init_pair(7, redsh, COLOR_BLACK);
    init_pair(8, teal, COLOR_BLACK);
    init_pair(9, orange, COLOR_BLACK);
    init_pair(10, py_cya, COLOR_BLACK);
    init_pair(11, py_pur, COLOR_BLACK);
    init_pair(12, py_gre, COLOR_BLACK);
    init_pair(13, white, COLOR_BLACK);
    init_pair(14, fwhite, COLOR_BLACK);

    // set the default color for non-colored text
    assume_default_colors(fwhite, COLOR_BLACK);

    return 0;
}

// extern const ColorCode COLOR_CODES[];
