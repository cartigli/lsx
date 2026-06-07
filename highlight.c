#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#include "blank.h"
#include "c_color.h"
#include "error.h"
#include "highlight.h"
#include "md_color.h"
#include "py_color.h"

static const lang_names LANG_NAMES[] = {
    {
        .lang = c,
        .l    = "c",
    },
    {
        .lang = py,
        .l    = "python",
    },
    {
        .lang = md,
        .l    = "markdown",
    },
    {
        .lang = blank,
        .l    = "none",
    },
};

#define SET_LANG(PREFIX)                            \
    do {                                            \
        lc->demands      = PREFIX##_DEMANDS;        \
        lc->rt_dmds      = PREFIX##_RT_DEMANDS;     \
        lc->n_demands    = N_##PREFIX##_DEMANDS;    \
        lc->indentables  = PREFIX##_INDENTABLES;    \
        lc->dedentables  = PREFIX##_DEDENTABLES;    \
        lc->indent_len   = PREFIX##_INDENT_LEN;     \
        lc->n_dentables  = strlen(lc->indentables); \
        lc->mx_demands   = PREFIX##_TWINTERMS;      \
        lc->n_mx_demands = N_##PREFIX##_TWINTERMS;  \
        lc->rt_mxi       = PREFIX##_RT_MXI_DEMANDS; \
        lc->rt_mxk       = PREFIX##_RT_MXK_DEMANDS; \
    } while (0)

void cache_regex(LangComponents *lc, language lang)
{
    // for the given lang, cache each regex expression

    flush_lang(lc); // initialize lc first

    if (lang < 0 || lang >= LANG_COUNT) return;

    switch (lang) {
        case c:
            SET_LANG(C);
            break;
        case py:
            SET_LANG(PY);
            break;
        case md:
            SET_LANG(MD);
            break;
        case blank:
        default:
            SET_LANG(BLANK);
            break;
    }

    // use the static const expressions to compile the expressions
    for (int i = 0, n = lc->n_demands; i < n; i++) {
        const SyntaxDemand *sx = &lc->demands[i];
        RGXE *rx               = &lc->rt_dmds[i];

        rx->cached = compile(&rx->cache, sx->rgx);
        if (rx->cached == 0) {
            LOG_WARN("regex expression failed to compile");
        }
    }

    for (int i = 0, n = lc->n_mx_demands; i < n; i++) {
        const SyntaxSpan *ss = &lc->mx_demands[i];
        RGXE *init           = &lc->rt_mxi[i];
        RGXE *kill           = &lc->rt_mxk[i];

        init->cached = compile(&init->cache, ss->init_rgx);
        if (!init->cached) {
            LOG_WARN("regex initiator failed to compile");
            continue;
        }
        kill->cached = compile(&kill->cache, ss->kill_rgx);
    }

    char bf[42];
    snprintf(bf, sizeof(bf), "detected lang: %s", LANG_NAMES[lang].l);
    LOG_DEBUG(bf);

    return;
}

int compile(regex_t *cache, const char *rgx)
{
    if (regcomp(cache, rgx, REG_EXTENDED) == 0) return 1;
    return 0;
}

void free_reg(LangComponents *lc)
{
    if (!lc->rt_dmds || !lc->n_demands) return;

    for (int i = 0, n = lc->n_demands; i < n; i++) {
        if (!lc->rt_dmds[i].cached) continue;
        regfree(&lc->rt_dmds[i].cache);
    }

    for (int i = 0, n = lc->n_mx_demands; i < n; i++) {
        RGXE *init = &lc->rt_mxi[i];
        RGXE *kill = &lc->rt_mxk[i];

        if (init->cached) {
            regfree(&init->cache);
            init->cached = 0;
        }
        if (kill->cached) {
            regfree(&kill->cache);
            kill->cached = 0;
        }
    }

    flush_lang(lc);
}

void flush_lang(LangComponents *lc)
{
    lc->demands   = NULL;
    lc->rt_dmds   = NULL;
    lc->n_demands = 0;

    lc->mx_demands   = NULL;
    lc->rt_mxi       = NULL;
    lc->rt_mxk       = NULL;
    lc->n_mx_demands = 0;

    lc->indentables = NULL;
    lc->dedentables = NULL;
    lc->n_dentables = 0;
    lc->indent_len  = 0;
}
