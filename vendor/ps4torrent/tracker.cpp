#include "tracker.h"
#include "net.h"
#include "bencode.h"
#include "log.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <time.h>

static int g_announcePort = 6881;
void tracker_set_port(int port) { if (port > 0 && port < 65536) g_announcePort = port; }

static std::string urlencode(const unsigned char* d, size_t n) {
    static const char* hex = "0123456789ABCDEF";
    std::string out;
    for (size_t i = 0; i < n; i++) {
        unsigned char c = d[i];
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') || c == '-' || c == '.' || c == '_' || c == '~') {
            out += (char)c;
        } else {
            out += '%';
            out += hex[c >> 4];
            out += hex[c & 15];
        }
    }
    return out;
}

bool tracker_announce_http(const std::string& url, const unsigned char infoHash[20],
                           const unsigned char peerId[20], uint64_t left,
                           AnnounceResult& res) {
    if (url.compare(0, 7, "http://") != 0) { res.error = "not http"; return false; }

    std::string rest = url.substr(7);
    std::string hostport = rest, path = "/";
    size_t slash = rest.find('/');
    if (slash != std::string::npos) {
        hostport = rest.substr(0, slash);
        path = rest.substr(slash);
    }
    std::string host = hostport;
    int port = 80;
    size_t colon = hostport.find(':');
    if (colon != std::string::npos) {
        host = hostport.substr(0, colon);
        port = atoi(hostport.c_str() + colon + 1);
    }

    char tail[112];
    snprintf(tail, sizeof(tail),
             "&port=%d&uploaded=0&downloaded=0&left=%llu&compact=1&numwant=50&event=started",
             g_announcePort, (unsigned long long)left);
    std::string q = path;
    q += (path.find('?') == std::string::npos) ? '?' : '&';
    q += "info_hash=" + urlencode(infoHash, 20);
    q += "&peer_id=" + urlencode(peerId, 20);
    q += tail;

    std::string req = "GET " + q + " HTTP/1.0\r\nHost: " + host +
                      "\r\nUser-Agent: ps4torrent/0.1\r\nConnection: close\r\n\r\n";

    int s = net_connect(host.c_str(), port, 8000);
    if (s < 0) { res.error = "connect failed"; return false; }

    size_t sent = 0;
    while (sent < req.size()) {
        int n = send(s, req.data() + sent, req.size() - sent, 0);
        if (n <= 0) { close(s); res.error = "send failed"; return false; }
        sent += n;
    }

    std::string resp;
    char buf[4096];
    for (;;) {
        int n = recv(s, buf, sizeof(buf), 0);
        if (n <= 0) break;
        resp.append(buf, n);
        if (resp.size() > 256 * 1024) break;
    }
    close(s);

    if (resp.empty()) { res.error = "empty response"; return false; }
    size_t hdrEnd = resp.find("\r\n\r\n");
    if (hdrEnd == std::string::npos) { res.error = "bad http response"; return false; }

    size_t eol = resp.find("\r\n");
    std::string status = resp.substr(0, eol);
    if (status.find(" 200") == std::string::npos) { res.error = "http: " + status; return false; }

    std::string body = resp.substr(hdrEnd + 4);
    BNode root;
    if (!bdecode(body, root) || root.type != BNode::DICT) { res.error = "bad tracker bencode"; return false; }

    const BNode* fr = root.find("failure reason");
    if (fr && fr->type == BNode::STR) { res.error = "tracker: " + fr->s; return false; }

    const BNode* iv = root.find("interval");
    if (iv && iv->type == BNode::INT) res.interval = (int)iv->i;

    const BNode* pe = root.find("peers");
    if (pe && pe->type == BNode::STR) {
        for (size_t i = 0; i + 6 <= pe->s.size(); i += 6) {
            Peer p;
            memcpy(p.ip, pe->s.data() + i, 4);
            p.port = ((unsigned char)pe->s[i + 4] << 8) | (unsigned char)pe->s[i + 5];
            res.peers.push_back(p);
        }
    } else if (pe && pe->type == BNode::LIST) {
        for (size_t i = 0; i < pe->l.size(); i++) {
            const BNode* ip = pe->l[i].find("ip");
            const BNode* pt = pe->l[i].find("port");
            if (!ip || !pt || ip->type != BNode::STR || pt->type != BNode::INT) continue;
            unsigned a, b, c, d;
            if (sscanf(ip->s.c_str(), "%u.%u.%u.%u", &a, &b, &c, &d) != 4) continue;
            Peer p;
            p.ip[0] = a; p.ip[1] = b; p.ip[2] = c; p.ip[3] = d;
            p.port = (uint16_t)pt->i;
            res.peers.push_back(p);
        }
    }
    return true;
}

static void put32t(unsigned char* p, uint32_t v) {
    p[0] = (unsigned char)(v >> 24); p[1] = (unsigned char)(v >> 16);
    p[2] = (unsigned char)(v >> 8); p[3] = (unsigned char)v;
}
static uint32_t get32t(const unsigned char* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}
static void put64t(unsigned char* p, uint64_t v) {
    for (int i = 7; i >= 0; --i) { p[i] = (unsigned char)v; v >>= 8; }
}

static bool tracker_announce_udp(const std::string& url, const unsigned char infoHash[20],
                                 const unsigned char peerId[20], uint64_t left,
                                 AnnounceResult& res) {
    if (url.compare(0, 6, "udp://") != 0) { res.error = "not udp"; return false; }
    std::string rest = url.substr(6);
    size_t slash = rest.find('/');
    std::string hostport = slash == std::string::npos ? rest : rest.substr(0, slash);
    size_t colon = hostport.rfind(':');
    if (colon == std::string::npos) { res.error = "UDP tracker URL has no port"; return false; }
    std::string host = hostport.substr(0, colon);
    int port = atoi(hostport.c_str() + colon + 1);
    if (host.empty() || port < 1 || port > 65535) { res.error = "bad UDP tracker host or port"; return false; }
    struct in_addr ip;
    if (!net_resolve(host.c_str(), &ip)) { res.error = "UDP tracker DNS failed"; return false; }

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { res.error = "UDP socket failed"; return false; }
    struct timeval tv; tv.tv_sec = 4; tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
#ifndef __linux__
    addr.sin_len = sizeof(addr);
#endif
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    addr.sin_addr = ip;

    unsigned char packet[98];
    uint32_t tx = (uint32_t)(time(NULL) ^ (uintptr_t)infoHash ^ (uintptr_t)peerId);
    memset(packet, 0, sizeof(packet));
    const uint64_t protocol = 0x41727101980ULL;
    for (int i = 7; i >= 0; --i) { packet[i] = (unsigned char)(protocol >> (8 * (7 - i))); }
    put32t(packet + 8, 0);
    put32t(packet + 12, tx);
    if (sendto(fd, packet, 16, 0, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd); res.error = "UDP tracker connect send failed"; return false;
    }
    unsigned char reply[2048];
    ssize_t n = recvfrom(fd, reply, sizeof(reply), 0, NULL, NULL);
    if (n < 16 || get32t(reply) != 0 || get32t(reply + 4) != tx) {
        close(fd); res.error = "UDP tracker connect timed out or returned invalid data"; return false;
    }
    uint64_t connection = 0;
    for (int i = 8; i < 16; ++i) connection = (connection << 8) | reply[i];

    memset(packet, 0, sizeof(packet));
    for (int i = 0; i < 8; ++i) packet[i] = (unsigned char)(connection >> (8 * (7 - i)));
    put32t(packet + 8, 1);
    put32t(packet + 12, tx);
    memcpy(packet + 16, infoHash, 20);
    memcpy(packet + 36, peerId, 20);
    put64t(packet + 56, 0);          // downloaded
    put64t(packet + 64, left);
    put64t(packet + 72, 0);          // uploaded
    put32t(packet + 80, 2);          // started
    put32t(packet + 84, 0);          // default IP
    put32t(packet + 88, tx ^ 0x5a5a5a5a);
    put32t(packet + 92, 50);         // num_want
    packet[96] = (unsigned char)(g_announcePort >> 8);
    packet[97] = (unsigned char)g_announcePort;
    if (sendto(fd, packet, sizeof(packet), 0, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd); res.error = "UDP tracker announce send failed"; return false;
    }
    n = recvfrom(fd, reply, sizeof(reply), 0, NULL, NULL);
    close(fd);
    if (n < 20 || get32t(reply) != 1 || get32t(reply + 4) != tx) {
        res.error = "UDP tracker announce timed out or returned invalid data"; return false;
    }
    res.interval = (int)get32t(reply + 8);
    for (ssize_t i = 20; i + 6 <= n; i += 6) {
        Peer p;
        memcpy(p.ip, reply + i, 4);
        p.port = (uint16_t)(((unsigned char)reply[i + 4] << 8) | (unsigned char)reply[i + 5]);
        if (p.port) res.peers.push_back(p);
    }
    return true;
}

bool tracker_announce(const std::string& url, const unsigned char infoHash[20],
                      const unsigned char peerId[20], uint64_t left,
                      AnnounceResult& res) {
    if (url.compare(0, 7, "http://") == 0)
        return tracker_announce_http(url, infoHash, peerId, left, res);
    if (url.compare(0, 6, "udp://") == 0)
        return tracker_announce_udp(url, infoHash, peerId, left, res);
    res.error = "unsupported tracker scheme";
    return false;
}
