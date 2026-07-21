/**
 * @file    utl_crypto.c
 * @brief   CRC32 / MD5 / SHA256 / HMAC-SHA256 implementation
 * @author  RenJiaqi
 * @version 1.0.0
 * @date    2026-07-07
 *
 * @copyright Copyright (c) 2025-2026 VaultCode. All rights reserved.
 */

#include <string.h>
#include <stdio.h>
#include "utl_crypto.h"

/* ======================== CRC32 ======================== */

static const uint32_t kCrc32Table[256] = {
    0x00000000,0x77073096,0xEE0E612C,0x990951BA,0x076DC419,0x706AF48F,0xE963A535,0x9E6495A3,
    0x0EDB8832,0x79DCB8A4,0xE0D5E91E,0x97D2D988,0x09B64C2B,0x7EB17CBD,0xE7B82D07,0x90BF1D91,
    0x1DB71064,0x6AB020F2,0xF3B97148,0x84BE41DE,0x1ADAD47D,0x6DDDE4EB,0xF4D4B551,0x83D385C7,
    0x136C9856,0x646BA8C0,0xFD62F97A,0x8A65C9EC,0x14015C4F,0x63066CD9,0xFA0F3D63,0x8D080DF5,
    0x3B6E20C8,0x4C69105E,0xD56041E4,0xA2677172,0x3C03E4D1,0x4B04D447,0xD20D85FD,0xA50AB56B,
    0x35B5A8FA,0x42B2986C,0xDBBBC9D6,0xACBCF940,0x32D86CE3,0x45DF5C75,0xDCD60DCF,0xABD13D59,
    0x26D930AC,0x51DE003A,0xC8D75180,0xBFD06116,0x21B4F4B5,0x56B3C423,0xCFBA9599,0xB8BDA50F,
    0x2802B89E,0x5F058808,0xC60CD9B2,0xB10BE924,0x2F6F7C87,0x58684C11,0xC1611DAB,0xB6662D3D,
    0x76DC4190,0x01DB7106,0x98D220BC,0xEFD5102A,0x71B18589,0x06B6B51F,0x9FBFE4A5,0xE8B8D433,
    0x7807C9A2,0x0F00F934,0x9609A88E,0xE10E9818,0x7F6A0DBB,0x086D3D2D,0x91646C97,0xE6635C01,
    0x6B6B51F4,0x1C6C6162,0x856530D8,0xF262004E,0x6C0695ED,0x1B01A57B,0x8208F4C1,0xF50FC457,
    0x65B0D9C6,0x12B7E950,0x8BBEB8EA,0xFCB9887C,0x62DD1DDF,0x15DA2D49,0x8CD37CF3,0xFBD44C65,
    0x4DB26158,0x3AB551CE,0xA3BC0074,0xD4BB30E2,0x4ADFA541,0x3DD895D7,0xA4D1C46D,0xD3D6F4FB,
    0x4369E96A,0x346ED9FC,0xAD678846,0xDA60B8D0,0x44042D73,0x33031DE5,0xAA0A4C5F,0xDD0D7CC9,
    0x5005713C,0x270241AA,0xBE0B1010,0xC90C2086,0x5768B525,0x206F85B3,0xB966D409,0xCE61E49F,
    0x5EDEF90E,0x29D9C998,0xB0D09822,0xC7D7A8B4,0x59B33D17,0x2EB40D81,0xB7BD5C3B,0xC0BA6CAD,
    0xEDB88320,0x9ABFB3B6,0x03B6E20C,0x74B1D29A,0xEAD54739,0x9DD277AF,0x04DB2615,0x73DC1683,
    0xE3630B12,0x94643B84,0x0D6D6A3E,0x7A6A5AA8,0xE40ECF0B,0x9309FF9D,0x0A00AE27,0x7D079EB1,
    0xF00F9344,0x8708A3D2,0x1E01F268,0x6906C2FE,0xF762575D,0x806567CB,0x196C3671,0x6E6B06E7,
    0xFED41B76,0x89D32BE0,0x10DA7A5A,0x67DD4ACC,0xF9B9DF6F,0x8EBEEFF9,0x17B7BE43,0x60B08ED5,
    0xD6D6A3E8,0xA1D1937E,0x38D8C2C4,0x4FDFF252,0xD1BB67F1,0xA6BC5767,0x3FB506DD,0x48B2364B,
    0xD80D2BDA,0xAF0A1B4C,0x36034AF6,0x41047A60,0xDF60EFC3,0xA867DF55,0x316E8EEF,0x4669BE79,
    0xCB61B38C,0xBC66831A,0x256FD2A0,0x5268E236,0xCC0C7795,0xBB0B4703,0x220216B9,0x5505262F,
    0xC5BA3BBE,0xB2BD0B28,0x2BB45A92,0x5CB36A04,0xC2D7FFA7,0xB5D0CF31,0x2CD99E8B,0x5BDEAE1D,
    0x9B64C2B0,0xEC63F226,0x756AA39C,0x026D930A,0x9C0906A9,0xEB0E363F,0x72076785,0x05005713,
    0x95BF4A82,0xE2B87A14,0x7BB12BAE,0x0CB61B38,0x92D28E9B,0xE5D5BE0D,0x7CDCEFB7,0x0BDBDF21,
    0x86D3D2D4,0xF1D4E242,0x68DDB3F8,0x1FDA836E,0x81BE16CD,0xF6B9265B,0x6FB077E1,0x18B74777,
    0x88085AE6,0xFF0F6A70,0x66063BCA,0x11010B5C,0x8F659EFF,0xF862AE69,0x616BFFD3,0x166CCF45,
    0xA00AE278,0xD70DD2EE,0x4E048354,0x3903B3C2,0xA7672661,0xD06016F7,0x4969474D,0x3E6E77DB,
    0xAED16A4A,0xD9D65ADC,0x40DF0B66,0x37D83BF0,0xA9BCAE53,0xDEBB9EC5,0x47B2CF7F,0x30B5FFE9,
    0xBDBDF21C,0xCABAC28A,0x53B39330,0x24B4A3A6,0xBAD03605,0xCDD70693,0x54DE5729,0x23D967BF,
    0xB3667A2E,0xC4614AB8,0x5D681B02,0x2A6F2B94,0xB40BBE37,0xC30C8EA1,0x5A05DF1B,0x2D02EF8D
};

uint32_t utl_crc32_update(uint32_t crc, const void *data, size_t len)
{
    const utl_byte_t *p = (const utl_byte_t *)data;
    crc ^= 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++)
        crc = kCrc32Table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFF;
}

/* ======================== MD5 ======================== */

#define MD5_F(x,y,z) (((x)&(y)) | ((~(x))&(z)))
#define MD5_G(x,y,z) (((x)&(z)) | ((y)&(~(z))))
#define MD5_H(x,y,z) ((x)^(y)^(z))
#define MD5_I(x,y,z) ((y)^((x)|(~(z))))
#define MD5_ROTL(x,n) (((x)<<(n)) | ((x)>>(32-(n))))
#define MD5_STEP(f,a,b,c,d,x,s,ac) do { (a)+=f(b,c,d)+(x)+(ac); (a)=MD5_ROTL(a,s); (a)+=(b); } while(0)

static const utl_byte_t kMd5Padding[64] = {0x80};

static void md5_encode(uint32_t *out, const utl_byte_t *in, size_t len)
{
    for (size_t i = 0, j = 0; j < len; i++, j += 4)
        out[i] = ((uint32_t)in[j]) | ((uint32_t)in[j+1]<<8) |
                 ((uint32_t)in[j+2]<<16) | ((uint32_t)in[j+3]<<24);
}

static void md5_decode(utl_byte_t *out, const uint32_t *in, size_t len)
{
    for (size_t i = 0, j = 0; j < len; i++, j += 4) {
        out[j]   = (utl_byte_t)(in[i] & 0xFF);
        out[j+1] = (utl_byte_t)((in[i]>>8) & 0xFF);
        out[j+2] = (utl_byte_t)((in[i]>>16) & 0xFF);
        out[j+3] = (utl_byte_t)((in[i]>>24) & 0xFF);
    }
}

static void md5_transform(uint32_t state[4], const utl_byte_t block[64])
{
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3], x[16];
    md5_encode(x, block, 64);
    MD5_STEP(MD5_F,a,b,c,d,x[0],7,0xD76AA478); MD5_STEP(MD5_F,d,a,b,c,x[1],12,0xE8C7B756);
    MD5_STEP(MD5_F,c,d,a,b,x[2],17,0x242070DB); MD5_STEP(MD5_F,b,c,d,a,x[3],22,0xC1BDCEEE);
    MD5_STEP(MD5_F,a,b,c,d,x[4],7,0xF57C0FAF); MD5_STEP(MD5_F,d,a,b,c,x[5],12,0x4787C62A);
    MD5_STEP(MD5_F,c,d,a,b,x[6],17,0xA8304613); MD5_STEP(MD5_F,b,c,d,a,x[7],22,0xFD469501);
    MD5_STEP(MD5_F,a,b,c,d,x[8],7,0x698098D8); MD5_STEP(MD5_F,d,a,b,c,x[9],12,0x8B44F7AF);
    MD5_STEP(MD5_F,c,d,a,b,x[10],17,0xFFFF5BB1); MD5_STEP(MD5_F,b,c,d,a,x[11],22,0x895CD7BE);
    MD5_STEP(MD5_F,a,b,c,d,x[12],7,0x6B901122); MD5_STEP(MD5_F,d,a,b,c,x[13],12,0xFD987193);
    MD5_STEP(MD5_F,c,d,a,b,x[14],17,0xA679438E); MD5_STEP(MD5_F,b,c,d,a,x[15],22,0x49B40821);
    MD5_STEP(MD5_G,a,b,c,d,x[1],5,0xF61E2562); MD5_STEP(MD5_G,d,a,b,c,x[6],9,0xC040B340);
    MD5_STEP(MD5_G,c,d,a,b,x[11],14,0x265E5A51); MD5_STEP(MD5_G,b,c,d,a,x[0],20,0xE9B6C7AA);
    MD5_STEP(MD5_G,a,b,c,d,x[5],5,0xD62F105D); MD5_STEP(MD5_G,d,a,b,c,x[10],9,0x02441453);
    MD5_STEP(MD5_G,c,d,a,b,x[15],14,0xD8A1E681); MD5_STEP(MD5_G,b,c,d,a,x[4],20,0xE7D3FBC8);
    MD5_STEP(MD5_G,a,b,c,d,x[9],5,0x21E1CDE6); MD5_STEP(MD5_G,d,a,b,c,x[14],9,0xC33707D6);
    MD5_STEP(MD5_G,c,d,a,b,x[3],14,0xF4D50D87); MD5_STEP(MD5_G,b,c,d,a,x[8],20,0x455A14ED);
    MD5_STEP(MD5_G,a,b,c,d,x[13],5,0xA9E3E905); MD5_STEP(MD5_G,d,a,b,c,x[2],9,0xFCEFA3F8);
    MD5_STEP(MD5_G,c,d,a,b,x[7],14,0x676F02D9); MD5_STEP(MD5_G,b,c,d,a,x[12],20,0x8D2A4C8A);
    MD5_STEP(MD5_H,a,b,c,d,x[5],4,0xFFFA3942); MD5_STEP(MD5_H,d,a,b,c,x[8],11,0x8771F681);
    MD5_STEP(MD5_H,c,d,a,b,x[11],16,0x6D9D6122); MD5_STEP(MD5_H,b,c,d,a,x[14],23,0xFDE5380C);
    MD5_STEP(MD5_H,a,b,c,d,x[1],4,0xA4BEEA44); MD5_STEP(MD5_H,d,a,b,c,x[4],11,0x4BDECFA9);
    MD5_STEP(MD5_H,c,d,a,b,x[7],16,0xF6BB4B60); MD5_STEP(MD5_H,b,c,d,a,x[10],23,0xBEBFBC70);
    MD5_STEP(MD5_H,a,b,c,d,x[13],4,0x289B7EC6); MD5_STEP(MD5_H,d,a,b,c,x[0],11,0xEAA127FA);
    MD5_STEP(MD5_H,c,d,a,b,x[3],16,0xD4EF3085); MD5_STEP(MD5_H,b,c,d,a,x[6],23,0x04881D05);
    MD5_STEP(MD5_H,a,b,c,d,x[9],4,0xD9D4D039); MD5_STEP(MD5_H,d,a,b,c,x[12],11,0xE6DB99E5);
    MD5_STEP(MD5_H,c,d,a,b,x[15],16,0x1FA27CF8); MD5_STEP(MD5_H,b,c,d,a,x[2],23,0xC4AC5665);
    MD5_STEP(MD5_I,a,b,c,d,x[0],6,0xF4292244); MD5_STEP(MD5_I,d,a,b,c,x[7],10,0x432AFF97);
    MD5_STEP(MD5_I,c,d,a,b,x[14],15,0xAB9423A7); MD5_STEP(MD5_I,b,c,d,a,x[5],21,0xFC93A039);
    MD5_STEP(MD5_I,a,b,c,d,x[12],6,0x655B59C3); MD5_STEP(MD5_I,d,a,b,c,x[3],10,0x8F0CCC92);
    MD5_STEP(MD5_I,c,d,a,b,x[10],15,0xFFEFF47D); MD5_STEP(MD5_I,b,c,d,a,x[1],21,0x85845DD1);
    MD5_STEP(MD5_I,a,b,c,d,x[8],6,0x6FA87E4F); MD5_STEP(MD5_I,d,a,b,c,x[15],10,0xFE2CE6E0);
    MD5_STEP(MD5_I,c,d,a,b,x[6],15,0xA3014314); MD5_STEP(MD5_I,b,c,d,a,x[13],21,0x4E0811A1);
    MD5_STEP(MD5_I,a,b,c,d,x[4],6,0xF7537E82); MD5_STEP(MD5_I,d,a,b,c,x[11],10,0xBD3AF235);
    MD5_STEP(MD5_I,c,d,a,b,x[2],15,0x2AD7D2BB); MD5_STEP(MD5_I,b,c,d,a,x[9],21,0xEB86D391);
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
}

void utl_md5_init(utl_md5_ctx_t *ctx)
{
    if (!ctx) return;
    ctx->state[0] = 0x67452301; ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE; ctx->state[3] = 0x10325476;
    ctx->count[0] = 0; ctx->count[1] = 0;
}

void utl_md5_update(utl_md5_ctx_t *ctx, const void *data, size_t len)
{
    if (!ctx || !data) return;
    const utl_byte_t *p = (const utl_byte_t *)data;
    size_t idx = (ctx->count[0] >> 3) & 0x3F;
    ctx->count[0] += (uint32_t)(len << 3);
    if (ctx->count[0] < (uint32_t)(len << 3)) ctx->count[1]++;
    ctx->count[1] += (uint32_t)(len >> 29);
    size_t part = 64 - idx;
    if (len >= part) {
        memcpy(ctx->buffer + idx, p, part);
        md5_transform(ctx->state, ctx->buffer);
        for (size_t i = part; i + 63 < len; i += 64)
            md5_transform(ctx->state, p + i);
        idx = 0;
    } else {
        part = 0;
    }
    memcpy(ctx->buffer + idx, p + part, len - part);
}

void utl_md5_final(utl_md5_ctx_t *ctx, utl_byte_t digest[UTL_MD5_DIGEST_SIZE])
{
    if (!ctx || !digest) return;
    utl_byte_t bits[8];
    md5_encode((uint32_t *)bits, (utl_byte_t *)ctx->count, 8);
    size_t idx = (ctx->count[0] >> 3) & 0x3F;
    size_t pad_len = (idx < 56) ? (56 - idx) : (120 - idx);
    utl_md5_update(ctx, kMd5Padding, pad_len);
    utl_md5_update(ctx, bits, 8);
    md5_decode(digest, ctx->state, UTL_MD5_DIGEST_SIZE);
}

void utl_md5(const void *data, size_t len, utl_byte_t digest[UTL_MD5_DIGEST_SIZE])
{
    utl_md5_ctx_t ctx;
    utl_md5_init(&ctx);
    utl_md5_update(&ctx, data, len);
    utl_md5_final(&ctx, digest);
}

void utl_md5_hex(const utl_byte_t digest[UTL_MD5_DIGEST_SIZE],
                 char hex[UTL_MD5_HEX_SIZE])
{
    for (int i = 0; i < UTL_MD5_DIGEST_SIZE; i++)
        snprintf(hex + i * 2, 3, "%02x", digest[i]);
    hex[UTL_MD5_HEX_SIZE - 1] = '\0';
}

/* ======================== SHA256 ======================== */

#define SHA256_ROTR(x,n) (((x)>>(n)) | ((x)<<(32-(n))))
#define SHA256_CH(x,y,z) (((x)&(y)) ^ ((~(x))&(z)))
#define SHA256_MAJ(x,y,z) (((x)&(y)) ^ ((x)&(z)) ^ ((y)&(z)))
#define SHA256_EP0(x) (SHA256_ROTR(x,2) ^ SHA256_ROTR(x,13) ^ SHA256_ROTR(x,22))
#define SHA256_EP1(x) (SHA256_ROTR(x,6) ^ SHA256_ROTR(x,11) ^ SHA256_ROTR(x,25))
#define SHA256_SIG0(x) (SHA256_ROTR(x,7) ^ SHA256_ROTR(x,18) ^ ((x)>>3))
#define SHA256_SIG1(x) (SHA256_ROTR(x,17) ^ SHA256_ROTR(x,19) ^ ((x)>>10))

static const uint32_t kSha256K[64] = {
    0x428A2F98,0x71374491,0xB5C0FBCF,0xE9B5DBA5,0x3956C25B,0x59F111F1,0x923F82A4,0xAB1C5ED5,
    0xD807AA98,0x12835B01,0x243185BE,0x550C7DC3,0x72BE5D74,0x80DEB1FE,0x9BDC06A7,0xC19BF174,
    0xE49B69C1,0xEFBE4786,0x0FC19DC6,0x240CA1CC,0x2DE92C6F,0x4A7484AA,0x5CB0A9DC,0x76F988DA,
    0x983E5152,0xA831C66D,0xB00327C8,0xBF597FC7,0xC6E00BF3,0xD5A79147,0x06CA6351,0x14292967,
    0x27B70A85,0x2E1B2138,0x4D2C6DFC,0x53380D13,0x650A7354,0x766A0ABB,0x81C2C92E,0x92722C85,
    0xA2BFE8A1,0xA81A664B,0xC24B8B70,0xC76C51A3,0xD192E819,0xD6990624,0xF40E3585,0x106AA070,
    0x19A4C116,0x1E376C08,0x2748774C,0x34B0BCB5,0x391C0CB3,0x4ED8AA4A,0x5B9CCA4F,0x682E6FF3,
    0x748F82EE,0x78A5636F,0x84C87814,0x8CC70208,0x90BEFFFA,0xA4506CEB,0xBEF9A3F7,0xC67178F2
};

static void sha256_transform(uint32_t state[8], const utl_byte_t block[64])
{
    uint32_t w[64];
    for (int i = 0; i < 16; i++)
        w[i] = ((uint32_t)block[i*4]<<24) | ((uint32_t)block[i*4+1]<<16) |
               ((uint32_t)block[i*4+2]<<8) | (uint32_t)block[i*4+3];
    for (int i = 16; i < 64; i++)
        w[i] = SHA256_SIG1(w[i-2]) + w[i-7] + SHA256_SIG0(w[i-15]) + w[i-16];

    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t e = state[4], f = state[5], g = state[6], h = state[7];

    for (int i = 0; i < 64; i++) {
        uint32_t t1 = h + SHA256_EP1(e) + SHA256_CH(e,f,g) + kSha256K[i] + w[i];
        uint32_t t2 = SHA256_EP0(a) + SHA256_MAJ(a,b,c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

void utl_sha256_init(utl_sha256_ctx_t *ctx)
{
    if (!ctx) return;
    ctx->state[0]=0x6A09E667; ctx->state[1]=0xBB67AE85;
    ctx->state[2]=0x3C6EF372; ctx->state[3]=0xA54FF53A;
    ctx->state[4]=0x510E527F; ctx->state[5]=0x9B05688C;
    ctx->state[6]=0x1F83D9AB; ctx->state[7]=0x5BE0CD19;
    ctx->count = 0;
}

void utl_sha256_update(utl_sha256_ctx_t *ctx, const void *data, size_t len)
{
    if (!ctx || !data) return;
    const utl_byte_t *p = (const utl_byte_t *)data;
    for (size_t i = 0; i < len; i++) {
        ctx->buffer[ctx->count % 64] = p[i];
        ctx->count++;
        if (ctx->count % 64 == 0) sha256_transform(ctx->state, ctx->buffer);
    }
}

void utl_sha256_final(utl_sha256_ctx_t *ctx,
                      utl_byte_t digest[UTL_SHA256_DIGEST_SIZE])
{
    if (!ctx || !digest) return;
    uint64_t bits = ctx->count * 8;
    size_t idx = (size_t)(ctx->count % 64);
    utl_byte_t pad[64];
    memset(pad, 0, 64);
    pad[0] = 0x80;
    size_t pad_len = (idx < 56) ? (56 - idx) : (120 - idx);
    utl_sha256_update(ctx, pad, pad_len);
    utl_byte_t bits_buf[8];
    for (int i = 0; i < 8; i++) bits_buf[i] = (utl_byte_t)(bits >> (56 - i*8));
    utl_sha256_update(ctx, bits_buf, 8);
    for (int i = 0; i < 8; i++) {
        digest[i*4]   = (utl_byte_t)(ctx->state[i] >> 24);
        digest[i*4+1] = (utl_byte_t)(ctx->state[i] >> 16);
        digest[i*4+2] = (utl_byte_t)(ctx->state[i] >> 8);
        digest[i*4+3] = (utl_byte_t)(ctx->state[i]);
    }
}

void utl_sha256(const void *data, size_t len,
                utl_byte_t digest[UTL_SHA256_DIGEST_SIZE])
{
    utl_sha256_ctx_t ctx;
    utl_sha256_init(&ctx);
    utl_sha256_update(&ctx, data, len);
    utl_sha256_final(&ctx, digest);
}

void utl_sha256_hex(const utl_byte_t digest[UTL_SHA256_DIGEST_SIZE],
                    char hex[UTL_SHA256_HEX_SIZE])
{
    for (int i = 0; i < UTL_SHA256_DIGEST_SIZE; i++)
        snprintf(hex + i * 2, 3, "%02x", digest[i]);
    hex[UTL_SHA256_HEX_SIZE - 1] = '\0';
}

/* ======================== HMAC-SHA256 ======================== */

void utl_hmac_sha256(const utl_byte_t *key, size_t key_len,
                     const void *data, size_t data_len,
                     utl_byte_t digest[UTL_SHA256_DIGEST_SIZE])
{
    utl_byte_t k_ipad[64], k_opad[64], tk[UTL_SHA256_DIGEST_SIZE];

    if (key_len > 64) {
        utl_sha256(key, key_len, tk);
        key = tk;
        key_len = UTL_SHA256_DIGEST_SIZE;
    }

    memset(k_ipad, 0, 64); memset(k_opad, 0, 64);
    memcpy(k_ipad, key, key_len); memcpy(k_opad, key, key_len);

    for (int i = 0; i < 64; i++) { k_ipad[i] ^= 0x36; k_opad[i] ^= 0x5C; }

    utl_sha256_ctx_t ctx;
    utl_sha256_init(&ctx);
    utl_sha256_update(&ctx, k_ipad, 64);
    utl_sha256_update(&ctx, data, data_len);
    utl_sha256_final(&ctx, digest);

    utl_sha256_init(&ctx);
    utl_sha256_update(&ctx, k_opad, 64);
    utl_sha256_update(&ctx, digest, UTL_SHA256_DIGEST_SIZE);
    utl_sha256_final(&ctx, digest);
}
