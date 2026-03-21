#include <cstdio>
#include <cstring>
#include <cstdint>
#include "TigerTagParser.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT_EQ(a, b, msg) do { \
    if ((a) == (b)) { tests_passed++; } \
    else { printf("  FAIL: %s: expected %d, got %d\n", msg, (int)(b), (int)(a)); tests_failed++; } \
} while(0)

#define ASSERT_TRUE(a, msg) do { \
    if ((a)) { tests_passed++; } \
    else { printf("  FAIL: %s: expected true\n", msg); tests_failed++; } \
} while(0)

#define ASSERT_FALSE(a, msg) do { \
    if (!(a)) { tests_passed++; } \
    else { printf("  FAIL: %s: expected false\n", msg); tests_failed++; } \
} while(0)

#define ASSERT_STR_EQ(a, b, msg) do { \
    if (strcmp((a), (b)) == 0) { tests_passed++; } \
    else { printf("  FAIL: %s: expected '%s', got '%s'\n", msg, (b), (a)); tests_failed++; } \
} while(0)

#define ASSERT_FLOAT_EQ(a, b, msg) do { \
    float diff = (a) - (b); if (diff < 0) diff = -diff; \
    if (diff < 0.01f) { tests_passed++; } \
    else { printf("  FAIL: %s: expected %.3f, got %.3f\n", msg, (double)(b), (double)(a)); tests_failed++; } \
} while(0)

/* Build a valid TigerTag V1.0 Maker payload (40 bytes, pages 4-13) */
static void build_tigertag_payload(uint8_t* data) {
    memset(data, 0, 40);

    /* Version ID: V1.0 Maker (bytes 0-3) */
    data[0] = 0x5B; data[1] = 0xF5; data[2] = 0x92; data[3] = 0x64;

    /* Product ID (bytes 4-7) */
    data[4] = 0xFF; data[5] = 0xFF; data[6] = 0xFF; data[7] = 0xFF;

    /* Material ID: PLA = 38219 (bytes 8-9, big-endian) */
    data[8] = 0x95; data[9] = 0x4B;

    /* Aspect IDs (bytes 10-11): 255 = None */
    data[10] = 0xFF; data[11] = 0xFF;

    /* Type ID (byte 12): 0x8E = Filament */
    data[12] = 0x8E;

    /* Diameter ID (byte 13): 56 = 1.75mm */
    data[13] = 56;

    /* Brand ID: Polymaker = 50604 (bytes 14-15, big-endian) */
    data[14] = 0xC5; data[15] = 0xAC;

    /* Color RGBA (bytes 16-19): Red */
    data[16] = 0xFF; data[17] = 0x00; data[18] = 0x00; data[19] = 0xFF;

    /* Weight: 1000g (bytes 20-22, big-endian 3 bytes) */
    data[20] = 0x00; data[21] = 0x03; data[22] = 0xE8;

    /* Unit ID (byte 23): 21 = grams */
    data[23] = 21;

    /* Nozzle temps (bytes 24-27, big-endian uint16s): 190-220 */
    data[24] = 0x00; data[25] = 0xBE;  /* 190 */
    data[26] = 0x00; data[27] = 0xDC;  /* 220 */

    /* Dry temp/time (bytes 28-29) */
    data[28] = 50;  /* 50°C */
    data[29] = 4;   /* 4 hours */

    /* Bed temps (bytes 30-31) */
    data[30] = 50;  /* min */
    data[31] = 65;  /* max */
}

static void test_check_magic_v10(void) {
    printf("Running test_check_magic_v10...\n");
    uint8_t data[40];
    build_tigertag_payload(data);
    ASSERT_TRUE(tigerTagCheckMagic(data, 40), "V1.0 Maker magic");
}

static void test_check_magic_v10_original(void) {
    printf("Running test_check_magic_v10_original...\n");
    uint8_t data[14] = {0};
    data[0] = 0x5B; data[1] = 0xF0; data[2] = 0x46; data[3] = 0x74;  /* TIGERTAG_V10 */
    ASSERT_TRUE(tigerTagCheckMagic(data, 14), "V1.0 original magic");
}

static void test_check_magic_init(void) {
    printf("Running test_check_magic_init...\n");
    uint8_t data[14] = {0};
    data[0] = 0x6C; data[1] = 0x2B; data[2] = 0x2D; data[3] = 0xF1;  /* TIGERTAG_INIT_V10 */
    ASSERT_TRUE(tigerTagCheckMagic(data, 14), "Init V1.0 magic");
}

static void test_check_magic_plus(void) {
    printf("Running test_check_magic_plus...\n");
    uint8_t data[14] = {0};
    data[0] = 0xBC; data[1] = 0x0A; data[2] = 0x59; data[3] = 0x27;  /* TIGERTAG_PLUS_V10 */
    ASSERT_TRUE(tigerTagCheckMagic(data, 14), "Plus V1.0 magic");
}

static void test_check_magic_unknown_with_type_field(void) {
    printf("Running test_check_magic_unknown_with_type_field...\n");
    /* Unknown version ID but valid type field and non-zero material */
    uint8_t data[14] = {0};
    data[0] = 0xDE; data[1] = 0xAD; data[2] = 0xBE; data[3] = 0xEF;  /* Unknown version */
    data[8] = 0x00; data[9] = 0x01;  /* Non-zero material ID */
    data[13] = 0x8E;  /* Filament type — byte 13 is Type field, not byte 12 */
    ASSERT_TRUE(tigerTagCheckMagic(data, 14), "unknown version with valid type+material");
}

static void test_check_magic_fails_empty(void) {
    printf("Running test_check_magic_fails_empty...\n");
    uint8_t data[40] = {0};
    ASSERT_FALSE(tigerTagCheckMagic(data, 40), "all zeros should fail");
}

static void test_check_magic_too_short(void) {
    printf("Running test_check_magic_too_short...\n");
    uint8_t data[3] = {0x5B, 0xF5, 0x92};
    ASSERT_FALSE(tigerTagCheckMagic(data, 3), "3 bytes too short");
}

static void test_parse_full(void) {
    printf("Running test_parse_full...\n");
    uint8_t data[40];
    build_tigertag_payload(data);

    TigerTagData tt = tigerTagParse(data, 40);

    ASSERT_TRUE(tt.valid, "parse valid");
    ASSERT_EQ(tt.material_id, 38219, "material_id PLA");
    ASSERT_EQ(tt.diameter_id, 56, "diameter_id");
    ASSERT_EQ(tt.type_id, 0x8E, "type_id filament");
    ASSERT_EQ(tt.color_r, 0xFF, "color_r");
    ASSERT_EQ(tt.color_g, 0x00, "color_g");
    ASSERT_EQ(tt.color_b, 0x00, "color_b");
    ASSERT_EQ(tt.color_a, 0xFF, "color_a");
    ASSERT_EQ(tt.weight_g, 1000, "weight_g");
    ASSERT_EQ(tt.nozzle_temp_min, 190, "nozzle_temp_min");
    ASSERT_EQ(tt.nozzle_temp_max, 220, "nozzle_temp_max");
    ASSERT_EQ(tt.bed_temp_min, 50, "bed_temp_min");
    ASSERT_EQ(tt.bed_temp_max, 65, "bed_temp_max");
    ASSERT_EQ(tt.dry_temp, 50, "dry_temp");
    ASSERT_EQ(tt.dry_time_hours, 4, "dry_time_hours");
    ASSERT_FLOAT_EQ(tt.diameter_mm, 1.75f, "diameter_mm");
}

static void test_parse_lookup_names(void) {
    printf("Running test_parse_lookup_names...\n");
    uint8_t data[40];
    build_tigertag_payload(data);

    TigerTagData tt = tigerTagParse(data, 40);

    ASSERT_TRUE(tt.valid, "parse valid");
    /* PLA material ID 38219 should resolve */
    ASSERT_STR_EQ(tt.material_name, "PLA", "material_name PLA");
    /* Polymaker brand ID 16725 should resolve */
    ASSERT_STR_EQ(tt.brand_name, "Polymaker", "brand_name Polymaker");
}

static void test_parse_too_short(void) {
    printf("Running test_parse_too_short...\n");
    uint8_t data[10] = {0x5B, 0xF5, 0x92, 0x64};  /* Valid magic but too short */
    TigerTagData tt = tigerTagParse(data, 10);
    ASSERT_FALSE(tt.valid, "too short should be invalid");
}

static void test_material_lookup_known(void) {
    printf("Running test_material_lookup_known...\n");
    ASSERT_STR_EQ(tigerTagMaterialName(38219), "PLA", "PLA lookup");
    ASSERT_STR_EQ(tigerTagMaterialName(38256), "PETG", "PETG lookup");
    ASSERT_STR_EQ(tigerTagMaterialName(20562), "ABS", "ABS lookup");
    ASSERT_STR_EQ(tigerTagMaterialName(43518), "TPU", "TPU lookup");
}

static void test_material_lookup_unknown(void) {
    printf("Running test_material_lookup_unknown...\n");
    ASSERT_STR_EQ(tigerTagMaterialName(0), "Unknown", "zero ID");
    ASSERT_STR_EQ(tigerTagMaterialName(65535), "None", "max ID = None");
}

static void test_brand_lookup_known(void) {
    printf("Running test_brand_lookup_known...\n");
    ASSERT_STR_EQ(tigerTagBrandName(50604), "Polymaker", "Polymaker lookup");
}

static void test_brand_lookup_unknown(void) {
    printf("Running test_brand_lookup_unknown...\n");
    ASSERT_STR_EQ(tigerTagBrandName(0), "Unknown", "zero brand ID");
    ASSERT_STR_EQ(tigerTagBrandName(65535), "Generic", "max brand ID = Generic");
}

static void test_diameter_lookup(void) {
    printf("Running test_diameter_lookup...\n");
    ASSERT_FLOAT_EQ(tigerTagDiameterMm(56), 1.75f, "1.75mm");
    ASSERT_FLOAT_EQ(tigerTagDiameterMm(221), 2.85f, "2.85mm");
    ASSERT_FLOAT_EQ(tigerTagDiameterMm(0), 0.0f, "unknown diameter");
}

static void test_parse_285mm(void) {
    printf("Running test_parse_285mm...\n");
    uint8_t data[40];
    build_tigertag_payload(data);
    data[13] = 221;  /* 2.85mm */

    TigerTagData tt = tigerTagParse(data, 40);
    ASSERT_TRUE(tt.valid, "valid");
    ASSERT_FLOAT_EQ(tt.diameter_mm, 2.85f, "diameter 2.85mm");
}

static void test_parse_heavy_spool(void) {
    printf("Running test_parse_heavy_spool...\n");
    uint8_t data[40];
    build_tigertag_payload(data);
    /* Weight: 5000g (bytes 20-22, big-endian 3 bytes) */
    data[20] = 0x00; data[21] = 0x13; data[22] = 0x88;

    TigerTagData tt = tigerTagParse(data, 40);
    ASSERT_TRUE(tt.valid, "valid");
    ASSERT_EQ(tt.weight_g, 5000, "weight 5000g");
}

int main(void) {
    printf("\n=== TigerTag Parser Unit Tests ===\n\n");

    test_check_magic_v10();
    test_check_magic_v10_original();
    test_check_magic_init();
    test_check_magic_plus();
    test_check_magic_unknown_with_type_field();
    test_check_magic_fails_empty();
    test_check_magic_too_short();
    test_parse_full();
    test_parse_lookup_names();
    test_parse_too_short();
    test_material_lookup_known();
    test_material_lookup_unknown();
    test_brand_lookup_known();
    test_brand_lookup_unknown();
    test_diameter_lookup();
    test_parse_285mm();
    test_parse_heavy_spool();

    printf("\n=== Results: %d passed, %d failed ===\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
