#!/usr/bin/env python3
"""Validate catalog JSON shape without fetching or executing remote content."""
import json
import sys
from pathlib import Path

REQUIRED_TOP = {"version", "items"}
REQUIRED_ITEM = {"id", "title", "source"}


def main() -> int:
    path = Path(sys.argv[1] if len(sys.argv) > 1 else "catalog/catalog.example.json")
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        print(f"ERROR: cannot read valid JSON from {path}: {exc}", file=sys.stderr)
        return 1

    if not isinstance(data, dict) or not REQUIRED_TOP.issubset(data):
        print(f"ERROR: top-level object must include {sorted(REQUIRED_TOP)}", file=sys.stderr)
        return 1
    if data["version"] != 1 or not isinstance(data["items"], list):
        print("ERROR: version must be 1 and items must be an array", file=sys.stderr)
        return 1

    seen = set()
    for index, item in enumerate(data["items"]):
        if not isinstance(item, dict) or not REQUIRED_ITEM.issubset(item):
            print(f"ERROR: item {index} must include {sorted(REQUIRED_ITEM)}", file=sys.stderr)
            return 1
        if not isinstance(item["id"], str) or not item["id"].strip():
            print(f"ERROR: item {index} has an empty/non-string id", file=sys.stderr)
            return 1
        if item["id"] in seen:
            print(f"ERROR: duplicate id: {item['id']}", file=sys.stderr)
            return 1
        seen.add(item["id"])
        if not isinstance(item["title"], str) or not item["title"].strip():
            print(f"ERROR: item {index} has an empty/non-string title", file=sys.stderr)
            return 1
        source = item["source"]
        if not isinstance(source, dict) or source.get("type") not in {"magnet", "torrent_url"}:
            print(f"ERROR: item {index} source.type must be magnet or torrent_url", file=sys.stderr)
            return 1
        if not isinstance(source.get("uri"), str) or not source["uri"].strip():
            print(f"ERROR: item {index} source.uri must be a non-empty string", file=sys.stderr)
            return 1
        if source["type"] == "magnet" and not source["uri"].startswith("magnet:?"):
            print(f"ERROR: item {index} magnet source must start with magnet:?", file=sys.stderr)
            return 1
        if source["type"] == "torrent_url" and not source["uri"].startswith(("https://", "http://")):
            print(f"ERROR: item {index} torrent_url source must be HTTP(S)", file=sys.stderr)
            return 1

    print(f"OK: {path} — {len(data['items'])} catalog item(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
