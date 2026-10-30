# VAYNAX TORRENT — product and implementation specification

## Product identity
- Product name: VAYNAX TORRENT.
- Platform: PlayStation 4 homebrew, with PS4 Pro/HEN as the initial target.
- UI: Russian-first, DualShock 4 navigation.
- Visual identity: Chechen-flag-inspired crest and a full flag-shaped progress indicator. The entire flag starts grayscale; as real transfer progress advances, the same flag image is progressively revealed in its correct green, white, and red colors. It is not a separate colored bar placed over a gray flag.
- The flag fill must be driven by measured bytes completed / total bytes, never by a decorative timer.

## Existing foundation — preserve and extend
The repository already contains:
- A vendored PS4 torrent client snapshot from sergiopoverony/ps4torrent, pinned to cf8699046a9711e98cd640982aa18fc00fa88c09, with its GPL license notice.
- Native PS4 client sources under vendor/ps4torrent/pkg/src.
- A catalog adapter and catalog-to-daemon integration code.
- Daemon status, pause/resume, delete, file-list, and add/magnet client API work.
- JSON catalog, validation scripts, CI, and an OpenOrbis package workflow.

Do not replace the vendored engine or rebuild the application from scratch. First verify existing code and build paths, then make small tested changes.

## Core torrent behavior
1. Accept local .torrent files and Magnet URIs.
2. Add a torrent and start it only after the daemon confirms the request.
3. Show real queue state, progress, speed, ETA, peers, seeds when exposed by the daemon, and errors.
4. Support pause/resume, remove, file selection where supported, destination selection (internal HDD / USB), and resume after restart.
5. Support seeding and creation of .torrent files if the pinned engine exposes a safe, verifiable API; otherwise record the missing engine capability before implementation.
6. HTTPS, redirects, malformed metadata, large files, and daemon timeouts must have explicit safe handling.

## Browser-to-client handoff — desktop-like flow
Target user flow:
1. User selects a legitimate torrent download on a website in the PS4 browser.
2. The browser downloads a small .torrent file.
3. User selects/opens that downloaded file.
4. VAYNAX TORRENT receives/imports the exact file and opens the add-torrent flow, preferably starting immediately after destination/confirmation settings are satisfied.

Implementation must investigate PS4 browser download locations, file-picker access, URI/content-handler registration available to homebrew, and HEN permissions. Do not assume PS4 supports Windows-style file associations. Provide a reliable fallback: an in-app “Import .torrent” picker for browser/download and USB locations, plus Magnet paste/import. Never claim automatic handoff until verified on a real console.

## Media center
- Browse local video files from internal storage and USB.
- Organize films and series; remember playback position.
- Subtitle and audio-track selection where the playback backend supports it.
- Resume playback and show basic media metadata.
- Do not claim support for formats/codecs until tested on PS4.

## Delivery and verification gates
- Keep GPL notices and upstream attribution intact.
- Never bundle or distribute copyrighted game/movie content. The app handles user-selected files and lawful torrents.
- A successful CI documentation/catalog check is not a PS4 build.
- A release requires an actual PKG, recorded SHA-256, successful install/startup on PS4, and a real torrent transfer test.
- Keep a clear status for every feature: implemented, build-verified, console-verified, or not yet supported.
