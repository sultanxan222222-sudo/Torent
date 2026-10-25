#include "httpc.h"
#include "net.h"
#include <unistd.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <orbis/Http.h>
#include <orbis/Ssl.h>
#include <orbis/Net.h>
#include <orbis/Sysmodule.h>

// Сервер отвечает HTTP/1.0 и сам закрывает соединение после ответа (см. respond_status
// в ps4torrentd), поэтому читаем до конца сокета — ждать Content-Length не обязательно,
// но мы всё равно его используем, если он есть, чтобы не зависеть от поведения сервера.

static bool send_all(int s, const char* data, size_t len)
{
    size_t sent = 0;
    while (sent < len) {
        int n = send(s, data + sent, len - sent, 0);
        if (n <= 0) return false;
        sent += (size_t)n;
    }
    return true;
}

static HttpResult do_request(const std::string& host, int port, const std::string& req, int timeout_ms)
{
    HttpResult r;
    int s = net_connect(host.c_str(), port, timeout_ms);
    if (s < 0) return r;
    r.connected = true;

    if (!send_all(s, req.data(), req.size())) { close(s); return r; }

    std::string resp;
    char buf[8192];
    for (;;) {
        int n = recv(s, buf, sizeof(buf), 0);
        if (n <= 0) break;
        resp.append(buf, (size_t)n);
        if (resp.size() > 8 * 1024 * 1024) break;   // защита от бесконечного ответа
    }
    close(s);

    size_t hend = resp.find("\r\n\r\n");
    if (hend == std::string::npos) return r;   // подключились, но не получили даже заголовков

    std::string head = resp.substr(0, hend);
    size_t firstLine = head.find("\r\n");
    r.headers = firstLine == std::string::npos ? std::string() : head.substr(firstLine + 2);
    r.body = resp.substr(hend + 4);

    // "HTTP/1.0 200 OK" -> 200
    size_t sp1 = head.find(' ');
    if (sp1 != std::string::npos) r.status = atoi(head.c_str() + sp1 + 1);

    return r;
}

HttpResult http_get(const std::string& host, int port, const std::string& path, int timeout_ms)
{
    char req[1024];
    snprintf(req, sizeof(req),
             "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", path.c_str(), host.c_str());
    return do_request(host, port, req, timeout_ms);
}

HttpResult http_post(const std::string& host, int port, const std::string& path,
                     const std::string& body, int timeout_ms)
{
    char head[1024];
    snprintf(head, sizeof(head),
             "POST %s HTTP/1.0\r\nHost: %s\r\nContent-Type: application/octet-stream\r\n"
             "Content-Length: %d\r\nConnection: close\r\n\r\n",
             path.c_str(), host.c_str(), (int)body.size());
    return do_request(host, port, std::string(head) + body, timeout_ms);
}


// HTTPS via the PS4 system HTTP/TLS stack. We deliberately do not install a
// permissive SSL callback: the system's certificate validation remains enabled.
static int g_httpsNetPool = -1, g_httpsSslCtx = -1, g_httpsHttpCtx = -1;
static bool g_httpsInitTried = false;
static bool https_init()
{
    if (g_httpsHttpCtx >= 0) return true;
    if (g_httpsInitTried) return false;
    g_httpsInitTried = true;
    if ((int32_t)sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_NET) < 0 ||
        (int32_t)sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_HTTP) < 0 ||
        (int32_t)sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SSL) < 0) return false;
    if (sceNetInit() < 0) return false;
    g_httpsNetPool = sceNetPoolCreate("torentHttpPool", 16 * 1024, 0);
    if (g_httpsNetPool < 0) return false;
    g_httpsSslCtx = sceSslInit(SSL_POOLSIZE);
    if (g_httpsSslCtx < 0) return false;
    g_httpsHttpCtx = sceHttpInit(g_httpsNetPool, g_httpsSslCtx, LIBHTTP_POOLSIZE);
    return g_httpsHttpCtx >= 0;
}
HttpResult http_get_secure_url(const std::string& url, int timeout_ms)
{
    HttpResult r;
    if (url.compare(0, 8, "https://") != 0 || !https_init()) return r;
    int tpl = sceHttpCreateTemplate(g_httpsHttpCtx, "Torent/1.0 (PS4)", ORBIS_HTTP_VERSION_1_1, 0);
    if (tpl < 0) return r;
    sceHttpSetConnectTimeOut(tpl, (uint32_t)timeout_ms * 1000U);
    int conn = sceHttpCreateConnectionWithURL(tpl, url.c_str(), 0);
    if (conn < 0) { sceHttpDeleteTemplate(tpl); return r; }
    int req = sceHttpCreateRequestWithURL(conn, ORBIS_METHOD_GET, url.c_str(), 0);
    if (req < 0) { sceHttpDeleteConnection(conn); sceHttpDeleteTemplate(tpl); return r; }
    int sent = sceHttpSendRequest(req, NULL, 0);
    int32_t status = 0;
    if (sent >= 0 && sceHttpGetStatusCode(req, &status) >= 0) {
        r.connected = true;
        r.status = (int)status;
        char* headers = NULL;
        size_t headersSize = 0;
        if (sceHttpGetAllResponseHeaders(req, &headers, &headersSize) >= 0 && headers && headersSize) {
            r.headers.assign(headers, headersSize);
            free(headers);
        }
        char buf[8192];
        while (r.body.size() <= 1024 * 1024) {
            int n = sceHttpReadData(req, buf, sizeof(buf));
            if (n <= 0) break;
            r.body.append(buf, (size_t)n);
        }
    }
    sceHttpDeleteRequest(req);
    sceHttpDeleteConnection(conn);
    sceHttpDeleteTemplate(tpl);
    return r;
}
