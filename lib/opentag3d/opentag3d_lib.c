#include "opentag3d_lib.h"
#include <string.h>

/* Read big-endian uint16 from buffer */
static uint16_t read_u16(const uint8_t *p) {
    return (uint16_t)((p[0] << 8) | p[1]);
}

/* Copy fixed-length field to null-terminated string, trimming trailing spaces */
static void read_str(const uint8_t *src, size_t src_len, char *dst, size_t dst_size) {
    size_t copy = src_len;
    if (copy >= dst_size) copy = dst_size - 1;
    memcpy(dst, src, copy);
    dst[copy] = '\0';
    /* Trim trailing spaces */
    while (copy > 0 && dst[copy - 1] == ' ') {
        dst[--copy] = '\0';
    }
}

opentag3d_result_t opentag3d_decode(const uint8_t *payload, size_t len, opentag3d_t *out) {
    if (out == NULL || payload == NULL) return OT3D_PARSE_ERROR;
    memset(out, 0, sizeof(opentag3d_t));

    /* Need at least the version field */
    if (len < 2) return OT3D_PARSE_ERROR;

    /* Version check */
    out->tag_version = read_u16(payload + 0x00);
    uint16_t major = out->tag_version / 1000;
    uint16_t reader_major = OT3D_SUPPORTED_VERSION / 1000;

    opentag3d_result_t version_result = OT3D_OK;
    if (major > reader_major) {
        return OT3D_VERSION_ERROR;
    }
    if (out->tag_version > OT3D_SUPPORTED_VERSION) {
        version_result = OT3D_VERSION_WARNING;
    }

    /* Need enough data for core fields */
    if (len < OT3D_CORE_SIZE) return OT3D_PARSE_ERROR;

    /* Core fields at fixed offsets */
    read_str(payload + 0x02, 5, out->base_material, sizeof(out->base_material));
    read_str(payload + 0x07, 5, out->material_modifiers, sizeof(out->material_modifiers));
    /* 0x0C - 0x1A: reserved/padding in spec */
    read_str(payload + 0x1B, 16, out->manufacturer, sizeof(out->manufacturer));
    read_str(payload + 0x2B, 32, out->color_name, sizeof(out->color_name));

    /* 4 RGBA colors */
    memcpy(out->color_rgba[0], payload + 0x4B, 4);
    memcpy(out->color_rgba[1], payload + 0x4F, 4);
    memcpy(out->color_rgba[2], payload + 0x53, 4);
    memcpy(out->color_rgba[3], payload + 0x57, 4);

    out->diameter_um            = read_u16(payload + 0x5C);
    out->target_weight_g        = read_u16(payload + 0x5E);
    out->print_temp_encoded     = payload[0x60];
    out->bed_temp_encoded       = payload[0x61];
    out->density_ugcm3          = read_u16(payload + 0x62);
    out->transmission_distance  = read_u16(payload + 0x64);

    /* Extended fields — parse if we have enough data */
    if (len >= OT3D_EXTENDED_MIN) {
        out->has_extended = 1;
        read_str(payload + 0x70, 32, out->online_url, sizeof(out->online_url));
        read_str(payload + 0x90, 16, out->serial_number, sizeof(out->serial_number));

        out->manufacture_year   = read_u16(payload + 0xA0);
        out->manufacture_month  = payload[0xA2];
        out->manufacture_day    = payload[0xA3];
        out->manufacture_hour   = payload[0xA4];
        out->manufacture_minute = payload[0xA5];
        out->manufacture_second = payload[0xA6];

        out->spool_core_diameter_mm = payload[0xA7];
        out->mfi_temp_encoded       = payload[0xA8];
        out->mfi_load               = payload[0xA9];
        out->mfi_value              = payload[0xAA];
        out->measured_tolerance_um  = payload[0xAB];
        out->empty_spool_weight_g   = read_u16(payload + 0xAC);
        out->measured_filament_weight_g = read_u16(payload + 0xAE);
        out->measured_filament_length_m = read_u16(payload + 0xB0);
        out->max_dry_temp_encoded   = payload[0xB2];
        out->dry_time_hours         = payload[0xB3];
        out->min_print_temp_encoded = payload[0xB4];
        out->max_print_temp_encoded = payload[0xB5];
        out->min_bed_temp_encoded   = payload[0xB6];
        out->max_bed_temp_encoded   = payload[0xB7];
        out->min_volumetric_speed   = payload[0xB8];
        out->max_volumetric_speed   = payload[0xB9];
        out->target_volumetric_speed = payload[0xBA];
    }

    return version_result;
}

/* Write big-endian uint16 to buffer */
static void write_u16(uint8_t *p, uint16_t val) {
    p[0] = (uint8_t)(val >> 8);
    p[1] = (uint8_t)(val & 0xFF);
}

/* Write fixed-length field, pad with spaces */
static void write_str(const char *src, uint8_t *dst, size_t field_len) {
    size_t slen = strlen(src);
    if (slen > field_len) slen = field_len;
    memcpy(dst, src, slen);
    if (slen < field_len) {
        memset(dst + slen, ' ', field_len - slen);
    }
}

int opentag3d_encode(const opentag3d_t *tag, uint8_t *buf, size_t buflen) {
    if (tag == NULL || buf == NULL) return -1;
    if (buflen < OT3D_CORE_SIZE) return -1;

    memset(buf, 0, buflen);

    /* Version */
    write_u16(buf + 0x00, tag->tag_version);

    /* Core fields */
    write_str(tag->base_material, buf + 0x02, 5);
    write_str(tag->material_modifiers, buf + 0x07, 5);
    /* 0x0C - 0x1A: zero (reserved) */
    write_str(tag->manufacturer, buf + 0x1B, 16);
    write_str(tag->color_name, buf + 0x2B, 32);

    /* 4 RGBA colors */
    memcpy(buf + 0x4B, tag->color_rgba[0], 4);
    memcpy(buf + 0x4F, tag->color_rgba[1], 4);
    memcpy(buf + 0x53, tag->color_rgba[2], 4);
    memcpy(buf + 0x57, tag->color_rgba[3], 4);

    write_u16(buf + 0x5C, tag->diameter_um);
    write_u16(buf + 0x5E, tag->target_weight_g);
    buf[0x60] = tag->print_temp_encoded;
    buf[0x61] = tag->bed_temp_encoded;
    write_u16(buf + 0x62, tag->density_ugcm3);
    write_u16(buf + 0x64, tag->transmission_distance);

    int written = OT3D_CORE_SIZE;

    /* Extended fields if buffer is large enough */
    if (buflen >= OT3D_EXTENDED_MIN) {
        write_str(tag->online_url, buf + 0x70, 32);
        write_str(tag->serial_number, buf + 0x90, 16);

        write_u16(buf + 0xA0, tag->manufacture_year);
        buf[0xA2] = tag->manufacture_month;
        buf[0xA3] = tag->manufacture_day;
        buf[0xA4] = tag->manufacture_hour;
        buf[0xA5] = tag->manufacture_minute;
        buf[0xA6] = tag->manufacture_second;

        buf[0xA7] = tag->spool_core_diameter_mm;
        buf[0xA8] = tag->mfi_temp_encoded;
        buf[0xA9] = tag->mfi_load;
        buf[0xAA] = tag->mfi_value;
        buf[0xAB] = tag->measured_tolerance_um;
        write_u16(buf + 0xAC, tag->empty_spool_weight_g);
        write_u16(buf + 0xAE, tag->measured_filament_weight_g);
        write_u16(buf + 0xB0, tag->measured_filament_length_m);
        buf[0xB2] = tag->max_dry_temp_encoded;
        buf[0xB3] = tag->dry_time_hours;
        buf[0xB4] = tag->min_print_temp_encoded;
        buf[0xB5] = tag->max_print_temp_encoded;
        buf[0xB6] = tag->min_bed_temp_encoded;
        buf[0xB7] = tag->max_bed_temp_encoded;
        buf[0xB8] = tag->min_volumetric_speed;
        buf[0xB9] = tag->max_volumetric_speed;
        buf[0xBA] = tag->target_volumetric_speed;

        written = OT3D_EXTENDED_MIN;
    }

    return written;
}
