#ifndef BUFF_H
#define BUFF_H

#include <stdint.h>

// WHY SOMETHING FAILED; NOT WHAT FAILED
typedef enum {
    BUF_OK = 0,
    BUF_NOMEM, // no memory
    BUF_OOB,   // out-of-bounds
    BUF_NOENT, // file doesn't exit
    BUF_IO,    // disk error during read/write
    BUF_PERM,  // permission denied
    BUF_STALE, // change-conflict
    BUF_NULLC, // NULL character inserted
} BufErr;

typedef struct {
    short color;
    int mx_line_initr; // 1 if leads/starts a multiline expression
    int mx_line_killr; // 1 if terminates a multiline expression
    int is_active;     // 1 if the char is a component of an expression
    uint32_t attr;     // A_NORMAL, A_ITALICS, A_BOLD, etc.,
} Cell;

// for each line in the file
typedef struct {
    char *text;    // line content
    int len;       // cached strlen of the line
    int cap;       // line's capacity; allocated bytes; always >= len + 1
    Cell *cells;   // array of colors for the Line
    int hlite_NOK; // 1 = needs recoloring (reapply regex expressions)
} Line;

// buffer created from file on disk; mutable source of truth
typedef struct {
    Line *lines; // array of lines
    int n_lines; // lines currently in use
    int cap;     // no. of lines allocated
    int dirty;   // unsaved changes (bool)
} Buffer;

// initialize the buffer
Buffer *buffer_load(const char *path);

// make an empty buffer if empty/ doesn't exist file
int fabricate_buffer(Buffer *b);

// the four main modifications: insert, delete, split, join
int buffer_insert_char(Buffer *b, int row, int col, char c);
int buffer_delete_char(Buffer *b, int row, int col);
int buffer_split_line(Buffer *b, int row, int col);
int buffer_join_lines(Buffer *b, int row);

int buffer_duplicate_line(Buffer *b, int row);

int buffer_insert_n(Buffer *b, int row, int col, char c, int n);
int buffer_clear_n(Buffer *b, int row, int col, int n);

// writes the modified buffer to the disk (saves file)
int buffer_writeout(Buffer *b, const char *path, FILE *tmp);
// check for changes before writing out
int compare(const char *path, FILE *tmp);

// free the buffer
void free_buff(Buffer *b);

#endif
