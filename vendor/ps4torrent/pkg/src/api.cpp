#include "api.h"
#include "app.h"
#include "httpc.h"
#include "json.h"
#include "log.h"
#include <algorithm>
#include <stdlib.h>

static const int TIMEOUT_MS = 3000;
static std::string g_lastErr;

static std::string with_token(const std::string& path)
{
    if (g_token.empty()) return path;
    return path + (path.find('?') == std::string::npos ? "?token=" : "&token=") + g_token;
}

bool api_refresh_status()
{
    HttpResult r = http_get(g_host, g_port, with_token("/status"), TIMEOUT_MS);
    if (!r.connected) { g_status.ok = false; g_lastErr = "no connection to ps4torrentd"; return false; }
    if (r.status != 200) { g_status.ok = false; g_lastErr = "server returned status " + std::to_string(r.status); return false; }

    JsonValue j;
    if (!json_parse(r.body, j) || j.type != JsonValue::OBJ) {
        g_status.ok = false;
        g_lastErr = "bad JSON from server";
        return false;
    }

    ServerStatus st;
    st.ok = true;
    const JsonValue& items = j["items"];
    for (size_t i = 0; i < items.arr.size(); i++) {
        const JsonValue& it = items.arr[i];
        Item x;
        x.hash = it["hash"].asStr();
        x.status = it["status"].asStr("queued");
        x.size = (uint64_t)it["size"].asInt(0);
        x.done = (uint64_t)it["done"].asInt(0);
        x.pct = (int)it["pct"].asInt(0);
        x.speedKB = (int)it["speed_kb"].asInt(0);
        x.etaSec = it["eta_s"].asInt(-1);
        x.peers = (int)it["peers"].asInt(0);
        x.paused = it["paused"].asBool(false);
        x.lowSpace = it["low_space"].asBool(false);
        x.root = it["root"].asStr();
        x.title = it["title"].asStr();
        if (x.hash.empty()) continue;   // битая запись: пропускаем, а не падаем
        st.items.push_back(x);
    }

    const JsonValue& drives = j["drives"];
    for (size_t i = 0; i < drives.arr.size(); i++) {
        const JsonValue& d = drives.arr[i];
        Drive dr;
        dr.root = d["root"].asStr();
        dr.freeBytes = (uint64_t)d["free"].asInt(0);
        dr.totalBytes = (uint64_t)d["total"].asInt(0);
        dr.isInternal = (d["kind"].asStr() == "internal");
        dr.low = d["low"].asBool(false);
        st.drives.push_back(dr);
    }

    st.totalSpeedKB = (int)j["speed_kb"].asInt(0);
    st.maxParallel = (int)j["max_parallel"].asInt(0);
    st.listenPort = (int)j["listen_port"].asInt(0);
    st.incoming = (unsigned)j["incoming"].asInt(0);
    st.deleting = (int)j["deleting"].asInt(0);
    st.netIp = j["net_ip"].asStr();
    st.version = j["version"].asStr();

    g_status = st;
    g_lastErr.clear();
    return true;
}

// Общий код для команд-"одна строка результата". Разбирает {"ok":true} / {"ok":false,"error":"..."}.
static bool simple_cmd(const std::string& path)
{
    HttpResult r = http_get(g_host, g_port, with_token(path), TIMEOUT_MS);
    if (!r.connected) { g_lastErr = "no connection to ps4torrentd"; return false; }

    JsonValue j;
    bool parsed = json_parse(r.body, j) && j.type == JsonValue::OBJ;
    bool ok = parsed && j["ok"].asBool(false);

    if (!ok) {
        g_lastErr = parsed ? j["error"].asStr("request failed") : ("server returned status " + std::to_string(r.status));
        logf_("api: %s -> %s", path.c_str(), g_lastErr.c_str());
    }
    return ok;
}

bool api_pause(const std::string& hash)  { return simple_cmd("/api/pause?hash=" + hash); }
bool api_resume(const std::string& hash) { return simple_cmd("/api/resume?hash=" + hash); }
bool api_pause_all()  { return simple_cmd("/api/pause_all"); }
bool api_resume_all() { return simple_cmd("/api/resume_all"); }

bool api_delete(const std::string& hash, bool withFiles)
{
    std::string path = "/api/delete?hash=" + hash;
    if (withFiles) path += "&files=1";
    return simple_cmd(path);
}

bool api_quit()
{
    HttpResult r = http_get(g_host, g_port, with_token("/quit"), TIMEOUT_MS);
    if (!r.connected) { g_lastErr = "no connection to ps4torrentd"; return false; }
    if (r.status != 200) { g_lastErr = "server returned status " + std::to_string(r.status); return false; }
    logf_("api: /quit -> %s", r.body.c_str());   // ожидаем "bye"
    return true;
}


static std::string query_encode(const std::string& s)
{
    static const char hex[] = "0123456789ABCDEF";
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = (unsigned char)s[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') out += (char)c;
        else { out += '%'; out += hex[c >> 4]; out += hex[c & 15]; }
    }
    return out;
}

bool api_add_catalog_torrent(const std::string& uri, const std::string& title)
{
    const std::string prefix = "http://";
    if (uri.compare(0, prefix.size(), prefix) != 0) {
        g_lastErr = "only direct http:// .torrent URLs are supported; HTTPS and magnet are not supported yet";
        return false;
    }
    std::string rest = uri.substr(prefix.size());
    size_t slash = rest.find('/');
    size_t query = rest.find('?');
    size_t cut = rest.size();
    if (slash != std::string::npos && slash < cut) cut = slash;
    if (query != std::string::npos && query < cut) cut = query;
    std::string hostPort = rest.substr(0, cut);
    std::string path = cut == rest.size() ? "/" : rest.substr(cut);
    if (!path.empty() && path[0] == '?') path = "/" + path;
    if (hostPort.empty() || hostPort.find('@') != std::string::npos || hostPort.find('#') != std::string::npos) {
        g_lastErr = "invalid catalog URL";
        return false;
    }
    int remotePort = 80;
    std::string remoteHost = hostPort;
    size_t colon = hostPort.rfind(':');
    if (colon != std::string::npos) {
        remoteHost = hostPort.substr(0, colon);
        std::string portText = hostPort.substr(colon + 1);
        if (portText.empty()) { g_lastErr = "invalid catalog URL port"; return false; }
        for (size_t i = 0; i < portText.size(); ++i)
            if (portText[i] < '0' || portText[i] > '9') { g_lastErr = "invalid catalog URL port"; return false; }
        remotePort = atoi(portText.c_str());
        if (remotePort < 1 || remotePort > 65535) { g_lastErr = "invalid catalog URL port"; return false; }
    }
    if (remoteHost.empty() || remoteHost.find(' ') != std::string::npos) {
        g_lastErr = "invalid catalog URL host";
        return false;
    }

    HttpResult remote = http_get(remoteHost, remotePort, path, 10000);
    if (!remote.connected || remote.status != 200 || remote.body.empty()) {
        g_lastErr = !remote.connected ? "cannot connect to catalog URL" :
                    ("catalog URL returned HTTP " + std::to_string(remote.status));
        return false;
    }
    if (remote.body.size() > 1024 * 1024) {
        g_lastErr = "torrent file exceeds 1 MiB limit";
        return false;
    }
    std::string filename = (title.empty() ? "catalog-download" : title) + ".torrent";
    std::string postPath = "/api/add?name=" + query_encode(filename) + "&drive=internal&start=1";
    HttpResult local = http_post(g_host, g_port, with_token(postPath), remote.body, 10000);
    if (!local.connected || local.status != 200) {
        g_lastErr = !local.connected ? "cannot send torrent to ps4torrentd" :
                    ("ps4torrentd returned HTTP " + std::to_string(local.status));
        return false;
    }
    JsonValue response;
    if (!json_parse(local.body, response) || response.type != JsonValue::OBJ || !response["ok"].asBool(false)) {
        g_lastErr = response.type == JsonValue::OBJ ? response["error"].asStr("daemon rejected torrent") : "daemon returned invalid JSON";
        return false;
    }
    g_lastErr.clear();
    return true;
}


bool api_add_torrent_file(const std::string& path)
{
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) { g_lastErr = "cannot open .torrent file"; return false; }
    std::string body;
    char buf[4096];
    size_t n = 0;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        if (body.size() + n > 1024 * 1024) {
            fclose(f); g_lastErr = "torrent file exceeds 1 MiB limit"; return false;
        }
        body.append(buf, n);
    }
    bool readOk = !ferror(f);
    fclose(f);
    if (!readOk || body.empty() || body[0] != 'd' || body.find("4:info") == std::string::npos) {
        g_lastErr = "file is not a valid .torrent file";
        return false;
    }
    size_t slash = path.find_last_of('/');
    std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
    if (name.empty()) name = "local.torrent";
    std::string postPath = "/api/add?name=" + query_encode(name) + "&drive=internal&start=1";
    HttpResult local = http_post(g_host, g_port, with_token(postPath), body, 10000);
    if (!local.connected || local.status != 200) {
        g_lastErr = !local.connected ? "cannot send torrent to ps4torrentd" :
                    ("ps4torrentd returned HTTP " + std::to_string(local.status));
        return false;
    }
    JsonValue response;
    if (!json_parse(local.body, response) || response.type != JsonValue::OBJ || !response["ok"].asBool(false)) {
        g_lastErr = response.type == JsonValue::OBJ ? response["error"].asStr("daemon rejected torrent") : "daemon returned invalid JSON";
        return false;
    }
    g_lastErr.clear();
    return true;
}

std::string api_last_error() { return g_lastErr; }
