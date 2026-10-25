# Architecture and integration boundaries

## Components

### Catalog adapter

A catalog is a JSON document with a `version` and an `items` array. Each item has a stable `id`, a display `title`, optional cover/description/category/platform metadata, and a source object. The adapter must reject malformed entries and tolerate optional fields being absent.

### Download daemon adapter

The app must not pretend a torrent is downloading merely because the user selected it. It should:
- validate the configured daemon base URL;
- query daemon status and supported endpoints before enabling download actions;
- send an add request only when the endpoint and payload format are confirmed for the pinned daemon revision;
- poll real task state and display daemon errors;
- preserve the last known state across UI restarts where the API permits it.

Do not assume endpoints from a different daemon fork. Pin the upstream revision and record verified routes in `docs/DAEMON_API.md`.

### UI

The first usable screen should show catalog, search, categories, and a separate downloads view. Every interactive control must work with the DualShock 4. Loading, empty, offline, and error states are first-class UI states, not hidden failures.

## Safety and reliability

- Do not execute commands or scripts received from a catalog feed.
- Treat feed text, image URLs, magnet URIs, and torrent metadata as untrusted input.
- Restrict downloads to an explicitly selected destination.
- Never report a successful download/install unless the daemon or filesystem confirms it.
- Do not bundle copyrighted game files or scrape/host unauthorized copies. Feeds are configurable and intended for content the operator is authorized to distribute.

## Current integration status

This initial commit defines the contract only. The upstream daemon and client must be inspected and pinned before writing API calls or claiming that queue controls work.
