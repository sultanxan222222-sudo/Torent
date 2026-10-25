# Roadmap

## Stage 0 — Repository foundation
- [x] Create project structure and document status honestly.
- [x] Define a sample catalog schema.
- [x] Add JSON/schema validation workflow.
- [x] Identify the upstream daemon and review its documented API and GPL-3.0 license.
- [x] Record documented status, pause/resume, add-torrent, delete, and config routes.
- [ ] Pin upstream daemon source revision and inspect implementation of every API route.

## Stage 1 — Integration proof
- [ ] Build a small daemon API probe for the supported PS4 environment.
- [ ] Verify status/list/add/pause/resume/remove endpoints against the pinned daemon.
- [ ] Define timeout, retry, and offline behavior.
- [ ] Add tests for catalog parsing and API response normalization.

## Stage 2 — PS4 application
- [ ] Bring the upstream PS4 PKG client source into this repository while preserving GPL notices.
- [ ] Create controller-first catalog and downloads screens.
- [ ] Add persistent daemon URL and UI preferences.
- [ ] Integrate the verified daemon adapter.
- [ ] Build PKG with the OpenOrbis toolchain.

## Stage 3 — Validation
- [x] CI validates the sample catalog JSON.
- [ ] CI verifies source and catalog checks.
- [ ] Produce PKG only in a job with the PS4 toolchain available.
- [ ] Install and test on a real PS4 with the target homebrew setup.
- [ ] Publish a release only after artifact and smoke-test results are recorded.

## Release gate

A release is not complete until the PKG exists, its SHA-256 is recorded, and installation/startup plus a real daemon interaction have been tested. CI success on documentation or JSON checks alone is not a release.
