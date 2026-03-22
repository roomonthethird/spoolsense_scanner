# Contributing to SpoolSense Scanner

Thanks for your interest in contributing! Here's how to get started.

## Reporting Bugs

Open an issue using the Bug Report template. Include:
- Exact steps to reproduce
- Serial logs (connect via USB, 115200 baud)
- Hardware details and firmware version

## Suggesting Features

Open an issue using the Feature Request template. Explain the use case and why it matters for your setup.

## Submitting Pull Requests

1. Fork the repo and create a branch from `dev`
2. Make your changes
3. Test on hardware — compile alone isn't enough for NFC-related changes
4. Ensure both targets compile: `pio run -e esp32dev` and `pio run -e esp32s3zero`
5. Open a PR targeting the `dev` branch (not `main`)

### Branch workflow

- `dev` — active development, all PRs target here
- `main` — production releases only, merged from dev when stable

### Code guidelines

- All user-facing config must live in `include/UserConfig.h`
- Avoid unnecessary heap allocations — runtime memory is limited
- Tag format parsers belong in `lib/` (e.g. `lib/opentag3d/`, `lib/tigertag/`)
- Consider thread safety — multiple FreeRTOS tasks share state

## Questions?

Open an issue using the Question template or start a discussion.
