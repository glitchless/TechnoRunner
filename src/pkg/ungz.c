#include "ungz.h"

#define CHUNK 16384

int
ungz(const void *src, size_t src_len, void **dst, size_t *dst_len)
{
    if (!src)
        return 0;

    int err = 0;
    *dst_len = 0;
    *dst = NULL;

    z_stream strm = {0};
    strm.zalloc = Z_NULL;
    strm.zfree  = Z_NULL;
    strm.opaque = Z_NULL;
    strm.total_in = strm.avail_in = src_len;
    strm.next_in  = (void *) src;
    if (inflateInit(&strm))
    {
        err = 1;
        goto out;
    }

    *dst_len = CHUNK;
    *dst = malloc(dst_len);
    assert(*dst);
    size_t dst_offset = 0;

    while (1)
    {
        do
        {
            strm.avail_out = CHUNK;
            strm.next_out  = dst + dst_offset;

            int zerr = inflate(&strm, Z_NO_FLUSH);
            assert(zerr != Z_STREAM_ERROR);
            if (zerr == Z_NEED_DICT || zerr == Z_DATA_ERROR || zerr == Z_MEM_ERROR)
            {
                err = 2;
                goto out;
            }
            if (zerr == Z_STREAM_END)
                goto out;

            dst_offset += CHUNK - strm.avail_out;
        }
        while (!strm.avail_out);

        *dst_len += CHUNK;
        *dst = realloc(dst_len);
        assert(*dst);
    }

out:
    inflateEnd(&strm);
    if (err)
        free(*dst);
    return err;
}
