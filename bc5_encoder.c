
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

void split_pairs(const uint8_t* bytes, size_t length, uint8_t** pitch_bytes, uint8_t** yaw_bytes);
void gen_palette_and_indexes(int start_x, int start_y, int stride, const uint8_t* bytes, uint8_t* palette, uint8_t* indexes);
void block_encode(int start_x, int start_y, int stride, const uint8_t* r_bytes, const uint8_t* g_bytes, 
                  uint8_t* r_palette, uint8_t* r_indexes, uint8_t* g_palette, uint8_t* g_indexes);
void pack_block(const uint8_t* rpal, const uint8_t* ridxs, const uint8_t* gpal, const uint8_t* gidxs, uint8_t* result);
uint8_t* compress_normalmap(const uint8_t* norm_bytes, int width, int height);

void split_pairs(const uint8_t* bytes, size_t length, uint8_t** pitch_bytes, uint8_t** yaw_bytes) {
    *pitch_bytes = (uint8_t*)malloc(length / 2);
    *yaw_bytes = (uint8_t*)malloc(length / 2);

    for (size_t i = 0, j = 0; i < length; i += 2, j++) {
        (*pitch_bytes)[j] = bytes[i];
        (*yaw_bytes)[j] = bytes[i + 1];
    }
}

void gen_palette_and_indexes(int start_x, int start_y, int stride, const uint8_t* bytes, uint8_t* palette, uint8_t* indexes) {
    uint8_t min_byte = 255, max_byte = 0;
    int idx = 0;
    float one_over_7 = 1.0f / 7.0f;

    // Find min and max bytes
    for (int y = start_y; y < start_y + 4; y++) {
        for (int x = start_x; x < start_x + 4; x++) {
            uint8_t byte = bytes[y * stride + x];
            if (byte < min_byte) min_byte = byte;
            if (byte > max_byte) max_byte = byte;
        }
    }

    palette[0] = min_byte;
    palette[1] = max_byte;

    // Generate indexes
    for (int y = start_y; y < start_y + 4; y++) {
        for (int x = start_x; x < start_x + 4; x++) {
            uint8_t tgt_color = bytes[y * stride + x];
            float best_dist = INFINITY;
            int best_idx = 0;

            for (int pidx = 0; pidx < 8; pidx++) {
                float a = (7 - pidx) * one_over_7;
                float b = pidx * one_over_7;
                float pal_color = palette[0] * a + palette[1] * b;
                float cur_dist = fabsf(pal_color - tgt_color);
                if (cur_dist < best_dist) {
                    best_dist = cur_dist;
                    best_idx = pidx;
                }
            }

            indexes[idx++] = best_idx;
        }
    }
}

void block_encode(int start_x, int start_y, int stride, const uint8_t* r_bytes, const uint8_t* g_bytes, 
                  uint8_t* r_palette, uint8_t* r_indexes, uint8_t* g_palette, uint8_t* g_indexes) {
    gen_palette_and_indexes(start_x, start_y, stride, r_bytes, r_palette, r_indexes);
    gen_palette_and_indexes(start_x, start_y, stride, g_bytes, g_palette, g_indexes);
}

void pack_block(const uint8_t* rpal, const uint8_t* ridxs, const uint8_t* gpal, const uint8_t* gidxs, uint8_t* result) {
    result[0] = rpal[0];
    result[1] = rpal[1];

    uint32_t r0s = ridxs[0] | (ridxs[1] << 3) | (ridxs[2] << 6) | (ridxs[3] << 9) |
                   (ridxs[4] << 12) | (ridxs[5] << 15) | (ridxs[6] << 18) | (ridxs[7] << 21) |
                   (ridxs[8] << 24) | (ridxs[9] << 27) | ((ridxs[10] & 0x3) << 30);

    uint16_t r1s = ((ridxs[10] & 0x4) >> 2) | (ridxs[11] << 1) | (ridxs[12] << 4) | (ridxs[13] << 7) |
                   (ridxs[14] << 10) | (ridxs[15] << 13);

    result[2] = r0s & 0xFF;
    result[3] = (r0s >> 8) & 0xFF;
    result[4] = (r0s >> 16) & 0xFF;
    result[5] = (r0s >> 24) & 0xFF;

    result[6] = r1s & 0xFF;
    result[7] = (r1s >> 8) & 0xFF;

    result[8] = gpal[0];
    result[9] = gpal[1];

    uint32_t g0s = gidxs[0] | (gidxs[1] << 3) | (gidxs[2] << 6) | (gidxs[3] << 9) |
                   (gidxs[4] << 12) | (gidxs[5] << 15) | (gidxs[6] << 18) | (gidxs[7] << 21) |
                   (gidxs[8] << 24) | (gidxs[9] << 27) | ((gidxs[10] & 0x3) << 30);

    uint16_t g1s = ((gidxs[10] & 0x4) >> 2) | (gidxs[11] << 1) | (gidxs[12] << 4) | (gidxs[13] << 7) |
                   (gidxs[14] << 10) | (gidxs[15] << 13);

    result[10] = g0s & 0xFF;
    result[11] = (g0s >> 8) & 0xFF;
    result[12] = (g0s >> 16) & 0xFF;
    result[13] = (g0s >> 24) & 0xFF;

    result[14] = g1s & 0xFF;
    result[15] = (g1s >> 8) & 0xFF;
}

uint8_t* compress_normalmap(const uint8_t* norm_bytes, int width, int height) {
    size_t compressed_size;
    uint8_t *pitch_bytes, *yaw_bytes;
    split_pairs(norm_bytes, width * height * 2, &pitch_bytes, &yaw_bytes);

    int width_in_blocks = width / 4;
    int height_in_blocks = height / 4;

    compressed_size = width_in_blocks * height_in_blocks * 16;
    uint8_t* all_block_bytes = (uint8_t*)malloc(compressed_size);

    for (int y = 0; y < height_in_blocks; y++) {
        for (int x = 0; x < width_in_blocks; x++) {
            uint8_t r_palette[2], g_palette[2], r_indexes[16], g_indexes[16];
            block_encode(x * 4, y * 4, width, pitch_bytes, yaw_bytes, r_palette, r_indexes, g_palette, g_indexes);
            pack_block(r_palette, r_indexes, g_palette, g_indexes, all_block_bytes + (y * width_in_blocks + x) * 16);
        }
    }

    free(pitch_bytes);
    free(yaw_bytes);

    return all_block_bytes;
}
