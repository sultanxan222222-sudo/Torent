#include "service.h"
#include "net.h"
#include "log.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>

static const char* SERVICE_HOST = "127.0.0.1";
static const int   LOADER_PORT  = 9090;
static const char* PAYLOAD_PATH = "/app0/assets/misc/ps4torrentd.elf";

// Два разных пути для одного и того же файла (проверено на ps4hddtools):
//  - AUTORUN_ELF_PATH: под этим путём приложение реально может писать (fopen/mkdir); "/user/data/payloads/..." из
//    песочницы приложения для записи недоступен;
//  - AUTORUN_INI_ENTRY: этот путь (с "/user") GoldHEN ожидает в секции [AutoRun] файла payloads.ini.
static bool read_payload(std::string& data, std::string& err)
{
    FILE* f = fopen(PAYLOAD_PATH, "rb");
    if (!f) { err = "no payload file in PKG"; logf_("service: cannot open %s", PAYLOAD_PATH); return false; }
    char buf[16384];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) data.append(buf, n);
    fclose(f);
    if (data.size() < 1024) { err = "payload file is empty"; return false; }
    return true;
}

bool service_send_payload(std::string& err, bool* connectFailed)
{
    if (connectFailed) *connectFailed = false;

    std::string data;
    if (!read_payload(data, err)) return false;

    int s = net_connect(SERVICE_HOST, LOADER_PORT, 3000);
    if (s < 0) {
        err = "BinLoader (port 9090) is not available. Is GoldHEN running?";
        logf_("service: BinLoader connect failed");
        if (connectFailed) *connectFailed = true;
        return false;
    }

    struct timeval tv;                       // отправка ~1,4 МБ: даём BinLoader время (по умолчанию net_connect ставит 3 с)
    tv.tv_sec = 20; tv.tv_usec = 0;
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    size_t sent = 0;
    while (sent < data.size()) {
        int w = send(s, data.data() + sent, data.size() - sent, 0);
        if (w <= 0) {
            err = "sending to BinLoader failed";
            logf_("service: send failed after %d of %d bytes", (int)sent, (int)data.size());
            close(s);
            return false;
        }
        sent += (size_t)w;
    }
    close(s);
    logf_("service: payload sent (%d bytes)", (int)sent);
    return true;
}
