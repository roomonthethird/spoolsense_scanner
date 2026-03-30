# Tag Writer Auto-Populate from Scanned Tag Data

**Issue:** #57
**Date:** 2026-03-30
**Status:** Approved

## Problem

When a tag with existing data is on the reader and the user navigates to a writer page, all form fields are blank. The user must re-enter everything from scratch even if they only want to change one field (e.g., update the color on an existing tag).

## Solution

Add a shared `prefillFromTag()` function to SharedJS that fetches `/api/status` on page load. If a tag with data is present, pre-fill the writer form fields. User can edit any field before writing. Works cross-format (scan a TigerTag, write as OpenPrintTag).

## Design Decisions

- **Cross-format:** All tag formats normalize to common fields. Material name string is the cross-format key.
- **Page load only:** Check once when the page opens. No polling. Existing material DB auto-fill still works if no tag is present.
- **Editable:** Pre-filled fields are a starting point, not locked. User edits override via `autoFilled` tracking.
- **Client-side only:** No firmware changes. Uses existing `/api/status` endpoint.
- **Shared code in SharedJS.h:** One `prefillFromTag()` function, three callers with field maps.

## Data Flow

```
Page Load → prefillFromTag(fieldMap) → fetch('/api/status')
    |
    Tag present + has data?
    |— No: return silently, user fills manually or uses material DB
    |— Yes: normalize tag data → fill form fields → mark autoFilled
```

## Cross-Format Material Mapping

`/api/status` returns different material representations per tag type:

| Source | Material field |
|--------|---------------|
| OpenPrintTag | `material_type` (enum) + `material_name` (string) |
| TigerTag | `tigertag.material_name` (string) + `tigertag.material_id` (int) |
| OpenTag3D | `opentag3d.base_material` (string) |
| NFC+ | `material_name` (string from Spoolman) |

`prefillFromTag()` normalizes all sources to a material name string (e.g., "PLA"). Each writer then maps to its own format:

- **OpenPrintTag writer:** name → material_type enum via JS lookup table
- **TigerTag writer:** name → search SpoolmanDB material list (already loaded), set material_id
- **OpenTag3D writer:** name → set base_material text field directly

## Field Mapping Per Writer

| Field | OpenPrintTag | TigerTag | OpenTag3D |
|-------|-------------|----------|-----------|
| Material | material_type dropdown | material_search input | base_material input |
| Color | colorHex + colorPicker | colorHex + colorPicker | colorHex + colorPicker |
| Manufacturer | manufacturer | brand_search | manufacturer |
| Weight (initial) | initial_weight_g | weight_g | target_weight_g |
| Weight (remaining) | remaining_g | — | — |
| Density | density | — | density |
| Diameter | diameter_mm | diameter_id dropdown | diameter_mm |
| Nozzle temp min | min_print_temp | nozzle_min | min_print_temp_c |
| Nozzle temp max | max_print_temp | nozzle_max | max_print_temp_c |
| Bed temp min | min_bed_temp | bed_min | min_bed_temp_c |
| Bed temp max | max_bed_temp | bed_max | max_bed_temp_c |
| Dry temp | — | dry_temp | max_dry_temp_c |
| Dry time | — | dry_time | dry_time_hours |

Fields not present on a writer page are skipped.

## Implementation Structure

### SharedJS.h

Add `prefillFromTag(fieldMap)`:
- Fetch `/api/status`
- If tag not present or no data, return null
- Extract normalized fields from response (handle all 4 tag types + NFC+)
- For each key in fieldMap, set form field value if data exists
- Mark filled fields as `autoFilled='true'`
- Sync color picker if color was set
- Return the normalized tag data so caller can do writer-specific work

### Each Writer Page (~10 lines each)

Call `prefillFromTag()` on page load with the writer's field map. Handle writer-specific follow-up:
- OpenPrintTag: sync material_type dropdown from material name
- TigerTag: sync material_search + brand_search inputs, set hidden material_id/brand_id
- OpenTag3D: set base_material text, sync modifiers if available

### No Firmware Changes

Uses existing `/api/status` endpoint which already returns all tag data for all formats.

## Files Modified

| File | Change |
|------|--------|
| src/SharedJS.h | Add `prefillFromTag()` function |
| src/OpenPrintTagWriterHTML.h | Call `prefillFromTag()` on load with OPT field map |
| src/TigerTagWriterHTML.h | Call `prefillFromTag()` on load with TT field map |
| src/OpenTag3DWriterHTML.h | Call `prefillFromTag()` on load with OT3D field map |

## Testing

1. Scan OpenPrintTag → open OpenPrintTag writer → fields pre-filled
2. Scan TigerTag → open TigerTag writer → fields pre-filled
3. Scan TigerTag → open OpenPrintTag writer → cross-format fields pre-filled, material mapped
4. Scan OpenTag3D → open TigerTag writer → cross-format fields pre-filled
5. No tag on reader → open any writer → fields blank, existing behavior unchanged
6. Pre-fill a field → manually edit it → field keeps user's value (autoFilled tracking)
7. Scan NFC+ tag (Spoolman data) → open any writer → material/color/weight from Spoolman
