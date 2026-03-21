/**
 * OpenTag3D Parser Unit Tests (pure C)
 *
 * Tests for the OpenTag3D binary format encoder/decoder.
 * Can be compiled and run on host machine without ESP32.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "opentag3d_lib.h"

/*============================================================================
 * Test Framework Macros
 *============================================================================*/

#define ASSERT_EQ(expected, actual) do { \
    if ((expected) != (actual)) { \
        printf("  FAIL: %s:%d: expected %d, got %d\n", __FILE__, __LINE__, \
               (int)(expected), (int)(actual)); \
        return 1; \
    } \
} while(0)

#define ASSERT_FLOAT_EQ(expected, actual, epsilon) do { \
    float _diff = fabsf((float)(expected) - (float)(actual)); \
    if (_diff > (epsilon)) { \
        printf("  FAIL: %s:%d: expected %f, got %f (diff %f)\n", \
               __FILE__, __LINE__, (double)(expected), (double)(actual), (double)_diff); \
        return 1; \
    } \
} while(0)

#define ASSERT_STR_EQ(expected, actual) do { \
    if (strcmp((expected), (actual)) != 0) { \
        printf("  FAIL: %s:%d: expected '%s', got '%s'\n", \
               __FILE__, __LINE__, (expected), (actual)); \
        return 1; \
    } \
} while(0)

#define RUN_TEST(test_func) do { \
    printf("Running %s... ", #test_func); \
    int _result = test_func(); \
    if (_result == 0) { \
        printf("PASS\n"); \
        passed++; \
    } else { \
        failed++; \
    } \
    total++; \
} while(0)

/*============================================================================
 * Helpers
 *============================================================================*/

/* Write big-endian uint16 into a buffer at offset */
static void put_u16(uint8_t *buf, size_t offset, uint16_t val) {
    buf[offset]     = (uint8_t)(val >> 8);
    buf[offset + 1] = (uint8_t)(val & 0xFF);
}

/* Write a fixed-length padded string into a buffer at offset */
static void put_str(uint8_t *buf, size_t offset, const char *s, size_t field_len) {
    size_t slen = strlen(s);
    if (slen > field_len) slen = field_len;
    memcpy(buf + offset, s, slen);
    if (slen < field_len) {
        memset(buf + offset + slen, ' ', field_len - slen);
    }
}

/*============================================================================
 * Tests
 *============================================================================*/

int test_decode_core(void) {
    uint8_t payload[OT3D_CORE_SIZE];
    memset(payload, 0, sizeof(payload));

    /* version 1000 */
    put_u16(payload, 0x00, 1000);
    /* base_material: "PLA" (5 bytes at 0x02) */
    put_str(payload, 0x02, "PLA", 5);
    /* material_modifiers: "CF" (5 bytes at 0x07) */
    put_str(payload, 0x07, "CF", 5);
    /* manufacturer: "Polymaker" (16 bytes at 0x1B) */
    put_str(payload, 0x1B, "Polymaker", 16);
    /* color_name: "Galaxy Purple" (32 bytes at 0x2B) */
    put_str(payload, 0x2B, "Galaxy Purple", 32);
    /* color RGBA[0]: 0x8000FFFF */
    payload[0x4B] = 0x80;
    payload[0x4C] = 0x00;
    payload[0x4D] = 0xFF;
    payload[0x4E] = 0xFF;
    /* diameter 1750 um */
    put_u16(payload, 0x5C, 1750);
    /* weight 1000g */
    put_u16(payload, 0x5E, 1000);
    /* print_temp_encoded: 42 (210C / 5) */
    payload[0x60] = 42;
    /* bed_temp_encoded: 12 (60C / 5) */
    payload[0x61] = 12;
    /* density 1240 ug/cm3 */
    put_u16(payload, 0x62, 1240);
    /* transmission distance 50 */
    put_u16(payload, 0x64, 50);

    opentag3d_t out;
    opentag3d_result_t rc = opentag3d_decode(payload, sizeof(payload), &out);

    ASSERT_EQ(OT3D_OK, rc);
    ASSERT_EQ(1000, out.tag_version);
    ASSERT_STR_EQ("PLA", out.base_material);
    ASSERT_STR_EQ("CF", out.material_modifiers);
    ASSERT_STR_EQ("Polymaker", out.manufacturer);
    ASSERT_STR_EQ("Galaxy Purple", out.color_name);
    ASSERT_EQ(0x80, out.color_rgba[0][0]);
    ASSERT_EQ(0x00, out.color_rgba[0][1]);
    ASSERT_EQ(0xFF, out.color_rgba[0][2]);
    ASSERT_EQ(0xFF, out.color_rgba[0][3]);
    ASSERT_EQ(1750, out.diameter_um);
    ASSERT_EQ(1000, out.target_weight_g);
    ASSERT_EQ(42, out.print_temp_encoded);
    ASSERT_EQ(12, out.bed_temp_encoded);
    ASSERT_EQ(1240, out.density_ugcm3);
    ASSERT_EQ(50, out.transmission_distance);
    ASSERT_EQ(0, out.has_extended);

    /* Verify helper functions */
    ASSERT_FLOAT_EQ(210.0f, opentag3d_temp_c(out.print_temp_encoded), 0.01f);
    ASSERT_FLOAT_EQ(60.0f, opentag3d_temp_c(out.bed_temp_encoded), 0.01f);
    ASSERT_FLOAT_EQ(1.75f, opentag3d_diameter_mm(&out), 0.01f);
    ASSERT_FLOAT_EQ(0.00124f, opentag3d_density_gcc(&out), 0.00001f);
    ASSERT_FLOAT_EQ(5.0f, opentag3d_transmission_mm(&out), 0.01f);

    return 0;
}

int test_decode_extended(void) {
    uint8_t payload[OT3D_EXTENDED_MIN];
    memset(payload, 0, sizeof(payload));

    /* Same core fields as test_decode_core */
    put_u16(payload, 0x00, 1000);
    put_str(payload, 0x02, "PLA", 5);
    put_str(payload, 0x07, "CF", 5);
    put_str(payload, 0x1B, "Polymaker", 16);
    put_str(payload, 0x2B, "Galaxy Purple", 32);
    payload[0x4B] = 0x80; payload[0x4C] = 0x00;
    payload[0x4D] = 0xFF; payload[0x4E] = 0xFF;
    put_u16(payload, 0x5C, 1750);
    put_u16(payload, 0x5E, 1000);
    payload[0x60] = 42;
    payload[0x61] = 12;
    put_u16(payload, 0x62, 1240);
    put_u16(payload, 0x64, 50);

    /* Extended fields */
    /* online_url (32 bytes at 0x70) — leave empty for this test */
    /* serial_number (16 bytes at 0x90) */
    put_str(payload, 0x90, "SN-2026-001", 16);
    /* manufacture date: 2026-03-15 14:30:00 */
    put_u16(payload, 0xA0, 2026);
    payload[0xA2] = 3;   /* month */
    payload[0xA3] = 15;  /* day */
    payload[0xA4] = 14;  /* hour */
    payload[0xA5] = 30;  /* minute */
    payload[0xA6] = 0;   /* second */
    /* spool_core_diameter_mm */
    payload[0xA7] = 200;
    /* MFI fields */
    payload[0xA8] = 0;   /* mfi_temp_encoded */
    payload[0xA9] = 0;   /* mfi_load */
    payload[0xAA] = 0;   /* mfi_value */
    /* empty spool weight 250g */
    put_u16(payload, 0xAC, 250);
    /* measured filament weight 980g */
    put_u16(payload, 0xAE, 980);
    /* measured filament length 330m */
    put_u16(payload, 0xB0, 330);
    /* dry: 50C/4h -> encoded = 50/5 = 10 */
    payload[0xB2] = 10;
    payload[0xB3] = 4;
    /* temps: 190-230C print -> encoded 38, 46 */
    payload[0xB4] = 38;
    payload[0xB5] = 46;
    /* temps: 50-70C bed -> encoded 10, 14 */
    payload[0xB6] = 10;
    payload[0xB7] = 14;
    /* volumetric: min=5, max=20, target=12 */
    payload[0xB8] = 5;
    payload[0xB9] = 20;
    payload[0xBA] = 12;

    opentag3d_t out;
    opentag3d_result_t rc = opentag3d_decode(payload, sizeof(payload), &out);

    ASSERT_EQ(OT3D_OK, rc);
    ASSERT_EQ(1, out.has_extended);

    /* Verify core still correct */
    ASSERT_STR_EQ("PLA", out.base_material);
    ASSERT_STR_EQ("CF", out.material_modifiers);
    ASSERT_STR_EQ("Polymaker", out.manufacturer);

    /* Verify extended fields */
    ASSERT_STR_EQ("SN-2026-001", out.serial_number);
    ASSERT_EQ(2026, out.manufacture_year);
    ASSERT_EQ(3, out.manufacture_month);
    ASSERT_EQ(15, out.manufacture_day);
    ASSERT_EQ(14, out.manufacture_hour);
    ASSERT_EQ(30, out.manufacture_minute);
    ASSERT_EQ(0, out.manufacture_second);
    ASSERT_EQ(200, out.spool_core_diameter_mm);
    ASSERT_EQ(250, out.empty_spool_weight_g);
    ASSERT_EQ(980, out.measured_filament_weight_g);
    ASSERT_EQ(330, out.measured_filament_length_m);
    ASSERT_EQ(10, out.max_dry_temp_encoded);
    ASSERT_FLOAT_EQ(50.0f, opentag3d_temp_c(out.max_dry_temp_encoded), 0.01f);
    ASSERT_EQ(4, out.dry_time_hours);
    ASSERT_EQ(38, out.min_print_temp_encoded);
    ASSERT_EQ(46, out.max_print_temp_encoded);
    ASSERT_FLOAT_EQ(190.0f, opentag3d_temp_c(out.min_print_temp_encoded), 0.01f);
    ASSERT_FLOAT_EQ(230.0f, opentag3d_temp_c(out.max_print_temp_encoded), 0.01f);
    ASSERT_EQ(10, out.min_bed_temp_encoded);
    ASSERT_EQ(14, out.max_bed_temp_encoded);
    ASSERT_FLOAT_EQ(50.0f, opentag3d_temp_c(out.min_bed_temp_encoded), 0.01f);
    ASSERT_FLOAT_EQ(70.0f, opentag3d_temp_c(out.max_bed_temp_encoded), 0.01f);
    ASSERT_EQ(5, out.min_volumetric_speed);
    ASSERT_EQ(20, out.max_volumetric_speed);
    ASSERT_EQ(12, out.target_volumetric_speed);

    return 0;
}

int test_encode_decode_roundtrip(void) {
    opentag3d_t original;
    memset(&original, 0, sizeof(original));

    original.tag_version = OT3D_SUPPORTED_VERSION;
    strncpy(original.base_material, "PETG", sizeof(original.base_material) - 1);
    strncpy(original.material_modifiers, "", sizeof(original.material_modifiers) - 1);
    strncpy(original.manufacturer, "Sunlu", sizeof(original.manufacturer) - 1);
    strncpy(original.color_name, "Orange", sizeof(original.color_name) - 1);
    original.color_rgba[0][0] = 0xFF;
    original.color_rgba[0][1] = 0xA5;
    original.color_rgba[0][2] = 0x00;
    original.color_rgba[0][3] = 0xFF;
    original.diameter_um = 1750;
    original.target_weight_g = 1000;
    original.print_temp_encoded = 46;   /* 230C */
    original.bed_temp_encoded = 16;     /* 80C */
    original.density_ugcm3 = 1270;
    original.transmission_distance = 40;

    /* Extended fields */
    original.has_extended = 1;
    strncpy(original.serial_number, "SUNLU-ORG-001", sizeof(original.serial_number) - 1);
    strncpy(original.online_url, "sunlu.com/petg-orange", sizeof(original.online_url) - 1);
    original.manufacture_year = 2026;
    original.manufacture_month = 1;
    original.manufacture_day = 20;
    original.manufacture_hour = 10;
    original.manufacture_minute = 0;
    original.manufacture_second = 0;
    original.spool_core_diameter_mm = 200;
    original.empty_spool_weight_g = 230;
    original.measured_filament_weight_g = 995;
    original.measured_filament_length_m = 340;
    original.max_dry_temp_encoded = 13;   /* 65C */
    original.dry_time_hours = 6;
    original.min_print_temp_encoded = 44; /* 220C */
    original.max_print_temp_encoded = 50; /* 250C */
    original.min_bed_temp_encoded = 14;   /* 70C */
    original.max_bed_temp_encoded = 18;   /* 90C */
    original.min_volumetric_speed = 8;
    original.max_volumetric_speed = 24;
    original.target_volumetric_speed = 15;

    /* Encode */
    uint8_t buf[OT3D_EXTENDED_MIN];
    int written = opentag3d_encode(&original, buf, sizeof(buf));
    ASSERT_EQ(OT3D_EXTENDED_MIN, written);

    /* Decode back */
    opentag3d_t decoded;
    opentag3d_result_t rc = opentag3d_decode(buf, (size_t)written, &decoded);
    ASSERT_EQ(OT3D_OK, rc);

    /* Verify all fields survived the roundtrip */
    ASSERT_EQ(original.tag_version, decoded.tag_version);
    ASSERT_STR_EQ("PETG", decoded.base_material);
    ASSERT_STR_EQ("Sunlu", decoded.manufacturer);
    ASSERT_STR_EQ("Orange", decoded.color_name);
    ASSERT_EQ(original.color_rgba[0][0], decoded.color_rgba[0][0]);
    ASSERT_EQ(original.color_rgba[0][1], decoded.color_rgba[0][1]);
    ASSERT_EQ(original.color_rgba[0][2], decoded.color_rgba[0][2]);
    ASSERT_EQ(original.color_rgba[0][3], decoded.color_rgba[0][3]);
    ASSERT_EQ(original.diameter_um, decoded.diameter_um);
    ASSERT_EQ(original.target_weight_g, decoded.target_weight_g);
    ASSERT_EQ(original.print_temp_encoded, decoded.print_temp_encoded);
    ASSERT_EQ(original.bed_temp_encoded, decoded.bed_temp_encoded);
    ASSERT_EQ(original.density_ugcm3, decoded.density_ugcm3);
    ASSERT_EQ(original.transmission_distance, decoded.transmission_distance);

    /* Extended */
    ASSERT_EQ(1, decoded.has_extended);
    ASSERT_STR_EQ("SUNLU-ORG-001", decoded.serial_number);
    ASSERT_STR_EQ("sunlu.com/petg-orange", decoded.online_url);
    ASSERT_EQ(original.manufacture_year, decoded.manufacture_year);
    ASSERT_EQ(original.manufacture_month, decoded.manufacture_month);
    ASSERT_EQ(original.manufacture_day, decoded.manufacture_day);
    ASSERT_EQ(original.spool_core_diameter_mm, decoded.spool_core_diameter_mm);
    ASSERT_EQ(original.empty_spool_weight_g, decoded.empty_spool_weight_g);
    ASSERT_EQ(original.measured_filament_weight_g, decoded.measured_filament_weight_g);
    ASSERT_EQ(original.measured_filament_length_m, decoded.measured_filament_length_m);
    ASSERT_EQ(original.max_dry_temp_encoded, decoded.max_dry_temp_encoded);
    ASSERT_EQ(original.dry_time_hours, decoded.dry_time_hours);
    ASSERT_EQ(original.min_print_temp_encoded, decoded.min_print_temp_encoded);
    ASSERT_EQ(original.max_print_temp_encoded, decoded.max_print_temp_encoded);
    ASSERT_EQ(original.min_bed_temp_encoded, decoded.min_bed_temp_encoded);
    ASSERT_EQ(original.max_bed_temp_encoded, decoded.max_bed_temp_encoded);
    ASSERT_EQ(original.min_volumetric_speed, decoded.min_volumetric_speed);
    ASSERT_EQ(original.max_volumetric_speed, decoded.max_volumetric_speed);
    ASSERT_EQ(original.target_volumetric_speed, decoded.target_volumetric_speed);

    return 0;
}

int test_version_warning(void) {
    uint8_t payload[OT3D_CORE_SIZE];
    memset(payload, 0, sizeof(payload));

    /* Version 1001 — same major (1), minor ahead */
    put_u16(payload, 0x00, 1001);
    put_str(payload, 0x02, "PLA", 5);
    put_str(payload, 0x07, "", 5);
    put_str(payload, 0x1B, "Generic", 16);
    put_str(payload, 0x2B, "White", 32);
    put_u16(payload, 0x5C, 1750);
    put_u16(payload, 0x5E, 500);

    opentag3d_t out;
    opentag3d_result_t rc = opentag3d_decode(payload, sizeof(payload), &out);

    ASSERT_EQ(OT3D_VERSION_WARNING, rc);
    /* Should still parse successfully */
    ASSERT_EQ(1001, out.tag_version);
    ASSERT_STR_EQ("PLA", out.base_material);
    ASSERT_STR_EQ("Generic", out.manufacturer);
    ASSERT_EQ(1750, out.diameter_um);
    ASSERT_EQ(500, out.target_weight_g);

    return 0;
}

int test_version_error(void) {
    uint8_t payload[OT3D_CORE_SIZE];
    memset(payload, 0, sizeof(payload));

    /* Version 2000 — major version 2, ahead of reader major 1 */
    put_u16(payload, 0x00, 2000);
    put_str(payload, 0x02, "ABS", 5);
    put_u16(payload, 0x5C, 1750);

    opentag3d_t out;
    opentag3d_result_t rc = opentag3d_decode(payload, sizeof(payload), &out);

    ASSERT_EQ(OT3D_VERSION_ERROR, rc);

    return 0;
}

int test_too_short(void) {
    opentag3d_t out;
    opentag3d_result_t rc;

    /* NULL payload */
    rc = opentag3d_decode(NULL, 100, &out);
    ASSERT_EQ(OT3D_PARSE_ERROR, rc);

    /* NULL output */
    uint8_t dummy[4] = {0};
    rc = opentag3d_decode(dummy, sizeof(dummy), NULL);
    ASSERT_EQ(OT3D_PARSE_ERROR, rc);

    /* Zero length */
    rc = opentag3d_decode(dummy, 0, &out);
    ASSERT_EQ(OT3D_PARSE_ERROR, rc);

    /* 1 byte — too short for version */
    rc = opentag3d_decode(dummy, 1, &out);
    ASSERT_EQ(OT3D_PARSE_ERROR, rc);

    /* 2 bytes — has version but too short for core (valid version) */
    uint8_t ver_buf[2];
    put_u16(ver_buf, 0, 1000);
    rc = opentag3d_decode(ver_buf, 2, &out);
    ASSERT_EQ(OT3D_PARSE_ERROR, rc);

    /* Just under core size */
    uint8_t almost[OT3D_CORE_SIZE - 1];
    memset(almost, 0, sizeof(almost));
    put_u16(almost, 0, 1000);
    rc = opentag3d_decode(almost, sizeof(almost), &out);
    ASSERT_EQ(OT3D_PARSE_ERROR, rc);

    return 0;
}

int test_encode_core_only(void) {
    opentag3d_t tag;
    memset(&tag, 0, sizeof(tag));

    tag.tag_version = OT3D_SUPPORTED_VERSION;
    strncpy(tag.base_material, "ABS", sizeof(tag.base_material) - 1);
    strncpy(tag.manufacturer, "Hatchbox", sizeof(tag.manufacturer) - 1);
    strncpy(tag.color_name, "Black", sizeof(tag.color_name) - 1);
    tag.diameter_um = 1750;
    tag.target_weight_g = 1000;
    tag.print_temp_encoded = 48;  /* 240C */
    tag.bed_temp_encoded = 20;    /* 100C */
    tag.density_ugcm3 = 1040;

    /* Extended fields set but buffer only fits core */
    tag.has_extended = 1;
    strncpy(tag.serial_number, "IGNORED", sizeof(tag.serial_number) - 1);

    uint8_t buf[OT3D_CORE_SIZE];
    int written = opentag3d_encode(&tag, buf, sizeof(buf));

    ASSERT_EQ(OT3D_CORE_SIZE, written);

    /* Decode and verify core-only output */
    opentag3d_t decoded;
    opentag3d_result_t rc = opentag3d_decode(buf, (size_t)written, &decoded);
    ASSERT_EQ(OT3D_OK, rc);
    ASSERT_STR_EQ("ABS", decoded.base_material);
    ASSERT_STR_EQ("Hatchbox", decoded.manufacturer);
    ASSERT_EQ(1750, decoded.diameter_um);
    ASSERT_EQ(0, decoded.has_extended);

    return 0;
}

int test_encode_too_small(void) {
    opentag3d_t tag;
    memset(&tag, 0, sizeof(tag));
    tag.tag_version = OT3D_SUPPORTED_VERSION;

    /* Buffer too small for even core */
    uint8_t tiny[10];
    int written = opentag3d_encode(&tag, tiny, sizeof(tiny));
    ASSERT_EQ(-1, written);

    /* NULL buffer */
    written = opentag3d_encode(&tag, NULL, 256);
    ASSERT_EQ(-1, written);

    /* NULL tag */
    uint8_t buf[OT3D_CORE_SIZE];
    written = opentag3d_encode(NULL, buf, sizeof(buf));
    ASSERT_EQ(-1, written);

    return 0;
}

/*============================================================================
 * Main
 *============================================================================*/

int main(void) {
    int passed = 0, failed = 0, total = 0;

    printf("\n=== OpenTag3D Parser Unit Tests ===\n\n");

    RUN_TEST(test_decode_core);
    RUN_TEST(test_decode_extended);
    RUN_TEST(test_encode_decode_roundtrip);
    RUN_TEST(test_version_warning);
    RUN_TEST(test_version_error);
    RUN_TEST(test_too_short);
    RUN_TEST(test_encode_core_only);
    RUN_TEST(test_encode_too_small);

    printf("\n=== Results: %d/%d passed ===\n", passed, total);

    return failed > 0 ? 1 : 0;
}
