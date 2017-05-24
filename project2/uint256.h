#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#ifndef __BYTEDEF__
#define __BYTEDEF__
typedef unsigned char BYTE;
#endif

//typedef unsigned char BYTE;

static inline void uint256_init (BYTE *uint256) {
    if (uint256 == NULL) {
        return;
    }
    size_t i = 0;
    for (i = 0; i < 32; uint256[i++] = 0);
}

static inline void print_uint256 (BYTE *uint256) {
    printf ("0x");
    size_t i = 0;
    for (i = 0; i < 32; i++) {
        printf ("%02x", uint256[i]);
    }
    printf ("\n");
}

static inline void uint256_sl (BYTE *res, BYTE *a, BYTE shift) {
    if (res == NULL || a == NULL) {
        return;
    }
    BYTE b = shift / 8;
    BYTE move_n = shift % 8;
    BYTE mask = 0;
    BYTE carry = 0;
    if (shift == 0) {
        memcpy (res, a, 32);
        return;
    }
    mask = 0xff << (8 - move_n);
    int i, j;
    for (i = 31 - b, j = 31; i > -1; i--, j--) {
        res[i] = (a[j] << move_n) | carry;
        carry = (a[j] & mask) >> move_n;
    }
}

static inline void uint256_add (BYTE *res, BYTE *a, BYTE *b) {
    if (res == NULL || a == NULL || b == NULL) {
        return;
    }
    BYTE aa[32], bb[32];
    uint256_init (aa);
    uint256_init (bb);

    memcpy (aa, a, 32);
    memcpy (bb, b, 32);
    uint16_t temp = 0;
    // Invalid Integer Overflow Bug discovered by Ziren Xiao on 09.05.2017 -
    // 23:36:40
    int i;
    for (i = 31; i > -1; i--) {
        temp >>= 8;
        temp += aa[i];
        temp += bb[i];
        res[i] = (BYTE) (temp & 0xff);
    }
}

static inline void uint256_mul (BYTE *res, BYTE *a, BYTE *b) {
    if (res == NULL || a == NULL || b == NULL) {
        return;
    }

    BYTE temp[32], acc[32], aa[32], bb[32];
    // we want to assert the invariance of a, b in the event that
    // a == res || b == res
    uint256_init (temp);
    uint256_init (aa);
    uint256_init (bb);
    uint256_init (acc);

    memcpy (bb, b, 32);
    memcpy (aa, a, 32);

    int i;
    for (i = 255; i > -1; i--) {
        if ((bb[i/8] & (1 << (7 - (i % 8)))) > 0) {
            uint256_sl (temp, aa, 255 - i);
            uint256_add (acc, acc, temp);
        }
    }
    memcpy (res, acc, 32);
}

// we bound the exponent to 4 bytes as it is pointless to support an
// integer greater than that, it'd simply overflow beyond 256bits.
static inline void uint256_exp (BYTE *res, BYTE *base, uint32_t exp) {
    if (res == NULL) {
        return;
    }
    if (exp == 0) {
        size_t i;
        for (i = 0; i < 31; res[i++] = 0);
        res[31] = 0x1;
        return;
    }

    BYTE temp[32], acc[32];
    uint256_init (temp);
    uint256_init (acc);

    if (res != base) {
        memcpy (acc, base, 32);
    }

    temp[31] = 0x1;

    while (exp > 1) {
        if (exp % 2 == 0) {
            uint256_mul (acc, acc, acc);
            exp = exp / 2;

        } else {
            uint256_mul (temp, acc, temp);
            uint256_mul (acc, acc, acc);
            exp = (exp - 1) / 2;
        }
    }
    uint256_mul (res, acc, temp);
}
