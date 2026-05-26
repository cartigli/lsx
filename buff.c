#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buff.h"
#include "error.h"

/* prototypes for static & local only functions *
 * add or expand memory for the modified buffer */
static void line_reserve(Line *l, int need);
static void buffer_reserve(Buffer *b, int need);


Buffer *buffer_load(const char *path) {
    Buffer *b = calloc(1, sizeof(Buffer));
    if (!b) {
        print_err(buff_src, "failed to allocate memory for the initial buffer", 5);
        return NULL;
    }

    FILE *f = fopen(path, "r");
    if (!f) {
        print_err(buff_src, "file does not exist/permission denied; fabricating it", 3);
        fabricate_buffer(b);
        return b;
    }

    else {
        buffer_reserve(b, 32);

        char chunk[4096];
        while (fgets(chunk, sizeof(chunk), f)) {
            /* the line returned could be truncated if greater *
             * than 4096 checking for the EOL terminator & growing *
             * the line resolves this */
            buffer_reserve(b, b->n_lines + 1);
            /* get the last line's address */
            Line *l = &b->lines[b->n_lines];
            l->data = NULL;
            l->len = 0;
            l->cap = 0;

            /* for lines longer than 4096 (identified by having no *
             * new line terminator) increase the line's reserve, update *
             * its len, and realloc more memory for the pad's new lines */
             for (;;) { 
                int chunk_len = (int)strlen(chunk);
                int has_newline = (chunk_len > 0 &&
                            chunk[chunk_len - 1] == '\n');

                if (has_newline) { chunk_len--; } /* strip the trailing '\n' */

                line_reserve(l, l->len + chunk_len + 1); /* reserve the room for the new chunk */
                memcpy(l->data + l->len, chunk, chunk_len); /* append the new chunk to the current line */
                l->len += chunk_len; /* update its length */
                l->data[l->len] = '\0'; /* and set its NULL terminator */

                if (has_newline) { break; } /* if there was a new line, this line is fin */
                if (!fgets(chunk, sizeof(chunk), f)) { break; } /* else, keep collecting until nothing left (EOF) */
            }
            b->n_lines++;
        }
    }
    fclose(f);

    if (b->n_lines < 1) {
        print_err(buff_src, "file is less than 1 line long; fabricating it", 3);
        fabricate_buffer(b);
    }

    return b;
}


void fabricate_buffer(Buffer *b) {
    buffer_reserve(b, 1);
    Line line;

    char *tmp = calloc(1, 16);
    if (!tmp) {
        print_err(buff_src, "failed to allocate memory for the fabricated buffer", 5);
        return;
    }
    line.data = tmp;
    line.len = 0;
    line.cap = 16;
    b->lines[0] = line;
    b->n_lines = 1;
}

/* update or increase the capacity of a single line */
static void line_reserve(Line *l, int need) {
    if (l->cap >= need) { return; } /* if the lnes capacity is greater than the need, its already fine */
    int new_cap = l->cap ? l->cap : 16; /* if no current cap, set to 16 */

    while (new_cap < need) { new_cap *= 2; } /* if still inadequate, double until it is */
    char *tmp = realloc(l->data, new_cap); /* realloc the new line size */
    if (!tmp) {
        print_err(buff_src, "failed to realloc memory while reserving a line in the buffer", 5);
        return;
    }
    l->data = tmp;
    l->cap = new_cap; /* & update the line's capacity */
}

/* update or increase the capacity of lines in the buffer */
static void buffer_reserve(Buffer *b, int need) {
    if (b->cap_lines >= need) {return; } /* same idea here but for no. of lines in the Buffer */
    int new_cap = b->cap_lines ? b->cap_lines : 32;
    while (new_cap < need) { new_cap *= 2; }

    Line *tmp = realloc(b->lines, new_cap * sizeof(Line));
    if (!tmp) {
        print_err(buff_src, "failed to realloc memory for the buffer reserve", 5);
        return;
    }
    b->lines = tmp;
    b->cap_lines = new_cap;
}


/* four main editors */

/* insert char 'c' into line 'row' at column 'col' *
 * precondition: 0<= row < n_lines, 0 <= col <= lines[row].len */
void buffer_insert_char(Buffer *b, int row, int col, char c) {
    Line *l = &b->lines[row];    /* 'id' the row being modified */
    line_reserve(l, l->len + 2); /* + 1 for c, + 1 for \0 */
    memmove(l->data + col + 1,   /* shift tail right */
            l->data + col,       /* start of text to shift; posit. in line where 'c' will be inserted */
            l->len - col + 1);   /* + 1 copies the \0 as well */
    l->data[col] = c;            /* add the newly inserted char 'c' where there is now space */
    l->len++;                    /* add 1 to the length of the given row */
    b->dirty = 1;                /* mark the changes as unsaved */
}


/* delete the char at (row, col)
 * precondition: 0 <= col < lines[row].len */
void buffer_delete_char(Buffer *b, int row, int col) {
    Line *l = &b->lines[row]; /* twin to above but inverse */
    memmove(l->data + col,
            l->data + col + 1,
            l->len - col);
    l->len--;
    b->dirty = 1;
}


/* split line: row at 'col'; text from 'col' on becomes a new line */
void buffer_split_line(Buffer *b, int row, int col) {
    buffer_reserve(b, b->n_lines + 1); /* add another line to the file's buffer (array of lines) */
    memmove(&b->lines[row + 2],        /* shift lines below 'row' down one */
            &b->lines[row + 1],
            (b->n_lines - row - 1) * sizeof(Line));

    Line *src = &b->lines[row];
    Line *dst = &b->lines[row + 1];
    int tail_len = src->len - col;     /* tail end is the current col - line length */

    dst->data = NULL;
    dst->len = 0;
    dst->cap = 0;

    line_reserve(dst, tail_len + 1); /* make room for the new tail-end of the original line */
    memcpy(dst->data, src->data + col, tail_len); /* copy the tail end in */
    dst->data[tail_len] = '\0';      /* add a null terminator to the end of the new dest */
    dst->len = tail_len;             /* set it to its split length */

    src->data[col] = '\0';           /* add a null terminator at 'col' of the original line */
    src->len = col;                  /* set it to the length of the line remaining */

    b->n_lines++;                    /* add a line to the total count */
    b->dirty = 1;                    /* mark the unsaved changes */
}


void buffer_duplicate_line(Buffer *b, int row) {
    buffer_reserve(b, b->n_lines + 1);
    memmove(&b->lines[row + 2],
            &b->lines[row + 1],
            (b->n_lines - row - 1) * sizeof(Line));

    Line *src = &b->lines[row];
    Line *dst = &b->lines[row + 1];

    dst->data = NULL;
    dst->len = 0;
    dst->cap = 0;

    line_reserve(dst, src->len + 1);
    memcpy(dst->data, src->data, src->len + 1); /* + 1 for the '\0' */
    dst->len = src->len;

    b->n_lines++;
    b->dirty = 1;
}


void buffer_join_lines(Buffer *b, int row) {
    if (row + 1 >= b->n_lines) { return; }
    Line *l = &b->lines[row];
    Line *next = &b->lines[row + 1];

    /* add row 1's length to row 0 */
    line_reserve(l, l->len + next->len + 1);
    memcpy(l->data + l->len, next->data, next->len + 1);
    l->len += next->len;

    free(next->data);

    /* shift lines above the gap down */
    memmove(&b->lines[row + 1],
            &b->lines[row + 2],
            (b->n_lines - row - 2) * sizeof(Line));

    b->n_lines--;
    b->dirty = 1;
}


int buffer_writeout(Buffer *b, const char *path) {
    char tmp[1024];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);

    FILE *f = fopen(tmp, "w");
    if (!f) {
        print_err(buff_src, "failed to open file to writeout buffer", 5);
        return 1;
    }
    for (int i = 0; i < b->n_lines; i++) {
        fputs(b->lines[i].data, f);
        /* add a trailing \n to the last line */
        if (i < b->n_lines - 1) {
            fputc('\n', f);
        }
    }
    fclose(f);

    if (rename(tmp, path) != 0) {
        print_err(buff_src, "failed to rename tmp", 5);
        return 1;
    }
    b->dirty = 0;
    return 0;
}

void free_buff(Buffer *b) {
    for (int i = 0; i < b->n_lines; i++) { free(b->lines[i].data); }
    free(b->lines);
    free(b);
}