# PS4 Torrent Catalog

A controller-first catalog and download manager for PS4 homebrew environments. The project is designed to integrate with the existing `ps4torrent` daemon instead of reimplementing its torrent engine.

## Project status

**Integration stage — not a console-ready PKG yet.** The pinned upstream PS4 daemon and controller-client source, license, and runtime assets have been vendored under `vendor/ps4torrent/`. The catalog is still a separate module; source integration is not the same as a successful package build. A successful PS4 package build and on-console test are required before any release is described as working.

## Intended features

- Russian-first interface designed for DualShock 4 navigation.
- Searchable catalog with cover art, categories, descriptions, and source links.
- Configurable JSON catalog feeds; no hard-coded dependency on one provider.
- Download queue with progress, speed, pause/resume, and error states through a compatible daemon API.
- Persistent settings and last-used daemon address.
- Clear separation between catalog metadata and torrent transport.

## Architecture

1. **PS4 UI / PKG** — controller navigation, catalog cards, search, download queue.
2. **Catalog adapter** — validates and normalizes JSON feed entries.
3. **Daemon client** — talks to the existing `ps4torrent` service; API paths must be verified against the exact daemon revision before integration.
4. **Build & validation** — reproducible OpenOrbis toolchain setup, static checks, package artifact, and explicit build status.

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md), [docs/ROADMAP.md](docs/ROADMAP.md), and [catalog/catalog.example.json](catalog/catalog.example.json).

## Build status

No PKG is attached to this initial commit. The OpenOrbis PS4 toolchain is required for a real package build; compiling a desktop mock-up is not a substitute. The CI workflow checks repository structure and catalog JSON validity, and will not claim that a PS4 PKG was produced unless the required toolchain is present.

## Sources and licensing

The torrent engine is intended to be integrated from the upstream [ps4torrent project](https://github.com/sergiopoverony/ps4torrent) after its source revision, API, and license are reviewed. Upstream code and assets must retain their original license notices. Catalog feeds should point to content the operator is authorized to distribute.
