# Vendored PS4 Torrent client

This directory contains a source snapshot of the PS4 client from upstream `sergiopoverony/ps4torrent`.

- Upstream repository: https://github.com/sergiopoverony/ps4torrent
- Pinned source commit: `cf8699046a9711e98cd640982aa18fc00fa88c09`
- License: GPL-3.0-or-later as stated by upstream; see [LICENSE](LICENSE).
- Snapshot purpose: provide the real controller-driven PS4 UI, HTTP client, daemon API adapter, JSON parser, and graphics implementation as the base for this project.

## Important

This is a vendored source snapshot, not yet a complete standalone build directory. The upstream PKG also requires its assets, OpenOrbis toolchain, generated system metadata, and the daemon ELF. Those dependencies must be restored or generated before building a PKG. The source must not be described as a working PS4 package until the build and on-console checks pass.

The application currently targets the existing daemon API at port 8787. The catalog UI and verified integration should be added without faking download status or breaking the original download-management screen.
