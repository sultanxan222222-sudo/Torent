# VAYNAX TORRENT

A Russian-first, DualShock 4-controlled torrent client and media center for PlayStation 4 homebrew, extending the existing PS4 torrent client foundation in this repository.

**Status: active integration work — not yet a verified console-ready release.**

## Product goals
- Real torrent downloads and seeding, with .torrent and Magnet import.
- Desktop-like browser handoff, subject to PS4 browser/HEN capabilities; in-app file picker is the required fallback.
- Chechen-flag-shaped progress indicator: the complete flag starts grayscale and progressively becomes its true green, white, and red colors according to actual downloaded bytes.
- Download queue, pause/resume, peers, speed, ETA, destination selection, and persistent state where supported.
- Local media library for films and series on HDD/USB.
- **Online Cinema** provider section: investigate LostFilm, Zona, HD Rezka, and other compatible providers through permitted catalog/API or official-page integrations. Provider support and playback must be verified; no DRM, paywall, access-control, or geo-restriction bypass.
- Russian-first interface and DualShock 4 navigation.

## Existing technical foundation
The project extends the vendored PS4 torrent client from [sergiopoverony/ps4torrent](https://github.com/sergiopoverony/ps4torrent), pinned at `cf8699046a9711e98cd640982aa18fc00fa88c09`. Original license notices are retained under `vendor/ps4torrent/`.

See [product specification](docs/VAYNAX_SPEC.md), [architecture](docs/ARCHITECTURE.md), [daemon API contract](docs/DAEMON_API.md), and [roadmap](docs/ROADMAP.md).

## Build and release honesty
No release is complete until a real PS4 PKG is produced, its SHA-256 is recorded, and installation/startup plus an actual torrent transfer are tested on a console. Browser handoff, online provider integrations, and media codec support also require real-device verification. The application does not bundle game or movie content.
