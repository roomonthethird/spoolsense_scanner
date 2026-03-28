# OpenPrintTag Write Analysis

Date: 2026-03-25
Branch: fix/openprinttag-temp-write

## Root Cause Found

The `/api/write-tag` handler unconditionally enqueued writes for ALL fields (material_type, color, weight, manufacturer, consumed_weight) even when only specific fields were provided in the JSON payload. Default values (0, empty string, black) overwrote existing tag data.

**Fix applied:** Only enqueue writes for fields explicitly present in the JSON payload using `doc.containsKey()` checks.

## Two Remaining Issues

### Issue 1: NFC Connection Drops After ~5 Sequential Writes

The PN5180 loses NFC connection to the SLIX2 tag after approximately 5-6 sequential `writeSingleBlock` calls. This happens on both ESP32-WROOM and ESP32-S3-Zero — confirmed it's firmware/library, not hardware.

The write queue processes one field at a time. Each field write:
1. Preflight: copy tag data, re-encode CBOR with updated field, check region overflow
2. Write: call `opt_write_dirty_pages()` which writes changed NFC blocks via `writeSingleBlock()`
3. If write fails: fallback to `opt_write_to_nfc()` (full write)
4. Verify: re-read and parse

After ~5 fields, step 2 fails with "No card detected" on `writeSingleBlock()`.

**Theories:**
- RF field power degradation during sustained writes
- PN5180 internal state not properly reset between write operations
- SLIX2 tag entering a protection/cooldown state after rapid writes
- SPI timing issue accumulating over multiple transactions

**Research areas:**
- Does the PN5180 need an RF field reset between writes? (`resetRF()` or `switchToSend()`?)
- Does SLIX2 have a write cooldown spec? Check ICODE SLIX2 datasheet timing requirements
- Does `opt_write_dirty_pages()` only write changed blocks, or does it write all? If dirty tracking is wrong, it's writing way more blocks than needed
- Try adding a small delay (10-50ms) between each `writeSingleBlock()` call in `opt_write_dirty_pages()`
- Try `opt_write_to_nfc()` (full write) instead of dirty pages — does a single bulk write succeed where sequential block writes fail?
- hyutrn fork of PN5180 library has reduced blocking delays and FreeRTOS awareness — might help

### Issue 2: Scan Loop Race Condition (Separate from NFC drops)

When multiple `/api/write-tag` calls are made sequentially, the scanner's continuous scan loop (`scanLoop()` in NFCScanTask) can re-read the tag between API calls. This re-read overwrites `currentSpool.tag_data` with the NFC state from AFTER the first batch but BEFORE it's fully committed. The second batch then uses stale in-memory data as its starting point, losing the first batch's writes.

**Evidence:** Test B (pre-fix) — basic fields wrote, then temps wiped them. The second API call's preflight snapshot (`writeScratchTag_ = currentSpool.tag_data`) got a stale copy.

**Potential fixes:**
- Add a `write_in_progress` flag that suppresses the scan loop's tag re-read while the write queue is non-empty
- After write queue drains, force a fresh NFC read before resuming scan loop
- Batch all fields into a single CBOR encode + single NFC write (eliminates the queue entirely)

## Test Results

### Before Fix (unconditional writes)

| Test | Strategy | Score |
|------|----------|-------|
| All 9 fields, one call | Single call | 5/9 |
| Two batches, 3s gap | Basic + temps | 4/9 |
| Temps only (4 fields) | Single call | 0/4 |
| Basic only (5 fields) | Single call | 5/5 |
| Single min_print_temp | Single call | 0/1 |
| Single min_bed_temp | Single call | 0/1 |

### After Fix (conditional writes)

| Test | Strategy | Score |
|------|----------|-------|
| Temps only (4 fields) | Single call | 3/4 |
| All 9 fields, one call | Single call | 5/9 |
| Two batches, 8s gap | Basic + temps | 8/9 |
| Two batches, 12s gap | Basic + temps | **9/9** |

## Key Findings

1. **Single temp write to freshly formatted tag: 0/1 → 3/4** — the fix eliminated the root cause (default overwrites)
2. **Two batches with 12s gap: 9/9** — gives the PN5180 time to recover between batch writes
3. **Basic fields (5) always succeed** — the connection holds for 5 sequential writes
4. **Temps fail at position 6+** — NFC connection drops after ~5 writes regardless of field type
5. **Both ESP32-WROOM and S3-Zero show identical behavior** — firmware, not hardware

## Optimal Write Strategy (Current Workaround)

**Writer page should split into two API calls:**
1. Format (if needed)
2. Batch 1: material_type, color, manufacturer, initial_weight_g, remaining_g
3. Wait 10-12 seconds
4. Batch 2: min_print_temp, max_print_temp, min_bed_temp, max_bed_temp

Total time: ~25 seconds. Not ideal but reliable.

## Ideal Fix (Future)

A single atomic write that encodes ALL fields into CBOR once and writes to NFC in one pass. This eliminates:
- The sequential write queue (no NFC drops)
- The scan loop race condition (no intermediate states)
- The 12-second gap (single operation)

This would require a new write path: accept all fields → build complete CBOR map → write full tag in one `opt_write_to_nfc()` call. The existing `formatNewSpool()` already does this pattern (builds tag then writes once).

## Files Changed

- `src/WebServerManager.cpp` — conditional field enqueuing in `handleApiWriteTag()`
- `lib/openprinttag/openprinttag_lib.c` — debug printf for region overflow (key, new_offset, region_size)

## Related Issues

- Scanner #17: Sequential write API calls race with scan loop
- OpenPrintTag CBOR library: region overflow detection
