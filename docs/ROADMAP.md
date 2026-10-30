# Roadmap

## Stage 0 — Repository foundation
- [x] Create project structure and document status honestly.
- [x] Define a sample catalog schema.
- [x] Add JSON/schema validation workflow.
- [x] Identify the upstream daemon and review its documented API and GPL-3.0 license.
- [x] Record documented status, pause/resume, add-torrent, delete, and config routes.
- [x] Pin upstream client source snapshot to `cf8699046a9711e98cd640982aa18fc00fa88c09` and preserve its GPL license notice.
- [x] Audit current source for the existing Magnet resolver, resume-state files, integrity checks, and current HEN-specific startup assumptions.
- [ ] Inspect implementation of every daemon API route against the pinned source.

## Stage 1 — Target HEN compatibility and integration proof
- [x] Research public PS4 13.02 browser/kernel exploit documentation and distinguish it from the HEN payload.
- [ ] Identify the actual payload/capability set loaded on the user's console without asking the user to change or update anything.
- [ ] Build a safe runtime capability probe for BinLoader, persistent autorun support, storage paths, and package-install interfaces.
- [ ] Build a small daemon API probe for the supported PS4 environment.
- [ ] Verify status/list/add/pause/resume/remove endpoints against the pinned daemon.
- [ ] Define timeout, retry, and offline behavior.

## Stage 2 — Download engine and recovery
- [x] Confirm existing source contains Magnet metadata resolution and persistent piece-state/resume code.
- [ ] Verify recovery after app exit, crash, and console restart, including rechecking completed pieces.
- [ ] Change default destination to internal HDD `Download/` and create category folders on first launch.
- [ ] Enforce download-only behavior: disable peer upload/seeding and remove torrent-creation UI/API.
- [ ] Add tests for Magnet parsing, resume metadata, and integrity verification.

## Stage 3 — PS4 application
- [x] Vendor the upstream PS4 PKG client source and required runtime assets while preserving license notices.
- [ ] Create controller-first catalog and downloads screens.
- [ ] Add persistent daemon URL and UI preferences.
- [ ] Integrate the verified daemon adapter.
- [ ] Build PKG with the OpenOrbis toolchain.

## Stage 4 — Media center
- [ ] Add local HDD/USB media browsing and playback-position persistence.
- [ ] Verify subtitle/audio-track controls and codec support on PS4.
- [ ] Investigate permitted integrations for LostFilm, Zona, HD Rezka, and other compatible providers.
- [ ] Mark each provider separately as supported only after real PS4 playback verification.

## Stage 5 — Validation and release
- [x] CI validates the sample catalog JSON.
- [ ] CI verifies source and catalog checks.
- [x] Add `scripts/build_ps4_pkg.sh` to build the real PKG when both required SDKs are installed.
- [ ] Produce PKG in an environment with the PS4 toolchains available.
- [ ] Install and test on a real PS4 Pro running the user's actual firmware/HEN.
- [ ] Test browser .torrent handoff, internal HDD download, interruption/restart recovery, and a real torrent transfer.
- [ ] Publish a release only after artifact and smoke-test results are recorded.

## Release gate

A release is not complete until the PKG exists, its SHA-256 is recorded, and installation/startup plus a real download have been tested. CI success on documentation or JSON checks alone is not a release.
