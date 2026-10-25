# Verified daemon API contract

Upstream source reviewed: [sergiopoverony/ps4torrent](https://github.com/sergiopoverony/ps4torrent), branch `main`, README and Russian user guide. The `/api/add` route and body handling have now been inspected in the pinned source. Runtime behavior still needs a PS4 build and console test.

## Connection

- Base URL: `http://<PS4-IP>:8787`
- `GET /status` — JSON status, torrent list, storage mounts, speed, daemon version.
- Optional `web_token`: if configured in daemon `config.txt`, API requests must include `&token=<token>`. Do not expose a configured token in logs or catalog feeds.

## Torrent operations

| Method / path | Purpose |
|---|---|
| `GET /api/pause?hash=<hash>` | Pause a torrent. Documentation allows an abbreviated hash of 8+ characters. |
| `GET /api/resume?hash=<hash>` | Resume a torrent. |
| `GET /api/pause_all` | Pause all unfinished torrents. |
| `GET /api/resume_all` | Resume all torrents. |
| `GET /api/delete?hash=<40-char-hash>` | Remove torrent from the list; downloaded files remain. |
| `GET /api/delete?hash=<40-char-hash>&files=1` | Remove and delete that torrent's downloaded files; full hash required. |
| `POST /api/add?name=<name>.torrent&drive=internal&start=1` | Add a torrent file as the POST body (maximum documented size 1 MiB). `drive=N` selects USB N; `internal` selects console storage. `start=0` adds paused. |
| `GET /api/config` | Read daemon configuration and version. |
| `GET /api/files?hash=<hash>` | Retrieve a torrent's file list; documented source requires a valid hexadecimal hash of at least 8 characters. |
| `GET /api/select?hash=<hash>&skip=<ranges>` | Set selected file ranges; verify exact route and skip semantics against the pinned source before exposing it in the UI. |
| `GET /api/set?... ` | Update supported configuration fields. |
| `GET /quit` | Stop the daemon; the UI should not expose this as an ordinary download action. |

Documented successful and error responses use JSON such as `{"ok":true}` and `{"ok":false,"error":"..."}`. The client must parse the actual HTTP status and JSON response rather than assume success.

## Integration requirements

1. Query `/status` before showing the service as connected.
2. Treat a timeout, non-2xx response, invalid JSON, or `ok:false` as an error state.
3. After pause/resume/delete, refresh `/status` and display the daemon's returned state.
4. For add, send the actual .torrent file bytes as the request body; `/api/add` validates the torrent and stores it for the daemon's normal scanner. A magnet URI is **not** accepted by this endpoint.
5. The current catalog client supports direct `http://` URLs returning a `.torrent` body, then POSTs those bytes to the local daemon. HTTPS, redirects, and magnet links are not supported in this first integration; the UI must report this clearly.
6. Do not allow catalog-provided URLs to change the daemon base URL or inject auth tokens. Catalog source URLs are only used for the remote fetch.

## Source reference

- User guide: `docs/README_ru.md`
- Upstream README: `README.md`
- Source repository: https://github.com/sergiopoverony/ps4torrent

The source route has been reviewed, but the new catalog-to-daemon path has not yet been compiled with the PS4 toolchain or tested on console.
