#include <stdio.h>
#include <string.h>

#include "buff.h"
#include "error.h"

#include "alloc_shim.h"

static int line_reserve(Line *l, int need);
static int buffer_reserve(Buffer *b, int need);

Buffer *buffer_load(const char *path)
{
    // initializes a buffer from a given path found on disk
    // if empty/non-existent file, then 'fabricate' an empty buffer

    Buffer *b = calloc(1, sizeof(Buffer));
    if (!b) {
        LOG_CRIT("failed to initialize the buffer");
        return NULL;
    }

    FILE *f = fopen(path, "r");
    if (!f) {
        LOG_WARN("couldn't open file; fabricating buffer");
        if (fabricate_buffer(b)) goto fail;
        return b;
    }

    if (buffer_reserve(b, 32)) goto fail_wfile;

    // the line returned could be truncated if greater than 4096,
    // checking for the EOL terminator & growing the line resolves this
    char chunk[4096];
    while (fgets(chunk, sizeof(chunk), f)) {
        if (buffer_reserve(b, b->n_lines + 1)) goto fail_wfile;

        Line *l      = &b->lines[b->n_lines];
        l->len       = 0;    // length of 0
        l->cap       = 0;    // cap of 0
        l->hlite_NOK = 1;    // needs refreshing
        l->cells     = NULL; // NULL color cells
        l->text      = NULL; // initialized line: NULL text,

        // for lines longer than 4096 (identified by having no
        // new line terminator) increase the line's reserve, update
        // its len, and realloc more memory for the pad's new lines
        while (1) {
            // if the chunk has a newline appended to it:
            int chunk_len = (int)strlen(chunk);
            int has_newline =
                (chunk_len > 0 && chunk[chunk_len - 1] == '\n');
            if (has_newline) chunk_len--;// strip it

            // reserve room for the chunk + the current length
            if (line_reserve(l, l->len + chunk_len + 1)) {
                // this line is allocated/populated, but its not
                // counted in b->n_lines yet; free it before it leaks
                // because free_buff() won't know to look for it
                free(l->text);
                free(l->cells);
                goto fail_wfile;
            }
            // copy into the new line, starting from the current length,
            // the text from the chunk, for the length of the chunk
            memcpy(l->text + l->len, chunk, chunk_len);
            l->len += chunk_len; // update the line's length
            l->text[l->len] = '\0'; // terminate the line

            // if the line had a newline escape, this line's done
            if (has_newline) break;
            // otherwise, keep looping until fgets returns nothing
            if (!fgets(chunk, sizeof(chunk), f)) break;
        }
        // increment the line count
        b->n_lines++;
    }

    fclose(f);

    // if the loop above caught no contents & the
    // first check failed, generate a buffer
    if (b->n_lines < 1) {
        LOG_WARN("empty file; fabricating buffer");
        if (fabricate_buffer(b)) goto fail;
        return b;
    }
    return b;

fail_wfile:
    fclose(f);
fail:
    free_buff(b);
    return NULL;
}

static int buffer_reserve(Buffer *b, int need)
{
    // increase the buffer's cap of lines

    // if the buffer can hold what is needed, there's nothing to do
    if (b->cap >= need) return BUF_OK;

    // if it does not have enough room, start at the
    // current cap or 32 (arbitrary initial size)
    int new_cap = b->cap ? b->cap : 32;
    // double the cap until it can contain what is needed
    while (new_cap < need) new_cap *= 2;

    // realloc enough memory for the buffer to hold the new cap of Lines
    Line *tmp = realloc(b->lines, new_cap * sizeof(Line));
    if (!tmp) {
        LOG_CRIT("failed to reserve buffer");
        return BUF_NOMEM;
    }

    // update the buffer's lines array && its' cap
    b->lines = tmp;
    b->cap   = new_cap;

    return BUF_OK;
}

static int line_reserve(Line *l, int need)
{
    // update or increase the cap of a single line's text/content

    // if the line's cap can fit what's needed, there's nothing to do
    if (l->cap >= need) return BUF_OK;

    // start at the current cap or 32 (arbitrary, above)
    int new_cap = l->cap ? l->cap : 32;

    // and double the cap until the size requirement is met
    while (new_cap < need) new_cap *= 2;

    // reallocate enough memory for the line's new cap
    // (no sizeof here because chars are byte-size
    char *tmp = realloc(l->text, new_cap);
    if (!tmp) {
        LOG_CRIT("failed to expand line capacity");
        return BUF_NOMEM;
    }

    // make the column_colors array match the new cap's size & length
    Cell *tcells = realloc(l->cells, new_cap * sizeof(Cell));
    if (!tcells) {
        LOG_CRIT("failed to expland colors array");
        free(tmp);
        return BUF_NOMEM;
    }
    // update the lines' text pointer && its text cap
    // *after both reallocs are checked
    l->text = tmp;
    l->cap  = new_cap;

    // update the line's column_colors to the expanded array
    l->cells = tcells;

    return BUF_OK;
}

int fabricate_buffer(Buffer *b)
{
    // when there is no file / empty file, generate a buffer
    // instead of making one of the file's content

    // reserve one line for the buffer
    if (buffer_reserve(b, 1)) {
        LOG_CRIT("NOMEM; failed reserve buffer while fabricating");
        return BUF_NOMEM;
    }

    Line line = { .hlite_NOK = 1 };
    if (line_reserve(&line, 1)) {
        LOG_CRIT("NOMEM; failed to reserve line while fabricating");
        return BUF_NOMEM;
    }
    line.text[0] = '\0';
    
    // assign the temporary line to the buffer & update the count
    b->lines[0] = line;
    b->n_lines  = 1;

    return BUF_OK;
}

// Four Main Editors

int buffer_insert_char(Buffer *b, int row, int col, char c)
{
    /* insert char <c> into lines[<row>] at the cursor's column
     * precondition: row is >= 0 && row < the no. of lines
     * precondition: col is >= 0 && col < the line's length 
     * precondition: c is NOT NULL */

     if (c == '\0') {
        LOG_ERRO("NULL char inserted");
        return BUF_NULLC;
     }
    if (row < 0 || row >= b->n_lines) {
        LOG_ERRO("illegal row");
        return BUF_OOB;
    }
    if (col < 0 || col > b->lines[row].len) {
        LOG_ERRO("illegal column");
        return BUF_OOB;
    }

    Line *line = &b->lines[row];

    // reserve room for the new char (2 bytes: 1 for c, 1 for \0)
    if (line_reserve(line, line->len + 2)) return BUF_NOMEM;

    // adjust the line's current array to make room for the new char
    memmove(line->text + col + 1, // move to the new char's col + 1...
        line->text + col,         // ...the char from the new char's col
        line->len - col + 1); // for the length of the line - new char's col + 1
    // i.e., until the EOL from the inserted char's column (+ 1 for \0)

    // insert/record the new char
    line->text[col] = c;
    // update the line's length (+1)
    line->len++;
    // mark the unsaved edits
    b->dirty = 1;
    // and indicate the line's highlighting needs to be refreshed
    line->hlite_NOK = 1;

    return BUF_OK;
}

int buffer_insert_n(Buffer *b, int row, int col, char c, int n)
{
    // insert char <c> n times from col to col + n on lines[<row>]
    // reserve the necessary space & adjus the line as needed, ofc

    if (c == '\0') {
        LOG_ERRO("NULL char inserted_n");
        return BUF_NULLC;
    }
    if (n <= 0) {
        LOG_ERRO("illegal n-span");
        return BUF_OOB;
    }
    if (row < 0 || row >= b->n_lines) {
        LOG_ERRO("illegal row");
        return BUF_OOB;
    }
    if (col < 0 || col > b->lines[row].len) {
        LOG_ERRO("illegal column");
        return BUF_OOB;
    }

    Line *line = &b->lines[row];

    // reserve room for n bytes (+ 1 for \0)
    if (line_reserve(line, line->len + n + 1)) return BUF_NOMEM;

    memmove(line->text + col + n, // move to the pos. in line + n...
        line->text + col,         // ...the bytes from the cusor's col
        line->len - col + n + 1); // for line.len - col_len + n bytes

    // set the entire 'block' n to char c
    memset(line->text + col, c, n);

    line->len += n;      // update the new length
    b->dirty        = 1; // mark unsaved edits
    line->hlite_NOK = 1; // brand the line for refreshing

    return BUF_OK;
}

int buffer_delete_char(Buffer *b, int row, int col)
{
    // delete the char from lines[<row>] at the cursor's column
    // precondition: col is >= to 0 && col < the line's length

    if (row < 0 || row >= b->n_lines) {
        LOG_ERRO("illegal row");
        return BUF_OOB;
    }
    if (col < 0 || col >= b->lines[row].len) {
        LOG_ERRO("illegal column");
        return BUF_OOB;
    }

    Line *line = &b->lines[row];

    // adjust the lines' array of text to fill the deleted char's 'gap'
    memmove(line->text + col, // move to the cursor's position the char from...
        line->text + col + 1, // the cursor's column + 1...
        line->len - col); // for the length of the line until the end of the line
    // no +1 ^here because the final \0 is copied with memmove's second arg +1

    // update the line's length
    line->len--;
    // mark the unsaved changes
    b->dirty = 1;
    // and indicate the line's highlighting needs to be refreshed
    line->hlite_NOK = 1;

    return BUF_OK;
}

int buffer_clear_n(Buffer *b, int row, int col, int n)
{
    // deletes chars from lines[<row>] at cursor's column
    // col: start of line to clear; n: bytes to clear

    if (n <= 0) {
        LOG_ERRO("illegal n-span");
        return BUF_OOB;
    }
    if (row < 0 || row >= b->n_lines) {
        LOG_ERRO("illegal row");
        return BUF_OOB;
    }
    if (col < 0 || col > b->lines[row].len) {
        LOG_ERRO("illegal column");
        return BUF_OOB;
    }
    if (col + n > b->lines[row].len) {
        LOG_ERRO("illegal n-span length");
        return BUF_OOB;
    }

    Line *line = &b->lines[row];

    memmove(line->text + col, line->text + col + n,
        line->len - col - n + 1); // carry the \0

    line->len -= n;
    b->dirty        = 1;
    line->hlite_NOK = 1;

    return BUF_OK;
}

int buffer_split_line(Buffer *b, int row, int col)
{
    // split lines[<row>] at the cursor's colum

    if (row < 0 || row >= b->n_lines) {
        LOG_ERRO("illegal row");
        return BUF_OOB;
    }
    if (col < 0 || col > b->lines[row].len) {
        LOG_ERRO("illegal column");
        return BUF_OOB;
    }

    int tail_len = b->lines[row].len - col;

    Line tmp = { .hlite_NOK = 1 };

    if (line_reserve(&tmp, tail_len + 1)) return BUF_NOMEM;
    
    // reserve space for the new line in the buffer
    if (buffer_reserve(b, b->n_lines + 1)) {
        free(tmp.text);
        free(tmp.cells);
        return BUF_NOMEM;
    }

    Line *src = &b->lines[row];

    memcpy(tmp.text, src->text + col, tail_len);
    tmp.text[tail_len] = '\0';
    tmp.len = tail_len;

    // shift the lines below the current line down 1
    memmove(&b->lines[row + 2], // move to the cursor's row down 2 from...
        &b->lines[row + 1], // the line underneath the previous line
        (b->n_lines - row - 1) * sizeof(Line)); // for all remaining lines

    b->lines[row + 1] = tmp;

    src->len = col;
    src->text[col] = '\0';
    src->hlite_NOK = 1;

    b->n_lines++;
    b->dirty = 1;

    return BUF_OK;
}

int buffer_join_lines(Buffer *b, int row)
{
    // join the cursor's current line with the previous line

    // if (row == 0) return BUF_OOB;
    if (row <= 0 || row >= b->n_lines) {
        LOG_ERRO("illegal row");
        return BUF_OOB;
    }

    // the cursor's current line/row:
    Line *current_line = &b->lines[row];
    // the line above the cursor's position to join/accept the current row
    Line *target_line = &b->lines[row - 1];

    // reserve enough room in the row's text array
    // for the target_line line's additional size
    if (line_reserve(target_line, current_line->len + target_line->len + 1)) {
        return BUF_NOMEM;
    }

    // copy into the line above the cursor's row/line:
    memcpy(target_line->text + // the previous line's text
            target_line->len,  // (from its current length & on)
        current_line->text,    // the text from the cursor's current column
        current_line->len + 1  // for the length of the cursor's current row + 1
    );

    // update the previous line's length
    target_line->len += current_line->len;

    // free the current row/line's text
    free(current_line->text);
    // free its color index as well
    free(current_line->cells);

    // shift the lines from below the cursor's row & on up one
    memmove(&b->lines[row], // shift into the cursor's row
        &b->lines[row + 1], // the line's below the cursor's row
        // for n lines present *after* the cursor's row
        (b->n_lines - row - 1) * sizeof(Line));

    // update the buffer's line count
    b->n_lines--;
    // mark the unsaved changes
    b->dirty = 1;
    // indicate the previous line needs to be refreshed
    b->lines[row - 1].hlite_NOK = 1;

    return BUF_OK;
}

int buffer_duplicate_line(Buffer *b, int row)
{
    // duplicate the cursor's current line (not a main editor)

    if (row < 0 || row >= b->n_lines) {
        LOG_ERRO("illegal row");
        return BUF_OOB;
    }

    Line tmp = { .hlite_NOK = 1 };

    Line *src = &b->lines[row];

    if (line_reserve(&tmp, src->len + 1)) return BUF_NOMEM;

    // reserve room for the new line in the buffer
    if (buffer_reserve(b, b->n_lines + 1)) {
        free(tmp.text);
        free(tmp.cells);
        return BUF_NOMEM;
    }

    memcpy(tmp.text, src->text, src->len);
    tmp.text[src->len] = '\0';
    tmp.len = src->len;

    // shift the lines below the current line down 1
    memmove(&b->lines[row + 2], // shift to the line down two...
        &b->lines[row + 1], // the lines from down 1
        (b->n_lines - row - 1) * sizeof(Line)); // for all lines

    b->lines[row + 1] = tmp;

    src->hlite_NOK = 1;

    b->n_lines++;
    b->dirty = 1;

    return BUF_OK;
}

int buffer_writeout(Buffer *b, const char *path, FILE *orig)
{
    // write out the modified buffer (save the file)

    // check if changes have been made to the file since opening & editing
    if (compare(path, orig)) {
        LOG_WARN("original file has been altered");
        return BUF_STALE;
    }

    char tmp[1024];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);

    // open & write out the modified buffer to the temporary file
    FILE *f = fopen(tmp, "w");
    if (!f) {
        LOG_ERRO("failed to open file to writeout buffer");
        return BUF_IO;
    }

    // write out the modified buffer, line by line
    for (int i = 0; i < b->n_lines; i++) {
        fputs(b->lines[i].text, f);
        // add a trailing \n to all lines, including the last line
        fputc('\n', f);
    }
    fclose(f);

    // rename to original name -- POSIX atomicy
    if (rename(tmp, path) != 0) {
        LOG_CRIT("atomic rename operation failed");
        return BUF_IO;
    }

    // mark saved changes
    b->dirty = 0;
    return BUF_OK;
}

#define BUFFSZ 4096

int compare(const char *path, FILE *tmp)
{
    // compare the original copy to the current file
    // if changes have been made, writeout is blocked

    FILE *src = fopen(path, "rb");
    if (!src) {
        LOG_ERRO("%s deleted or moved; comparison failed", path);
        return BUF_IO;
    }

    rewind(tmp);

    char p_buff[BUFFSZ];
    char t_buff[BUFFSZ];
    size_t p_sz, t_sz;

    while (1) {
        p_sz = fread(p_buff, 1, BUFFSZ, src);
        t_sz = fread(t_buff, 1, BUFFSZ, tmp);

        if (p_sz != t_sz) {
            fclose(src);
            LOG_WARN("difference in sizes detected");
            return BUF_STALE;
        }

        if (p_sz == 0) {
            if (ferror(src) || ferror(tmp)) {
                fclose(src);
                LOG_ERRO("read error while comparing original");
                return BUF_IO;
            }
            break;
        }

        if (memcmp(p_buff, t_buff, p_sz) != 0) {
            fclose(src);
            LOG_WARN("difference in content detected");
            return BUF_STALE;
        }
    }

    fclose(src);
    return BUF_OK;
}

void free_buff(Buffer *b)
{
    if (b == NULL) return;

    for (int i = 0; i < b->n_lines; i++) {
        free(b->lines[i].text);
        free(b->lines[i].cells);
    }
    free(b->lines);
    free(b);
}
