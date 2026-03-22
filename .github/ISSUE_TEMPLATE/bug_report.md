---
name: Bug Report
about: Report a bug or unexpected behavior
title: "[Bug] "
labels: bug
assignees: ''
---

## Steps to Reproduce
Exact steps to trigger the bug. Be specific — NFC tag behavior can vary by placement, timing, and tag type.

1. 
2. 
3. 

## Expected Behavior
What should happen.

## Actual Behavior
What actually happens.

## Tag Details
- **Tag type:** (e.g. NTAG215, SLIX2, MIFARE Classic, unknown)
- **Tag format:** (e.g. OpenPrintTag, TigerTag, OpenTag3D, blank)
- **Tag placement:** (e.g. directly on antenna, ~1 inch away)

## Hardware
- **Board:** (e.g. ESP32-S3 Zero, ESP32 DevKit)
- **NFC Reader:** (e.g. AITRIP PN5180)
- **Firmware Version:** (check at spoolsense.local)

## Serial Logs
Connect to the scanner via USB and open a serial monitor at 115200 baud.
In PlatformIO: `pio device monitor -b 115200`
In Arduino IDE: Tools → Serial Monitor → set baud to 115200

```
Paste relevant serial output here
```

## Additional Context
Any other details, screenshots, or web UI behavior.
