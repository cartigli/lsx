#ifndef COLORS_H
#define COLORS_H

// supported colors
typedef enum {
    PINK      = 1,
    GREEN     = 2,
    PURPLE    = 3,
    CYAN      = 4,
    LTGRAY    = 5,
    YELLOW    = 6,
    REDDSH    = 7,
    TEAL      = 8,
    ORANGE    = 9,
    PY_CYAN   = 10,
    PY_PURPLE = 11,
    PY_GREEN  = 12,
    WHITE     = 13,
    OFFWHITE  = 14,
} Colors;

// a single color's RGB values (hexadecimal format)
typedef struct {
    char r[4];
    char g[4];
    char b[4];
} ColorCode;

int load_colors(void);

#endif
