# SpoolSense Installer — Engineering Plan

## Overview

A unified installer CLI (`spoolsense-installer` repo) that covers both the scanner firmware and the SpoolSense middleware. One command, works from a Raspberry Pi (recommended) or a laptop.

**Recommended approach:** Run from the printer host (Pi) with ESP32 connected via USB. Installs everything in one pass. If the printer host has no free USB port, flash the scanner from a laptop ("Scanner only"), then run the installer again on the Pi ("Middleware only").

**Note:** SpoolSense middleware must run on the printer host.

## Architecture

### Key Decision: NVS-based config

The scanner firmware currently uses compile-time `#define` in `UserConfig.h`. The installer approach uses **NVS (Non-Volatile Storage)** instead:

- One generic firmware binary per board type (same for all users)
- Per-user config (WiFi, MQTT, Spoolman) stored in a separate NVS flash partition
- `esptool` flashes both the firmware binary and the NVS config binary
- No PlatformIO needed on the user's machine — just `pip install esptool`
- `UserConfig.h` still works as a fallback for developers compiling from source

```
Generic firmware binary (same for everyone)
    +
NVS partition binary (per-user config: WiFi, MQTT, etc.)
    ↓
esptool flashes BOTH to different flash addresses
    ↓
Firmware reads config from NVS at boot, falls back to compile-time defaults
```

### NVS Contract

A shared `nvs_keys.csv` file in the installer repo defines every NVS key, type, and default. Firmware code references the same key names. Single source of truth prevents drift.

### Repo Structure

```
spoolsense-installer/
├── install.sh              ← curl target, installs pip deps + runs install.py
├── install.py              ← interactive CLI (click + jinja2 via shell wrapper)
├── nvs_keys.csv            ← shared NVS key contract
├── lib/
│   └── nvs_partition_gen.py  ← bundled from ESP-IDF (Apache 2.0)
├── firmware/
│   └── partitions.csv      ← partition table (bundled for flashing)
├── templates/
│   ├── config.single.yaml
│   ├── config.toolchanger.yaml
│   └── config.afc.yaml
├── README.md
└── tests/
    └── test_install.py
```

### Firmware Changes (spoolsense_scanner)

1. **`ConfigurationManager.cpp`** — add NVS read path (~30 lines). Per-key fallback: each key reads from NVS first, falls back to compile-time default individually.
2. **`platformio.ini`** — verify partition table has NVS partition
3. **`.github/workflows/release.yml`** — GitHub Actions workflow to build and publish release binaries on tag push

## Data Flow

```
install.sh (bash wrapper)
    │
    ├─ pip install esptool (+ click, jinja2 for CLI)
    │
    └─ python3 install.py
          │
          ▼
    ┌─────────────────┐
    │ Collect answers  │
    │ (interactive)    │
    └────────┬────────┘
             │
     ┌───────┴────────┐
     ▼                ▼
[Scanner?]      [Middleware?]
     │                │
     ▼                ▼
Generate NVS     Generate config.yaml
CSV from         from template +
answers          answers
     │                │
     ▼                ▼
nvs_partition    git clone SpoolSense
_gen.py →        pip install -r
config.bin       requirements.txt
     │           create systemd service
     ▼                │
Detect USB port       │
     │                │
     ▼                │
esptool.py flash_id   │
  → verify flash      │
    size >= 4MB        │
  → verify chip       │
    matches board      │
    selection          │
     │                │
     ▼                │
esptool.py            │
write_flash:          │
  partitions.bin      │
  config.bin          │
  firmware.bin        │
     │                │
     └───────┬────────┘
             ▼
    ┌─────────────────┐
    │ Validate:       │
    │ • Format checks │
    └─────────────────┘
```

## Decisions Made

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Dependencies | Shell wrapper + Python libs (click, jinja2) | Best of both — zero friction entry, nice UX |
| Firmware source | Pre-built binary from GitHub Releases | No PlatformIO needed, just esptool |
| NVS contract | Shared CSV schema (`nvs_keys.csv`) | Single source of truth, prevents key drift |
| Release workflow | GitHub Actions on tag push | Automated, required for binary distribution |
| Partition table | Include in release artifacts | Installer reads flash offset from it |
| Flash recovery | Good error messages + BOOT/RESET instructions | Standard esptool recovery |
| Flash size verification | `esptool.py flash_id` before flashing | Verify flash >= 4MB and chip type matches board selection; refuse to flash on mismatch |
| Supported boards | WROOM DevKit 4MB + S3-Zero 4MB only | "Other" option tells user to compile from source via PlatformIO; only publish binaries for tested boards |
| Input validation | Format only, skip connectivity checks | User preference — keep install fast |
| Re-run behavior | Fresh install each time, warn before overwrite | Simpler code, user preference |
| Password handling | Mask input (getpass), plaintext in config files | Matches existing middleware pattern |
| Partial NVS | Per-key fallback to compile-time defaults | Most resilient across firmware versions |

## Failure Modes

| Codepath | Failure | Handled? | User sees |
|----------|---------|----------|-----------|
| GitHub API fetch | Network timeout | Error msg + retry | Clear error |
| USB detection | No device found | Error msg + instructions | Clear error |
| Flash size check | Flash < 4MB or chip mismatch | Refuse to flash, tell user to compile from source | Clear error |
| Flash mid-write | USB disconnected | esptool error + BOOT/RESET guide | Clear error |
| NVS gen | Invalid CSV | nvs_partition_gen error | Clear error |
| Middleware clone | git not installed | Error msg + install instruction | Clear error |
| NVS read (firmware) | Corrupted NVS | Fall back to compile-time | Silent fallback + log |
| NVS read (firmware) | Partial keys | Per-key fallback | Silent fallback + log |

## NOT in Scope

- Web-based first-boot config (AP mode + captive portal) — future firmware feature
- Auto-update mechanism — installer is run manually
- Connectivity validation during install — format checks only
- Idempotent re-runs — fresh install with overwrite warning

## Test Plan

| Codepath | Test type | Priority |
|----------|-----------|----------|
| NVS CSV generation from answers | Unit | High |
| YAML config generation from answers | Unit | High |
| Input validation (format checks) | Unit | High |
| NVS read in firmware (all keys present) | Native test | High |
| NVS read in firmware (empty, fallback) | Native test | High |
| NVS partial population handling | Native test | Medium |
| GitHub Release API parsing | Unit | Medium |
| USB port detection | Integration | Low |
