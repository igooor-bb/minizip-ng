/* A one-shot output failure must reach the caller of entry_write_close. */
#include <stdio.h>
#include <string.h>
#include "mz.h"
#include "mz_strm.h"
#include "mz_zip.h"

typedef struct {
    mz_stream stream;
    int64_t position;
    int fail_next;
    int failures;
} sink;

static int32_t is_open(void *s) { (void)s; return MZ_OK; }
static int32_t write_bytes(void *s, const void *data, int32_t size) {
    sink *out = s;
    (void)data;
    if (out->fail_next) {
        out->fail_next = 0;
        out->failures++;
        return MZ_WRITE_ERROR;
    }
    out->position += size;
    return size;
}
static int64_t tell(void *s) { return ((sink *)s)->position; }
static int32_t seek(void *s, int64_t offset, int32_t origin) {
    sink *out = s;
    out->position = origin == MZ_SEEK_SET ? offset : out->position + offset;
    return MZ_OK;
}
static mz_stream_vtbl vtable = {
    .is_open = is_open, .write = write_bytes, .tell = tell, .seek = seek
};

int main(int argc, char **argv) {
    sink out = {{&vtable, NULL}, 0, 0, 0};
    void *zip = mz_zip_create();
    mz_zip_file entry = {0};
    entry.filename = "entry";
    entry.compression_method = MZ_COMPRESS_METHOD_DEFLATE;
    if (argc > 2)
        return 2;
    if (argc == 2) {
        if (strcmp(argv[1], "bzip2") == 0) entry.compression_method = MZ_COMPRESS_METHOD_BZIP2;
        else if (strcmp(argv[1], "lzma") == 0) entry.compression_method = MZ_COMPRESS_METHOD_LZMA;
        else if (strcmp(argv[1], "xz") == 0) entry.compression_method = MZ_COMPRESS_METHOD_XZ;
        else if (strcmp(argv[1], "zstd") == 0) entry.compression_method = MZ_COMPRESS_METHOD_ZSTD;
        else if (strcmp(argv[1], "ppmd") == 0) entry.compression_method = MZ_COMPRESS_METHOD_PPMD;
        else if (strcmp(argv[1], "deflate") != 0) return 2;
    }
    if (mz_zip_open(zip, &out, MZ_OPEN_MODE_WRITE) != MZ_OK ||
        mz_zip_entry_write_open(zip, &entry, 6, 0, NULL) != MZ_OK ||
        mz_zip_entry_write(zip, "payload", 7) != 7)
        return 2;

    /* Fail the first write during finalization. Later writes succeed. */
    out.fail_next = 1;
    int32_t result = mz_zip_entry_write_close(zip, 0, -1, -1);
    printf("injected failures=%d; entry_close=%d; expected=%d\n",
           out.failures, result, MZ_WRITE_ERROR);
    mz_zip_close(zip);
    mz_zip_delete(&zip);
    if (out.failures != 1 || out.fail_next)
        return 2;
    return result == MZ_WRITE_ERROR ? 0 : 1;
}
