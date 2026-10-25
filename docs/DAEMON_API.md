# Daemon API verification

This document is deliberately a checklist until the exact upstream revision has been inspected. Do not invent endpoint names or payloads.

For the pinned `sergiopoverony/ps4torrent` revision, verify from source:
- daemon startup options and bind address;
- status/health endpoint;
- torrent list response shape;
- add-torrent endpoint and supported input types (`.torrent`, magnet URI, or both);
- pause/resume/remove semantics;
- per-torrent progress, download speed, total size, and error reporting;
- authentication or origin restrictions, if any;
- default download directory and how it is configured.

Record each verified method, path, request, and response example here, with a source-file path and commit SHA. Until this checklist is complete, the UI must show daemon controls as unavailable rather than simulating success.
