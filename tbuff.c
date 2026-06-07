#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tbuff.h"
#include "error.h"

static int g_tests = 0, g_fails = 0;

int g_fail_at = -1;
int g_alloc_n = 0;

#define CHECK(cond, msg) do { \
    g_tests++; \
    if (!(cond)) { \
        g_fails++; \
        fprintf(stdout, "[FAIL] %s:%d %s\n", __FILE__, __LINE__, msg); \
    } } while (0)
    // } else {
        // fprintf(stdout, "[PASS] %s:%d %s\n", __FILE__, __LINE__, msg);

#define CHECK_INT(got, want, msg) do { \
    g_tests++; \
    long _g=(long)(got), _w=(long)(want); \
    if (_g != _w) { g_fails++; \
        fprintf(stdout, "[FAIL] %s:%d  %s (got %ld, want %ld)\n", __FILE__, __LINE__, msg, _g, _w); \
    } } while (0)
    // } else {
        // fprintf(stdout, "[PASS] %s:%d  %s (got %ld, want %ld)\n", __FILE__, __LINE__, msg, _g, _w);

#define CHECK_LINE(b, row, want) do { \
    g_tests++; \
    if (strcmp(b->lines[row].text, want) != 0) { \
        g_fails++; \
        fprintf(stdout, "[FAIL] LINE MISMATCH: have=%s wanted=%s\n", b->lines[row].text, want); \
    } } while (0)
    // } else {
        // fprintf(stdout, "[PASS] LINE MATCH: have=%s wanted=%s\n", b->lines[row].text, want);

static int should_fail(void)
{
    int n = g_alloc_n++;
    return (g_fail_at >= 0 && n == g_fail_at);
}

void *test_malloc(size_t s) { return should_fail() ? NULL : malloc(s); }
void *test_realloc(void *p, size_t s) { return should_fail() ? NULL : realloc(p, s); }
void *test_calloc(size_t n, size_t s) { return should_fail() ? NULL : calloc(n, s); }

static int validate_buffer(const Buffer *b)
{
    // returns 1 if every documented invariant holds

    int ok = 1;
    if (b->n_lines < 0 || b->n_lines > b->cap) {
        fprintf(stdout, "INVARIANT: n_lines=%d cap=%d\n", b->n_lines, b->cap);
        ok = 0;
    }
    if (b->lines == NULL) {
        fprintf(stdout, "INVARIANT: b->lines == NULL\n");
        ok = 0;
    }
    // if (b->dirty != 0) {
    //     fprintf(stdout, "INVARIANT: b->dirty == 1\n");
    //     ok = 0;
    // }
    if (b->cap < 0) {
        fprintf(stdout, "INVARIANT: b->cap <= 0\n");
        ok = 0;
    }
    for (int i = 0, n = b->n_lines; i < n; i++) {
        const Line *line = &b->lines[i];

        if (!line->text) {
            fprintf(stdout, "INVARIANT: line=%d text=NULL\n", i);
            ok = 0;
            continue;
        }
        if (line->cap < line->len + 1) {
            fprintf(stdout, "INVARIANT: line=%d cap=%d < len + 1=%d\n",
                i, line->cap, line->len + 1);
            ok = 0;
            continue;
        }
        if (line->text[line->len] != '\0') {
            fprintf(stdout, "INVARIANT: line=%d no null terminator\n", i);
            ok = 0;
            continue;
        }

        if ((int)strlen(line->text) != line->len) {
            fprintf(stdout, "INVARIANT: line=%d strlen=%d line->len=%d\n",
                i, (int)strlen(line->text), line->len);
            ok = 0;
            continue;
        }
        if (line->cap > 0 && line->cells == NULL) {
            fprintf(stdout, "INVARIANT: NULL line->cells with b->cap > 0\n");
            ok = 0;
            continue;
        }

    }
    return ok;
}

static Buffer *mem_buffer(const char **src, int n)
{
    // purely in-memory; no buffer_load() or file-paths

    Buffer *b = NULL;
    b = calloc(1, sizeof(Buffer));
    if (!b) { perror("NOMEM; fab_buff()\n"); return NULL; }

    if (fabricate_buffer(b) != BUF_OK) { free(b); return NULL; }

    for (int i = 0; i < n; i++) {
        int len = (int)strlen(src[i]);
        for (int c = 0; c < len; c++) {
            CHECK_INT(buffer_insert_char(b, i, c, src[i][c]), BUF_OK, "mem_buffer insert char");
        }
        if (i < n - 1) {
            CHECK_INT(buffer_split_line(b, i, len), BUF_OK, "mem_buffer split line");
        }
    }
    // b->dirty = 0;
    return b;
}

static Buffer *text_buffer(const char *contents)
{
    // buffer load from a file (generated internally)

    // char path[] = "/Volumes/HomeXx/compuir/test.txt";
    char path[] = "/tmp/buff_loadXXXXXX";
    int fd = mkstemp(path);
    FILE *f = fdopen(fd, "w");
    fputs(contents, f);
    fclose(f);

    Buffer *b = NULL;
    b = buffer_load(path);
    if (!b) { perror("BUFFER LOAD FAILED\n"); return NULL; }
    CHECK_INT(b->dirty, 0, "new buffer loaded with dirty flag");

    unlink(path);
    return b;
}

static void test_load(void)
{
    // test generating a single empty & long line buffer (from file)

    Buffer *b = NULL;
    b = text_buffer("");
    if (!b) {
        perror("FAILED TO GEN BUFFER FROM TEXT\n");
        return;
    }
    CHECK(b != NULL, "text_buffer returned non-empty buffer");
    CHECK_INT(b->n_lines, 1, "empty file creates 1-line buffer");
    CHECK_INT(b->dirty, 0, "new mem buffer is not dirty");
    CHECK_LINE(b, 0, "");
    CHECK(validate_buffer(b), "empty text-buffer valid");
    free_buff(b);

    b = NULL;
    char big[5001];
    memset(big, 'x', 5000);
    big[5000] = '\0';

    b = text_buffer(big);
    if (!b) {
        perror("FAILED TO GEN BUFFER FROM TEXT\n");
        return;
    }
    CHECK_INT(b->n_lines, 1, "long-line stays one line");
    CHECK_INT(b->lines[0].len, 5000, "long-line is full length");
    CHECK_INT(b->dirty, 0, "long-line buffer is not dirty");
    CHECK(validate_buffer(b), "long-line-buffer valid");
    free_buff(b);
}

static void test_line_lim(void)
{
    // test buffer loading lines at or on the chunk size in buffer_load()
    // buffer_load() reads in 4096 chunk bytes + a null terminator

    Buffer *b = NULL;

    // case a: 4095 bytes & no newline
    char biga[4096];
    memset(biga, 'x', 4095);
    biga[4095] = '\0';

    b = text_buffer(biga);
    if (!b) {
        perror("FAILED TO GEN BUFFER FROM TEXT\n");
        return;
    }
    CHECK_INT(b->n_lines, 1, "long-line stays one line");
    CHECK_INT(b->lines[0].len, 4095, "4095-len line (no newline) is full length");
    CHECK_INT(b->dirty, 0, "4095-len line (no newline) buffer not dirty");
    CHECK(validate_buffer(b), "long line-buffer valid");
    free_buff(b);

    // case b: 4095 bytes & newline
    b = NULL;
    char bigb[4096];
    memset(bigb, 'x', 4095);
    bigb[4094] = '\n';
    bigb[4095] = '\0';

    b = text_buffer(bigb);
    if (!b) {
        perror("FAILED TO GEN BUFFER FROM TEXT\n");
        return;
    }
    CHECK_INT(b->n_lines, 1, "long-line stays one line");
    CHECK_INT(b->lines[0].len, 4094, "4095-len line (newline) is full length");
    CHECK_INT(b->dirty, 0, "4095-len line (newline) buffer not dirty");
    CHECK(validate_buffer(b), "long line-buffer valid");
    free_buff(b);

    // case c: 4096 bytes with newline;
    // \n is first byte of next chunk
    b = NULL;
    char bigc[4097];
    memset(bigc, 'x', 4096);
    bigc[4095] = '\n';
    bigc[4096] = '\0';

    b = text_buffer(bigc);
    if (!b) {
        perror("FAILED TO GEN BUFFER FROM TEXT\n");
        return;
    }
    CHECK_INT(b->n_lines, 1, "long-line stays one line");
    CHECK_INT(b->lines[0].len, 4095, "4096-len line (newline) is full length");
    CHECK_INT(b->dirty, 0, "4096-len line (newline) buffer not dirty");
    CHECK(validate_buffer(b), "long line-buffer valid");
    free_buff(b);
}

static void test_editor_x(void)
{
    // test basic insert & delete behavior

    const char *src[] = { "hello world", "second" };
    Buffer *b = NULL;
    b = mem_buffer(src, 2);
    if (!b) { perror("FAILED TO MAKE BUFFER\n"); return; }

    // insert ',' at col=5: hello world -> hello, world
    CHECK_INT(buffer_insert_char(b, 0, 5, ','), BUF_OK, "insert OK");
    CHECK_LINE(b, 0, "hello, world");
    CHECK_INT(b->lines[0].len, 12, "len after insert");
    CHECK_INT(b->dirty, 1, "show unsaved changes after inserted char");

    // delete ',' at col=5: hello, world -> hello world
    CHECK_INT(buffer_delete_char(b, 0, 5), BUF_OK, "delete OK");
    CHECK_LINE(b, 0, "hello world");
    CHECK_INT(b->lines[0].len, 11, "len after delete");
    CHECK_INT(b->dirty, 1, "show unsaved changes after deleted char");

    // insert 3 dashes at col=0: hello world -> ---hello world
    CHECK_INT(buffer_insert_n(b, 0, 0, '-', 3), BUF_OK, "insert_n OK");
    CHECK_LINE(b, 0, "---hello world");

    // clear 3 dashes from col=3: ---hello world -> hello world
    CHECK_INT(buffer_clear_n(b, 0, 0, 3), BUF_OK, "clear_n OK");
    CHECK_LINE(b, 0, "hello world");

    // split line at col=5: hello world -> hello\n world
    CHECK_INT(buffer_split_line(b, 0, 5), BUF_OK, "split OK");
    CHECK_INT(b->n_lines, 3, "added a newline");
    CHECK_LINE(b, 0, "hello");
    CHECK_LINE(b, 1, " world");
    CHECK_LINE(b, 2, "second");
    
    // join the lines back: hello\n world -> hello world
    CHECK_INT(buffer_join_lines(b, 1), BUF_OK, "join OK");
    CHECK_INT(b->n_lines, 2, "removed a line");
    CHECK_LINE(b, 0, "hello world");

    // CHECK_INT(buffer_split_line(b, 2, 0), BUF_OK, "split on last line");
    // CHECK_INT(b->n_lines, 3, "added a newline (last line) n_lines");

    // duplicate a row
    CHECK_INT(buffer_duplicate_line(b, 0), BUF_OK, "dupe OK");
    CHECK_INT(b->n_lines, 3, "duplicated line added");
    CHECK_LINE(b, 0, "hello world");
    CHECK_LINE(b, 1, "hello world");

    CHECK(validate_buffer(b), "buffer valid after full sequence");
    free_buff(b);
}

static void test_error_returns(void)
{
    // test error codes returned from bad ops

    // n_lines = 2, each len = 3
    const char *src[] = { "abc", "def" };
    Buffer *b = NULL;
    b = mem_buffer(src, 2);
    if (!b) {
        perror("FAILED TO MAKE MEMORY ONLY BUFFER\n");
        return;
    }

    // OOB row insert
    CHECK_INT(buffer_insert_char(b, -1, 0, 'x'), BUF_OOB, "insert row < 0");
    CHECK_INT(buffer_insert_char(b, 2, 0, 'x'), BUF_OOB, "insert row == n_lines");

    // OOB column insert (insert col=len OK; col > len NOK)
    CHECK_INT(buffer_insert_char(b, 0, 4, 'x'), BUF_OOB, "insert col > len");
    // CHECK_INT(buffer_insert_char(b, 0, 3, 'x'), BUF_OK, "insert col == len");
    // buffer_delete_char(b, 0, 3); // remove it

    // delete rejects col == len (nothing to delete)
    CHECK_INT(buffer_delete_char(b, 0, 3), BUF_OOB, "delete col == len");

    // span functions: reject n <= 0 & over-runs
    CHECK_INT(buffer_insert_n(b, 0, 0, 'x', 0), BUF_OOB, "insert_n n=0");
    CHECK_INT(buffer_insert_n(b, 0, 0, 'x', -2), BUF_OOB, "insert_n n < 0");
    CHECK_INT(buffer_clear_n(b, 0, 2, 5), BUF_OOB, "clear_n col + n > len");

    // join the first line (has no predecessor)
    CHECK_INT(buffer_join_lines(b, 0), BUF_OOB, "join row == 0");

    // split bad position (past line end)
    CHECK_INT(buffer_split_line(b, 0, 99), BUF_OOB, "split col > len");

    // all the above should not modify the buffer
    CHECK_INT(b->n_lines, 2, "n_lines untouched after bad ops");
    CHECK_INT(b->dirty, 1, "should be something to save (split line)");
    CHECK_LINE(b, 0, "abc");
    CHECK_LINE(b, 1, "def");

    CHECK(validate_buffer(b), "buffer OK after error flood");
    free_buff(b);
}

static void test_null_insert(void)
{
    // test the behavior around a NULL inserted as a char
    // currently untreated in the buff.c, so this is a BUG

    const char *src[] = { "abc" };
    Buffer *b = NULL;
    b = mem_buffer(src, 1);
    if (!b) { perror("buffer failed to load"); return; }

    CHECK_INT(buffer_insert_char(b, 0, 1, '\0'), BUF_NULLC, "insert NULL returned BUF_NULLC");
    // but now there's an error, and it should be flagged:
    CHECK(validate_buffer(b), "NULL inserted keeps len == strlen (SHOULD FAIL)");
    free_buff(b);
}

static void test_roundtrip_writeout(void)
{
    char path[] = "/tmp/buff_woXXXXXX";
    int fd = mkstemp(path);
    if (!fd) { perror("couldn't open tmp file"); return; }
    FILE *f = fdopen(fd, "w");
    if (!f) { perror("couldn't open file"); return; }
    fputs("a\nb", f);
    fclose(f);

    FILE *orig = fopen(path, "rb");
    if (!orig) { perror("couldn't open file"); return; }
    Buffer *b = NULL;
    b = buffer_load(path);
    if (!b) { perror("buffer failed to load"); return; }
    CHECK_INT(buffer_writeout(b, path, orig), BUF_OK, "writeout OK");
    fclose(orig);

    f = fopen(path, "rb");
    if (!f) { perror("failed to open file"); return; }
    char out[16];
    size_t n = fread(out, 1, sizeof out - 1, f);
    out[n] = '\0';
    fclose(f);

    // on writeout, every line *should* get newline: a\nb -> a\nb\n
    CHECK(strcmp(out, "a\nb\n") == 0, "writeout appends trailing newline");
    unlink(path);
    free_buff(b);
}

static void test_stale_writeout(void)
{
    char path_o[] = "/tmp/buff_wo_oriXXXXXX";
    int fd = mkstemp(path_o);
    if (!fd) { perror("couldn't open tmp file"); return; }
    FILE *f = fdopen(fd, "w");
    if (!f) { perror("failed to open file"); return; }
    fputs("a\nb", f);
    fclose(f);

    FILE *orig = fopen(path_o, "rb");
    if (!orig) { perror("failed to open file"); return; }
    Buffer *b = NULL;
    b = buffer_load(path_o);
    if (!b) { perror("buffer failed to load"); return; }
    fclose(orig);

    char path_m[] = "/tmp/buf_wo_modXXXXXX";
    fd = mkstemp(path_m);
    if (!fd) { perror("couldn't open tmp file"); return; }
    FILE *m = fdopen(fd, "w");
    fputs("a\nb\nedit", m);
    fflush(m);

    CHECK_INT(buffer_writeout(b, path_o, m), BUF_STALE, "writeout refused STALE buffer");
    fclose(m);

    f = fopen(path_o, "rb");
    if (!f) { perror("failed to open file"); return; }
    char out[16];
    size_t n = fread(out, 1, sizeof out - 1, f);
    out[n] = '\0';
    fclose(f);

    // on writeout, every line *should* get newline: a\nb -> a\nb\n
    CHECK(strcmp(out, "a\nb") == 0, "STALE WRITEOUT did not alter original");
    unlink(path_o);
    unlink(path_m);
    free_buff(b);
}

void stress_test_alloc_failure(void)
{
    
    const char *src[] = { "hello world", "second" };
    
    // find out how many allocations occur in 'happy' path
    Buffer *b = NULL;
    b = mem_buffer(src, 2);
    if (!b) { perror("buffer failed to load"); return; }
    g_alloc_n = 0;
    g_fail_at = -1;
    buffer_split_line(b, 0, 5);
    int K = g_alloc_n;
    free_buff(b);

    // fail each one in turn
    for (int k = 0; k < K; k++) {
        b = mem_buffer(src, 2);
        if (!b) { perror("buffer failed to load"); return; }
        int n_before = b->n_lines;
        int len0_before = b->lines[0].len;
        int dirty_before = b->dirty;

        // set the break-points
        g_alloc_n = 0;
        g_fail_at = k;
        int rc = buffer_split_line(b, 0, 5);
        g_fail_at = -1;

        CHECK_INT(rc, BUF_NOMEM, "split returns NOMEM on alloc failure");
        CHECK(validate_buffer(b), "buffer still valid after alloc failure");
        CHECK_INT(b->n_lines, n_before, "n_lines unchanged after alloc failure");
        CHECK_INT(b->lines[0].len, len0_before, "b->lines[0].len unchanged after alloc failure");
        CHECK_INT(b->dirty, dirty_before, "b->dirty unchanged after alloc failure");
        free_buff(b);
    }
}

int main(void)
{
    char path[1024];
    init_logging(path);

    test_load();
    test_line_lim();
    test_editor_x();
    test_error_returns();
    test_null_insert();
    test_roundtrip_writeout();
    test_stale_writeout();

    stress_test_alloc_failure();

    if (g_fails == 0) { fprintf(stdout, "All tests passed\n"); return 0; }
    fprintf(stdout, "tests complete: passed %d/%d\n", g_tests - g_fails, g_tests);
    return 0;
}
