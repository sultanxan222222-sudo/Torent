# VAYNAX TORRENT

A Russian-first, DualShock 4-controlled torrent client and media center for PlayStation 4 homebrew, built by extending the existing PS4 torrent client foundation in this repository.

**Status: active integration work — not yet a verified console-ready release.** The repository contains a vendored PS4 client source snapshot, catalog and daemon integration code, JSON validation, and an OpenOrbis package workflow. A successful documentation or catalog CI check must not be confused with a built or console-tested PKG.

## Product goals

- Real torrent downloads and seeding, with .torrent and Magnet import.
- Desktop-like browser handoff: select a downloaded .torrent file and open it in VAYNAX TORRENT. This depends on what the PS4 browser/HEN environment permits; an in-app file picker is the required fallback.
- Full-flag progress indicator: the Chechen-flag-shaped image starts entirely grayscale and progressively becomes its true green, white, and red colors according to actual downloaded bytes.
- Download queue, pause/resume, peers, speed, ETA, destination selection, and persistent state where supported by the daemon.
- Media center for user-owned/local films and series on HDD/USB, with playback resume and subtitle/audio selection where supported.
- Russian-first interface and DualShock 4 navigation.

## Existing technical foundation

The project extends the vendored PS4 torrent client from [sergiopoverony/ps4torrent](https://github.com/sergiopoverony/ps4torrent), pinned at `cf8699046a9711e98cd640982aa18fc00fa88c09`. Original license notices are retained under `vendor/ps4torrent/`.

See:
- [Product and implementation specification](docs/VAYNAX_SPEC.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Daemon API contract](docs/DAEMON_API.md)
- [Roadmap](docs/ROADMAP.md)

## Build and release honesty

No release is complete until a real PS4 PKG is produced, its SHA-256 is recorded, and installation/startup plus an actual torrent transfer are tested on a console with the intended homebrew environment. Browser handoff and media codec support also require real-device verification.

Catalog feeds and torrent sources must be used only for content the operator is authorized to distribute. The application does not bundle game or movie content.
