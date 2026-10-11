# Download and play Smooth Zoom

Download a tested build from [the latest fork release](https://github.com/Deku-shrub/OpenTTD/releases/latest). Official OpenTTD downloads do not contain this fork's camera features.

| System | Download | Installation |
| --- | --- | --- |
| Windows 64-bit | [Portable ZIP](https://github.com/Deku-shrub/OpenTTD/releases/latest/download/smooth-zoom-windows-x64.zip) | Extract the entire archive into a new folder and run `openttd.exe`. |
| Windows 32-bit | [Portable ZIP](https://github.com/Deku-shrub/OpenTTD/releases/latest/download/smooth-zoom-windows-x86.zip) | Extract the entire archive and run `openttd.exe`. |
| Ubuntu 24.04 x64 | [Debian package](https://github.com/Deku-shrub/OpenTTD/releases/latest/download/smooth-zoom-linux-x64.deb) | Run `sudo apt install ./smooth-zoom-linux-x64.deb`, then start OpenTTD. This package can replace a system OpenTTD installation; back up that package and your configuration first. |
| macOS, Apple Silicon | [Disk image](https://github.com/Deku-shrub/OpenTTD/releases/latest/download/smooth-zoom-macos-arm64.dmg) | Open the disk image and copy OpenTTD into Applications. These builds are unsigned; if macOS blocks the app, use the approval option in System Settings → Privacy & Security after attempting to open it. |

Keep the executable, `lang` directory and `baseset` directory together in portable installations. OpenGFX 8.0 graphics are included. You can install optional OpenSFX sound, OpenMSX music and NewGRF content through the game's content downloader.

Smooth zoom and fly navigation are enabled for new configurations. Existing choices are preserved. Open **Game Options → Advanced**, select the Expert/all-settings filter and search for **smooth** or **fly** to enable or adjust them. The main fly switch controls all destinations; individual destination switches provide overrides. Scroll or zoom during a flight to take control immediately.

The toolbar zoom percentage shows your scale. Click it to stop and snap to a traditional zoom level. See the [user guide](smooth_zoom_readme.md) and [controls](smooth_zoom_controls.md) for examples and pacing options.

These are experimental development builds. Releases identify the exact source revision, include SHA-256 checksums and link to their installation instructions. Publication waits for every platform build and test job to pass. A failed build leaves the previous download available.

Existing saves require no camera-specific conversion, but the underlying OpenTTD version still determines save compatibility. Back up saves before using a development engine; an older engine may not load saves written by a newer version. Read the [compatibility notes](smooth_zoom_readme.md#existing-saves-rollback-and-mod-compatibility).

For multiplayer, all participants should use packages from the same fork release. The initial `fb5123084a` release has a Linux revision-stamp mismatch with Windows/macOS; use a subsequent corrected release for cross-platform multiplayer. CI now checks clean, matching engine identities before publication.

To compile a matching multiplayer client, use a clean Git checkout of the release's `BUILD.txt` source revision in a repository cloned with `git clone --no-tags`. Release tags are created after the binaries are built; rebuilding a tagged checkout can change OpenTTD's network identity. GitHub's automatic source ZIPs omit Git revision metadata. Matching source and revision identity matter as well as matching content.
