#include "BambuKeyDeriver.h"
#include <mbedtls/md.h>
#include <cstring>

static const uint8_t MASTER_KEY[16] = {
    0x9A, 0x75, 0x9C, 0xF2, 0xC4, 0xF7, 0xCA, 0xFF,
    0x22, 0x2C, 0xB9, 0x76, 0x9B, 0x41, 0xBC, 0x96
};

static const uint8_t CONTEXT[] = {
    'R', 'F', 'I', 'D', '-', 'A', 0x00
};
static const size_t CONTEXT_LEN = 7;

BambuKeys deriveBambuKeys(const uint8_t* uid, uint8_t uidLen) {
    BambuKeys result;

    // HKDF-Extract: PRK = HMAC-SHA256(salt=master_key, IKM=uid)
    const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    uint8_t prk[32];
    mbedtls_md_hmac(md_info, MASTER_KEY, 16, uid, uidLen, prk);

    // HKDF-Expand: generate 96 bytes (3 blocks of 32 bytes each)
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, md_info, 1);

    uint8_t previous[32] = {0};
    uint8_t T[32];
    size_t bytes_generated = 0;
    uint8_t counter = 1;

    // Generate 3 blocks of 32 bytes each
    for (int i = 0; i < 3; i++) {
        mbedtls_md_hmac_starts(&ctx, prk, 32);

        // Add previous output (empty for first iteration)
        if (i > 0) {
            mbedtls_md_hmac_update(&ctx, previous, 32);
        }

        // Add context
        mbedtls_md_hmac_update(&ctx, CONTEXT, CONTEXT_LEN);

        // Add counter byte
        mbedtls_md_hmac_update(&ctx, &counter, 1);

        mbedtls_md_hmac_finish(&ctx, T);

        // Copy output (all 32 bytes for the last block, exact amount for final block)
        size_t copy_len = (bytes_generated + 32 <= 96) ? 32 : (96 - bytes_generated);
        memcpy(&result.keys[bytes_generated], T, copy_len);
        bytes_generated += copy_len;

        // Save T for next iteration
        memcpy(previous, T, 32);
        counter++;
    }

    mbedtls_md_free(&ctx);

    return result;
}
