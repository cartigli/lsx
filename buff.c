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
            // for every char read, increase, or check if needs to increase, the buffer by 1
            // this only actually means reallocing rarely since the 
            // capacity gets doubled, not incremented, on each call
            buffer_reserve(b, b->n_lines + 1);
            /* get the last line's address */
            Line *l = &b->lines[b->n_lines];
            l->text = NULL;
            l->len = 0;
            l->capacity = 0;
            l->column_colors = NULL;
            l->hlite_NOK = 1;
            l->multiline = 0;

            /* for lines longer than 4096 (identified by having no *
             * new line terminator) increase the line's reserve, update *
             * its len, and realloc more memory for the pad's new lines */
            //  for (;;) {
            while (1) {
                // chunk_len != sizeof(chunk)
                int chunk_len = (int)strlen(chunk);
                // if the chunk has a newline appended to it:
                int has_newline = (chunk_len > 0 &&
                            chunk[chunk_len - 1] == '\n');

                // if it does, 'strip' it
                if (has_newline) { chunk_len--; } /* strip the trailing '\n' */

                // reserve room for the chunk + the current length
                line_reserve(l, l->len + chunk_len + 1); /* reserve the room for the new chunk */
                // copy into the new line, starting from the current length,
                // the text from the chunk, for the length of the chunk
                memcpy(l->text + l->len, chunk, chunk_len); /* append the new chunk to the current line */
                // update the line's length
                l->len += chunk_len; /* update its length */
                // terminate the line
                l->text[l->len] = '\0'; /* and set its NULL terminator */

                // if the line had a newline escape, this line's done
                if (has_newline) { break; } /* if there was a new line, this line is fin */
                // otherwise, keep looping until fgets returns nothing
                if (!fgets(chunk, sizeof(chunk), f)) { break; } /* else, keep collecting until nothing left (EOF) */
            }
            // update the buffer's line count
            b->n_lines++;
        }
    }
    fclose(f);

    // if the loop above caught no contents & the 
    // first check failed, generate a buffer
    if (b->n_lines < 1) {
        print_err(buff_src, "file is less than 1 line long; fabricating it", 3);
        fabricate_buffer(b);
    }

    return b;
}


static void buffer_reserve(Buffer *b, int need) {
    // increase the buffer's capacity of lines

    // if the buffer can hold what is needed, there's nothing to do
    if (b->capacity >= need) {return; }
    // if it does not have enough room, start 
    // at the current capacity or 32 (arbitrary initial size)
    int new_capacity = b->capacity ? b->capacity : 32;
    // double the capacity until it can contain what is needed
    while (new_capacity < need) { new_capacity *= 2; }

    // realloc enough memory for the buffer to hold the new capacity of Lines
    Line *tmp = realloc(b->lines, new_capacity * sizeof(Line));
    if (!tmp) {
        print_err(buff_src, "failed to realloc memory for the buffer reserve", 5);
        return;
    }
    // update the buffer's lines array && its' capacity
    b->lines = tmp;
    b->capacity = new_capacity;
}


static void line_reserve(Line *l, int need) {
    // update or increase the capacity of a single line's text/content

    // if the line's capacity can fit what's needed, there's nothing to do
    if (l->capacity >= need) { return; }
    // start at the current capacity or 32 (arbitrary, above)
    int new_capacity = l->capacity ? l->capacity : 32;
    // and double the capacity until the size requirement is met
    while (new_capacity < need) { new_capacity *= 2; }

    // reallocate enough memory for the line's new capacity
    // (no sizeof() here because chars are byte-size
    char *tmp = realloc(l->text, new_capacity);
    if (!tmp) {
        print_err(buff_src, "failed to realloc memory while reserving a line in the buffer", 5);
        return;
    }
    // update the lines' text pointer && its text capacity
    l->text = tmp;
    l->capacity = new_capacity;

    // make the column_colors array match the new capacity's size & length
    short *ctmp = realloc(l->column_colors, new_capacity * sizeof(short));
    if (!ctmp) {
        print_err(buff_src, "failed to realloc memory while expanding the column_colors array", 5);
        free(tmp);
        return;
    }
    // update the line's column_colors to the expanded array
    l->column_colors = ctmp;
}


void fabricate_buffer(Buffer *b) {
    // when there is no file / empty file, generate a buffer
    // instead of making one of the file's content

    // reserve one line for the buffer
    buffer_reserve(b, 1);
    Line line;

    // give it an arbitrary (& small) line size/length
    char *tmp = calloc(1, 32);
    if (!tmp) {
        print_err(buff_src, "failed to allocate memory for the fabricated buffer", 5);
        return;
    }
    line.text = tmp;

    // initialize the empty buffer
    line.len = 0;
    line.capacity = 32;

    // allocate a column_colors array to match
    short *ctmp = calloc(1, 32 * sizeof(short));
    if (!ctmp) {
        print_err(buff_src, "failed to allocate memory for the column_colors array", 5);
        free(tmp);
        return;
    }
    line.column_colors = ctmp;

    // assign the temporary line to the buffer & update the count
    b->lines[0] = line;
    b->n_lines = 1;
}


// Four Main Editors

void buffer_insert_char(Buffer *b, int row, int col, char c) {
    // insert char <c> into lines[<row>] at the cursor's column
    // precondition: row is >= 0 && row < the no. of lines
    // precondition: col is >= 0 && col < the line's length

    Line *line = &b->lines[row];
    // reserve room for the new char (2 bytes: 1 for c, 1 for \0)
    line_reserve(line, line->len + 2);
    
    // adjust the line's current array to make room for the new char
    memmove(line->text + col + 1, // move to the new char's col + 1...
            line->text + col, // the char from the new char's col...
            line->len - col + 1); // for the length of the line - new char's col + 1
            // i.e., until the end of the line from the inserted char's column (+1 for \0)

    // insert/record the new char
    line->text[col] = c;
    // update the line's length (+1)
    line->len++;
    // mark the unsaved edits
    b->dirty = 1;
    // and indicate the line's highlighting needs to be refreshed
    line->hlite_NOK = 1;
    line->multiline = 0;
}


void buffer_delete_char(Buffer *b, int row, int col) {
    // delete the char from lines[<row>] at the cursor's column
    // precondition: col is >= to 0 && col < the line's length

    Line *l = &b->lines[row];

    // adjust the lines' array of text to fill the deleted char's 'gap'
    memmove(l->text + col, // move to the cursor's position the char from...
            l->text + col + 1, // the cursor's column + 1...
            l->len - col); // for the length of the line until the end of the line
            // no +1 ^here because the final \0 is copied with memmove's second arg's +1

    // update the line's length
    l->len--;
    // mark the unsaved changes
    b->dirty = 1;
    // and indicate the line's highlighting needs to be refreshed
    l->hlite_NOK = 1;
    l->multiline = 0;
}


void buffer_split_line(Buffer *b, int row, int col) {
    // split lines[<row>] at the cursor's column

    // reserve space for the new line in the buffer
    buffer_reserve(b, b->n_lines + 1);

    // shift the all the line's below the cursor's current row down 1
    memmove(&b->lines[row + 2], // move to the cursor's row + 2 the line from...
            &b->lines[row + 1], // the cursor's row + 1 to make space for a line at row
            // for all rows from the current row until the last line
            (b->n_lines - row - 1) * sizeof(Line));

    // source: the cursor's current row
    Line *src = &b->lines[row];
    // destination: the cursor's row + 1, i.e., below current row
    Line *dst = &b->lines[row + 1];

    // the destination length is:
    // the line's length - the cursor's current column
    int tail_len = src->len - col;

    // initialize the new line
    dst->text = NULL;
    dst->len = 0;
    dst->capacity = 0;
    dst->column_colors = NULL;

    // indicate both source & destination's highlighting needs to be refreshed
    dst->hlite_NOK = 1;
    dst->multiline = 0;
    src->hlite_NOK = 1;
    src->multiline = 0;

    // reserve the new line memory, size of original line - col + 1 for \0
    line_reserve(dst, tail_len + 1);
    // copy into the new line's text the original line's text from the col on
    // for the length of the original line - the cursor's column
    memcpy(dst->text, src->text + col, tail_len); // wouldn't tail_len + 1 copy the \0 ?

    // fill the space for the null terminator in the new line
    dst->text[tail_len] = '\0';
    // set its length
    dst->len = tail_len;

    // terminate the original line's sliced text
    src->text[col] = '\0';
    // set its new length
    src->len = col;

    // update the buffer's line count
    b->n_lines++;
    // mark the unsaved changes
    b->dirty = 1;
}


void buffer_join_lines(Buffer *b, int row) {
    // join the cursor's current line with the previous line

    // the cursor's current line/row:
    Line *current_line = &b->lines[row];
    // the line above the cursor's position to join/accept the current row
    Line *target_line = &b->lines[row - 1];

    // reserve enough room in the row's text array for the target_line line's additional size
    line_reserve(target_line, current_line->len + target_line->len + 1);

    // copy into the line above the cursor's row/line:
    memcpy(target_line->text + target_line->len, // the previous line's text (from its current length & on)
            current_line->text, // the text from the cursor's current column
            current_line->len + 1); // for the length of the cursor's current row + 1

    // update the previous line's length (+ current line's length)
    target_line->len += current_line->len;

    // free the current row/line's text
    free(current_line->text);
    // free its color index as well
    free(current_line->column_colors);

    // shift the lines from below the cursor's row & on up one
    memmove(&b->lines[row], // shift into the cursor's row
            &b->lines[row + 1], // the line's below the cursor's row
            (b->n_lines - row - 1) * sizeof(Line)); // for n lines present *after* the cursor's row

    // update the buffer's line count
    b->n_lines--;
    // mark the unsaved changes
    b->dirty = 1;

    // indicate the previous line needs to be refreshed
    b->lines[row - 1].hlite_NOK = 1;
    b->lines[row - 1].multiline = 0;
}


void buffer_duplicate_line(Buffer *b, int row) {
    // duplicate the cursor's current line (not a main editor)

    // reserve room for the new line in the buffer
    buffer_reserve(b, b->n_lines + 1);
    // shift the lines from the cursor's current line down 2 (+ 2)
    memmove(&b->lines[row + 2], // shift to the line down two from the current row/line
            &b->lines[row + 1], // the lines from the current row down 1
            (b->n_lines - row - 1) * sizeof(Line)); // for the number of rows left in the file
            // to make room or 'space' for the new line

    // source: cursor's current row
    Line *src = &b->lines[row];
    // destination: the next row
    Line *dst = &b->lines[row + 1];

    // initialize the new line's values
    dst->text = NULL;
    dst->len = 0;
    dst->capacity = 0;
    dst->column_colors = NULL;
    dst->hlite_NOK = 1;
    dst->multiline = 0;

    // reserve room in the new line for the current
    // line's content to be duplicated + \0
    line_reserve(dst, src->len + 1);
    // copy into this new line the current line's text array
    memcpy(dst->text, src->text, src->len + 1); // + 1 for the \0
    // set the new line's length
    dst->len = src->len;

    // update the buffer's line count
    b->n_lines++;
    // mark the unsaved changes
    b->dirty = 1;
}


int buffer_writeout(Buffer *b, const char *path) {
    // write out the modified buffer (save the file/edits)

    char tmp[1024];
    // make a temporary file named by unitialized garbage values - likely unused
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);

    // open & write out the modified buffer to the temporary file
    FILE *f = fopen(tmp, "w");
    if (!f) {
        print_err(buff_src, "failed to open file to writeout buffer", 5);
        return 1;
    }
    // write out the modified buffer, line by line
    for (int i = 0; i < b->n_lines; i++) {
        fputs(b->lines[i].text, f);
        /* add a trailing \n to all lines, including the last line */
        // if (i < b->n_lines - 1) {
        fputc('\n', f);
        // }
    }
    fclose(f);

    // rename to original name -- POSIX atomicy
    if (rename(tmp, path) != 0) {
        print_err(buff_src, "failed to rename tmp", 5);
        return 1;
    }
    // mark saved changes
    b->dirty = 0;
    return 0;
}

void free_buff(Buffer *b) {
    for (int i = 0; i < b->n_lines; i++) {
        free(b->lines[i].text);
        free(b->lines[i].column_colors);
    }
    free(b->lines);
    free(b);
}