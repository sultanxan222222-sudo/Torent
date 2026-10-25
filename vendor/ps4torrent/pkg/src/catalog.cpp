#include "catalog.h"
#include "json.h"
#include <stdio.h>
#include <vector>

static std::vector<CatalogEntry> g_entries;
static std::string g_error;

static bool read_file(const char* path, std::string& out)
{
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    out.clear();
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        if (out.size() + n > 512 * 1024) {
            fclose(f);
            g_error = "catalog file exceeds 512 KiB limit";
            return false;
        }
        out.append(buf, n);
    }
    bool ok = !ferror(f);
    fclose(f);
    return ok;
}

bool catalog_load()
{
    g_entries.clear();
    g_error.clear();
    std::string raw;
    if (!read_file("/data/ps4tc_client/catalog.json", raw) &&
        !read_file("/app0/assets/misc/catalog.json", raw)) {
        g_error = "catalog.json not found";
        return false;
    }

    JsonValue root;
    if (!json_parse(raw, root) || root.type != JsonValue::OBJ ||
        root["version"].asInt(0) != 1 || root["items"].type != JsonValue::ARR) {
        g_error = "catalog.json has an invalid structure";
        return false;
    }

    const JsonValue& items = root["items"];
    for (size_t i = 0; i < items.arr.size() && g_entries.size() < 200; ++i) {
        const JsonValue& value = items.arr[i];
        if (value.type != JsonValue::OBJ) continue;
        CatalogEntry e;
        e.id = value["id"].asStr();
        e.title = value["title"].asStr();
        e.description = value["description"].asStr();
        e.category = value["category"].asStr("Other");
        e.platform = value["platform"].asStr("PS4");
        e.cover = value["cover"].asStr();
        e.sourceType = value["source"]["type"].asStr();
        e.sourceUri = value["source"]["uri"].asStr();
        if (e.id.empty() || e.title.empty()) continue;
        if (e.sourceType != "magnet" && e.sourceType != "torrent_url") continue;
        if (e.sourceUri.empty()) continue;
        g_entries.push_back(e);
    }

    if (g_entries.empty()) {
        g_error = "catalog has no valid entries";
        return false;
    }
    return true;
}

const std::vector<CatalogEntry>& catalog_items() { return g_entries; }
const std::string& catalog_last_error() { return g_error; }
