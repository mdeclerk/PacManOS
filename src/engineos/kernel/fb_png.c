#include "engineos/kernel/fb.h"
#include "engineos/kernel/heap.h"
#include "stdlib/string.h"

/* ========== Helper macros ========== */

#define RD32(p) (((uint32_t)(p)[0]<<24)|((uint32_t)(p)[1]<<16)|((uint32_t)(p)[2]<<8)|(p)[3])
#define RD16(p) (((uint16_t)(p)[0]<<8)|(p)[1])
#define CHUNK(a,b,c,d) ((uint32_t)(a)<<24|(uint32_t)(b)<<16|(uint32_t)(c)<<8|(d))

/* ========== Bitstream reader for inflate ========== */

typedef struct {
    const uint8_t *data;
    size_t size;
    size_t pos;
    uint32_t bits;
    int nbits;
} bitstream_t;

static void bs_init(bitstream_t *bs, const uint8_t *data, size_t size)
{
    bs->data = data;
    bs->size = size;
    bs->pos = 0;
    bs->bits = 0;
    bs->nbits = 0;
}

static uint32_t bs_read(bitstream_t *bs, int n)
{
    uint32_t val;
    while (bs->nbits < n) {
        if (bs->pos >= bs->size) return 0;
        bs->bits |= (uint32_t)bs->data[bs->pos++] << bs->nbits;
        bs->nbits += 8;
    }
    val = bs->bits & ((1u << n) - 1);
    bs->bits >>= n;
    bs->nbits -= n;
    return val;
}

/* ========== Huffman tables ========== */

#define MAXBITS 15
#define MAXSYM  320

typedef struct {
    uint16_t counts[MAXBITS + 1];
    uint16_t symbols[MAXSYM];
} huffman_t;

static int huff_build(huffman_t *h, const uint8_t *lens, int n)
{
    int i;
    uint16_t offs[MAXBITS + 1];
    memset(h->counts, 0, sizeof(h->counts));
    for (i = 0; i < n; i++) {
        if (lens[i]) h->counts[lens[i]]++;
    }
    offs[0] = 0;
    for (i = 1; i <= MAXBITS; i++) {
        offs[i] = offs[i - 1] + h->counts[i - 1];
    }
    for (i = 0; i < n; i++) {
        if (lens[i]) h->symbols[offs[lens[i]]++] = (uint16_t)i;
    }
    return 0;
}

static int huff_decode(huffman_t *h, bitstream_t *bs)
{
    int code = 0, first = 0, idx = 0, len;
    for (len = 1; len <= MAXBITS; len++) {
        code = (code << 1) | bs_read(bs, 1);
        int count = h->counts[len];
        if (code < first + count) {
            return h->symbols[idx + code - first];
        }
        idx += count;
        first = (first + count) << 1;
    }
    return -1;
}

/* ========== Fixed Huffman tables ========== */

static huffman_t fixed_lit, fixed_dist;
static int fixed_init = 0;

static void init_fixed_tables(void)
{
    uint8_t lens[320];
    int i;
    if (fixed_init) return;
    for (i = 0; i <= 143; i++) lens[i] = 8;
    for (i = 144; i <= 255; i++) lens[i] = 9;
    for (i = 256; i <= 279; i++) lens[i] = 7;
    for (i = 280; i <= 287; i++) lens[i] = 8;
    huff_build(&fixed_lit, lens, 288);
    for (i = 0; i < 32; i++) lens[i] = 5;
    huff_build(&fixed_dist, lens, 32);
    fixed_init = 1;
}

/* ========== Inflate ========== */

static const uint16_t len_base[29] = {
    3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258
};
static const uint8_t len_extra[29] = {
    0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0
};
static const uint16_t dist_base[30] = {
    1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,
    1025,1537,2049,3073,4097,6145,8193,12289,16385,24577
};
static const uint8_t dist_extra[30] = {
    0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13
};
static const uint8_t codelen_order[19] = {
    16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15
};

static int inflate(uint8_t *out, size_t out_size, const uint8_t *in, size_t in_size)
{
    bitstream_t bs;
    size_t out_pos = 0;
    int bfinal;

    bs_init(&bs, in, in_size);

    do {
        int btype;
        bfinal = bs_read(&bs, 1);
        btype = bs_read(&bs, 2);

        if (btype == 0) {
            /* Uncompressed block */
            uint16_t len, nlen;
            bs.bits = 0;
            bs.nbits = 0;
            if (bs.pos + 4 > bs.size) return -1;
            len = bs.data[bs.pos] | (bs.data[bs.pos + 1] << 8);
            nlen = bs.data[bs.pos + 2] | (bs.data[bs.pos + 3] << 8);
            bs.pos += 4;
            if ((len ^ nlen) != 0xFFFF) return -1;
            if (out_pos + len > out_size) return -1;
            if (bs.pos + len > bs.size) return -1;
            memcpy(out + out_pos, bs.data + bs.pos, len);
            bs.pos += len;
            out_pos += len;
        } else if (btype == 1 || btype == 2) {
            huffman_t *hlit, *hdist;
            huffman_t dyn_lit, dyn_dist;

            if (btype == 1) {
                init_fixed_tables();
                hlit = &fixed_lit;
                hdist = &fixed_dist;
            } else {
                /* Dynamic Huffman */
                int hlit_n = bs_read(&bs, 5) + 257;
                int hdist_n = bs_read(&bs, 5) + 1;
                int hclen = bs_read(&bs, 4) + 4;
                uint8_t codelen_lens[19];
                uint8_t lens[320];
                huffman_t hcl;
                int i, n, sym;

                memset(codelen_lens, 0, sizeof(codelen_lens));
                for (i = 0; i < hclen; i++) {
                    codelen_lens[codelen_order[i]] = bs_read(&bs, 3);
                }
                huff_build(&hcl, codelen_lens, 19);

                n = 0;
                while (n < hlit_n + hdist_n) {
                    sym = huff_decode(&hcl, &bs);
                    if (sym < 0) return -1;
                    if (sym < 16) {
                        lens[n++] = sym;
                    } else if (sym == 16) {
                        int rep = bs_read(&bs, 2) + 3;
                        uint8_t prev = n > 0 ? lens[n - 1] : 0;
                        while (rep-- && n < hlit_n + hdist_n) lens[n++] = prev;
                    } else if (sym == 17) {
                        int rep = bs_read(&bs, 3) + 3;
                        while (rep-- && n < hlit_n + hdist_n) lens[n++] = 0;
                    } else if (sym == 18) {
                        int rep = bs_read(&bs, 7) + 11;
                        while (rep-- && n < hlit_n + hdist_n) lens[n++] = 0;
                    }
                }
                huff_build(&dyn_lit, lens, hlit_n);
                huff_build(&dyn_dist, lens + hlit_n, hdist_n);
                hlit = &dyn_lit;
                hdist = &dyn_dist;
            }

            /* Decode compressed data */
            for (;;) {
                int sym = huff_decode(hlit, &bs);
                if (sym < 0) return -1;
                if (sym < 256) {
                    if (out_pos >= out_size) return -1;
                    out[out_pos++] = (uint8_t)sym;
                } else if (sym == 256) {
                    break;
                } else {
                    int len_idx = sym - 257;
                    int len, dist_sym, dist;
                    size_t i;
                    if (len_idx >= 29) return -1;
                    len = len_base[len_idx] + bs_read(&bs, len_extra[len_idx]);
                    dist_sym = huff_decode(hdist, &bs);
                    if (dist_sym < 0 || dist_sym >= 30) return -1;
                    dist = dist_base[dist_sym] + bs_read(&bs, dist_extra[dist_sym]);
                    if (out_pos + len > out_size) return -1;
                    if ((size_t)dist > out_pos) return -1;
                    for (i = 0; i < (size_t)len; i++) {
                        out[out_pos] = out[out_pos - dist];
                        out_pos++;
                    }
                }
            }
        } else {
            return -1;
        }
    } while (!bfinal);

    return (int)out_pos;
}

/* ========== PNG filter decode ========== */

static uint8_t paeth(uint8_t a, uint8_t b, uint8_t c)
{
    int p = (int)a + (int)b - (int)c;
    int pa = p > a ? p - a : a - p;
    int pb = p > b ? p - b : b - p;
    int pc = p > c ? p - c : c - p;
    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

static int png_unfilter_inplace(uint8_t *buf, uint32_t w, uint32_t h, int bpp)
{
    uint32_t stride = w * bpp;

    for (uint32_t y = 0; y < h; y++) {
        const uint8_t *src = buf + y * (stride + 1) + 1;
        uint8_t *dst = buf + y * stride;
        uint8_t filter = src[-1];
        const uint8_t *prev = (y == 0) ? NULL : (dst - stride);

        for (uint32_t x = 0; x < stride; x++) {
            uint8_t raw_byte = src[x];
            uint8_t a = (x >= (uint32_t)bpp) ? dst[x - bpp] : 0;
            uint8_t b = prev ? prev[x] : 0;
            uint8_t c = (prev && x >= (uint32_t)bpp) ? prev[x - bpp] : 0;
            uint8_t val;

            switch (filter) {
            case 0: val = raw_byte; break;
            case 1: val = raw_byte + a; break;
            case 2: val = raw_byte + b; break;
            case 3: val = raw_byte + ((a + b) >> 1); break;
            case 4: val = raw_byte + paeth(a, b, c); break;
            default: return -1;
            }
            dst[x] = val;
        }
    }
    return 0;
}

/* ========== Main PNG decoder ========== */

static const uint8_t png_sig[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

struct image fb_decode_png(const void *data, size_t size)
{
    struct image result = { 0 };
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t width = 0, height = 0;
    uint8_t bit_depth = 0, color_type = 0;
    uint8_t *idat_buf = NULL;
    size_t idat_size = 0, idat_cap = 0;
    uint8_t *raw = NULL;

    if (!bytes || size < 8)
        return result;
    const uint8_t *p = bytes;
    const uint8_t *end = bytes + size;

    /* Check signature */
    if (memcmp(p, png_sig, 8) != 0) return result;
    p += 8;

    /* Parse chunks */
    while (p + 12 <= end) {
        uint32_t len = RD32(p);
        uint32_t type = RD32(p + 4);
        const uint8_t *chunk_data = p + 8;

        if (p + 12 + len > end) break;

        if (type == CHUNK('I','H','D','R')) {
            if (len < 13) goto fail;
            width = RD32(chunk_data);
            height = RD32(chunk_data + 4);
            bit_depth = chunk_data[8];
            color_type = chunk_data[9];
            /* Only support 8-bit RGB (2) or RGBA (6), non-interlaced */
            if (bit_depth != 8) goto fail;
            if (color_type != 2 && color_type != 6) goto fail;
            if (chunk_data[12] != 0) goto fail; /* interlace */
        } else if (type == CHUNK('I','D','A','T')) {
            if (idat_size + len > idat_cap) {
                size_t new_cap = idat_cap ? idat_cap * 2 : 65536;
                uint8_t *new_buf;
                while (new_cap < idat_size + len) new_cap *= 2;
                new_buf = (uint8_t *)heap_alloc(new_cap);
                if (!new_buf) goto fail;
                if (idat_buf) {
                    memcpy(new_buf, idat_buf, idat_size);
                    heap_free(idat_buf);
                }
                idat_buf = new_buf;
                idat_cap = new_cap;
            }
            memcpy(idat_buf + idat_size, chunk_data, len);
            idat_size += len;
        } else if (type == CHUNK('I','E','N','D')) {
            break;
        }
        p += 12 + len;
    }

    if (!width || !height || !idat_buf) goto fail;

    int bpp = (color_type == 6) ? 4 : 3;
    size_t raw_size = (size_t)height * ((size_t)width * bpp + 1);
    raw = (uint8_t *)heap_alloc(raw_size);
    if (!raw) goto fail;

    /* Skip zlib header (2 bytes) and decompress */
    if (idat_size < 2) goto fail;
    if (inflate(raw, raw_size, idat_buf + 2, idat_size - 2) < 0) goto fail;

    /* Unfilter in-place: compacts raw from [filter|row]... to [row]... */
    if (png_unfilter_inplace(raw, width, height, bpp) < 0) goto fail;

    /* Convert to ARGB */
    result.width = (int)width;
    result.height = (int)height;
    result.pitch = (int)width;
    result.pixels = (color_t *)heap_alloc((size_t)width * height * sizeof(color_t));
    if (!result.pixels) {
        result.width = 0;
        result.height = 0;
        result.pitch = 0;
        goto fail;
    }

    if (color_type == 6) {
        /* RGBA -> ARGB */
        for (uint32_t i = 0; i < width * height; i++) {
            uint8_t r = raw[i * 4 + 0];
            uint8_t g = raw[i * 4 + 1];
            uint8_t b = raw[i * 4 + 2];
            uint8_t a = raw[i * 4 + 3];
            result.pixels[i] = ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
        }
    } else {
        /* RGB -> ARGB (alpha = 0xFF) */
        for (uint32_t i = 0; i < width * height; i++) {
            uint8_t r = raw[i * 3 + 0];
            uint8_t g = raw[i * 3 + 1];
            uint8_t b = raw[i * 3 + 2];
            result.pixels[i] = 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
        }
    }

fail:
    if (idat_buf) heap_free(idat_buf);
    if (raw) heap_free(raw);
    return result;
}
