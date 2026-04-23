# SpoolSense Usermods

Community-contributed modifications for the SpoolSense scanner. Each subdirectory is maintained by its author.

## Available Mods

| Mod | Author | Description |
|-----|--------|-------------|
| [jims_enclosure](jims_enclosure/) | Jim | 3-piece scanner enclosure for ESP32-S3-Zero + PN5180 |
| [linuxgangster](linuxgangster/) | LinuxGangster | Standalone case (S3-Zero + PN532) + BoxTurtle PN532 tray |
| [plasticsnake](plasticsnake/) | plasticsnake | 3-part standalone enclosure for ESP32-S3-Zero + PN5180 + 16x2 I2C LCD |
| [roomonthethird](roomonthethird/) | roomonthethird | SpoolSense NFC reader case — two-piece base + lid (STEP/STL/F3D) |

## Contributing

Want to add your own mod?

1. Fork this repo and create a branch
2. Create a directory under `usermods/` with a descriptive name (e.g., `usermods/my_mount/`)
3. Include:
   - Your STL/3MF/STEP files
   - A `README.md` with description, print settings, hardware needed, and photos if possible
4. Open a PR targeting `dev`

Each contributor is responsible for maintaining their mod. If a mod becomes incompatible with the current scanner hardware, it will be marked as deprecated.
