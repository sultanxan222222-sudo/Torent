# Verified daemon API contract

Upstream source reviewed: [sergiopoverony/ps4torrent](https://github.com/sergiopoverony/ps4torrent), branch `main`, README and Russian user guide. This is a preliminary contract from documentation; response examples and edge cases still need source-level verification and runtime testing.

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
| `GET /api/set?... ` | Update supported configuration fields. |
| `GET /quit` | Stop the daemon; the UI should not expose this as an ordinary download action. |

Documented successful and error responses use JSON such as `{"ok":true}` and `{"ok":false,"error":"..."}`. The client must parse the actual HTTP status and JSON response rather than assume success.

## Integration requirements

1. Query `/status` before showing the service as connected.
2. Treat a timeout, non-2xx response, invalid JSON, or `ok:false` as an error state.
3. After pause/resume/delete, refresh `/status` and display the daemon's returned state.
4. For add, send the actual .torrent file bytes as the request body; a magnet URI is **not** accepted by the documented `/api/add` endpoint.
5. Do not implement magnet add until the pinned daemon source explicitly confirms a supported endpoint.
6. Do not allow catalog-provided URLs to change the daemon base URL or inject auth tokens.

## Source reference

- User guide: `docs/README_ru.md`
- Upstream README: `README.md`
- Source repository: https://github.com/sergiopoverony/ps4torrent

This is documentation-based, not yet verified by a live PS4 daemon. Before release, pin an upstream commit and test each operation against that exact build.
