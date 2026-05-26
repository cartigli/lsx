#ifndef BUFF_H
#define BUFF_H

/* for each line in the file */
typedef struct {
    char *data; /* line content */
    int    len; /* cached strlen of the line */
    int    cap; /* line's capacity; allocated bytes; always >= len + 1 */
    // int hlstate; /* computed regex or not */
} Line;

/* buffer created from file on disk; mutable source of truth */
typedef struct {
    Line   *lines; /* array of lines                   */
    int   n_lines; /* lines currently in use           */
    int cap_lines; /* no. of lines currently allocated */
    int     dirty; /* unsaved changes (bool)           */
} Buffer;

/* initialize the buffer */
Buffer *buffer_load(const char *path);

/* make an empty buffer if empty/ doesn't exist file */
void fabricate_buffer(Buffer *b);

/* the four main modifications: insert, delete, split, join */
void buffer_insert_char(Buffer *b, int row, int col, char c);
void buffer_delete_char(Buffer *b, int row, int col);
void buffer_split_line(Buffer *b, int row, int col);
void buffer_join_lines(Buffer *b, int row);

void buffer_duplicate_line(Buffer *b, int row);

/* writes the modified buffer to the disk (saves file) */
int buffer_writeout(Buffer *b, const char *path);

/* free the buffer */
void free_buff(Buffer *b);

#endif