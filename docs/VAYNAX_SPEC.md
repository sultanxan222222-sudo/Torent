# VAYNAX TORRENT — product and implementation specification

## Product identity
- Product name: VAYNAX TORRENT.
- Platform: PlayStation 4 homebrew, with PS4 Pro/firmware 13.02/browser-launched HEN as the initial target.
- UI: Russian-first, DualShock 4 navigation.
- Visual identity: Chechen-flag-inspired crest and a full flag-shaped progress indicator. The entire flag starts grayscale; as real transfer progress advances, the same flag image is progressively revealed in its correct green, white, and red colors.
- The flag fill must be driven by measured bytes completed / total bytes, never by a decorative timer.

## Existing foundation — preserve and extend
- Vendored PS4 torrent client snapshot from sergiopoverony/ps4torrent, pinned to cf8699046a9711e98cd640982aa18fc00fa88c09, with GPL notices retained.
- Native PS4 client sources under vendor/ps4torrent/pkg/src.
- Catalog adapter, catalog-to-daemon integration, daemon API client, JSON catalog, validation scripts, CI, and OpenOrbis package workflow.
- Do not replace the vendored engine or rebuild from scratch. Verify existing code/build paths, then make small tested changes.

## Target console and HEN compatibility
- Initial hardware target: user's PS4 Pro, firmware 13.02.
- Jailbreak entry point: PS4 browser-based chain documented for 13.02; do not confuse the WebKit/kernel exploit with the HEN payload subsequently loaded.
- The exact payload build running on the user's console is not yet confirmed. Do not hard-code a guessed GoldHEN version.
- Before using BinLoader (9090), FTP, GoldHEN AutoRun configuration, internal package-install hooks, or any other payload-specific facility, detect/probe it and verify its response. Missing capabilities must disable only the dependent feature and produce a clear diagnostic.
- Never edit GoldHEN configuration or install an autorun payload merely because the firmware is 13.02. Preserve existing user configuration.
- See docs/PS4_13_02_HEN_COMPAT.md for source distinctions and test plan.

## Core torrent behavior
1. Accept local .torrent files and Magnet URIs.
2. Add and start a torrent only after daemon confirmation.
3. Show real queue state, progress, speed, ETA, peers where exposed, and errors.
4. Support pause/resume, remove, file selection where supported, internal HDD/USB destination, and restart persistence.
5. **Download-only operation. No seeding/upload to other peers and no torrent-creation workflow.**
6. Handle HTTPS, redirects, malformed metadata, large files, and timeouts explicitly.

## Browser-to-client handoff
Target flow: user downloads a .torrent file in the PS4 browser, selects/opens it, and VAYNAX TORRENT imports that exact file and starts the add flow. Investigate PS4 browser download locations, file-picker access, content-handler registration, and HEN permissions. Do not assume Windows-style file associations exist. Required fallback: in-app import picker for browser/download and USB locations, plus Magnet import. Claim automatic handoff only after real-console verification.

## Download storage and recovery
- On first launch, create a persistent internal-HDD `Download/` directory if it does not exist.
- Use `Download/Games`, `Download/Movies`, `Download/Series`, and `Download/Other` as default categories.
- Internal HDD is the default target; USB selection is optional and must not replace the default.
- Persist queue metadata and verified piece state. On restart, recheck saved pieces against torrent hashes before trusting them, then resume remaining pieces. Recover after application exit, crash, or console restart when the user next launches the app and HEN is active.
- Never claim background downloading after a full shutdown or before HEN is reactivated.

## PKG handling
- Recognize completed .pkg files and validate file size/integrity and available storage before offering installation.
- Use only a detected, compatible PS4/HEN package-install mechanism; verify installation success before reporting completion.
- After verified installation, offer deletion of the downloaded PKG as a separate confirmation. Never delete automatically.
- Direct installation from internal HDD is unverified until tested with the actual HEN payload on the target console.

## Online cinema — provider integrations
The Media Center must include an **Online Cinema** section alongside local HDD/USB playback. The user's requested provider/source names to investigate for compatibility are:
- LostFilm
- Zona
- HD Rezka
- Other compatible online video/catalog providers

Implement this as a modular provider/catalog connector system, not hard-coded scraping tied to one website. For each provider, first verify whether it offers a permitted public catalog/feed/API or otherwise allows the proposed integration. Respect provider terms, copyright, regional availability, authentication, and DRM. Do not bypass paywalls, DRM, access controls, or geo-restrictions; do not redistribute provider video or credentials. If a provider cannot be integrated lawfully/reliably, show it as unsupported and offer supported alternatives rather than pretending it works.

Online Cinema UX goals:
- Search and browse titles, seasons, episodes, and metadata from supported providers.
- Clearly show the selected provider and whether playback is available.
- Open the provider's official page or use an authorized playback interface where permitted.
- Continue watching / playback history only where the provider integration and playback backend allow it.
- Distinguish online-provider playback from local files and torrent transfers.
- Never represent a catalog listing as a playable stream until playback has been verified on PS4.

## Local media center
- Browse local video files from internal storage and USB.
- Organize films and series; remember playback position.
- Subtitle and audio-track selection where the playback backend supports it.
- Resume playback and show basic metadata.
- Do not claim support for formats/codecs until tested on PS4.

## Delivery and verification gates
- Keep GPL notices and upstream attribution intact.
- Never bundle or distribute copyrighted game/movie content. Handle only content the operator is authorized to access/distribute.
- Documentation/catalog CI is not a PS4 build.
- A release requires an actual PKG, recorded SHA-256, successful install/startup on PS4, and a real torrent transfer test.
- Track each feature as implemented, build-verified, console-verified, or unsupported.
