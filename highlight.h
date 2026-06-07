#ifndef HIGHLIGHT_H
#define HIGHLIGHT_H

#include <ncurses.h>
#include <regex.h>

#include "colors.h"

typedef enum { c, py, md, blank, LANG_COUNT } language;

// the langauges name as a string
typedef struct {
    language lang;
    const char *l;
} lang_names;

// so Syntax's can be static const's
typedef struct {
    regex_t cache;
    int cached;
} RGXE;

typedef struct {
    const char *rgx; // RegEx expression (string)
    Colors color;    // a code to RGB values
    attr_t attr;     // ncurses text attribute
} SyntaxDemand;

typedef struct {
    const char *init_rgx;
    const char *kill_rgx;
    Colors color;
    attr_t attr;
} SyntaxSpan;

typedef struct {
    language lang;

    const SyntaxDemand *demands;
    RGXE *rt_dmds;
    int n_demands;

    const SyntaxSpan *mx_demands;
    RGXE *rt_mxi;
    RGXE *rt_mxk;
    int n_mx_demands;

    const char *indentables;
    const char *dedentables;
    int n_dentables;
    int indent_len;
} LangComponents;

int load_colors(void);

void cache_regex(LangComponents *lc, language lang);

int compile(regex_t *cache, const char *rgx);

void free_reg(LangComponents *lc);

void flush_lang(LangComponents *lc);

#endif
