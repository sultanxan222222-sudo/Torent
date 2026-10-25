#include "magnet.h"
#include "net.h"
#include "tracker.h"
#include "sha1.h"
#include "bencode.h"
#include "log.h"
#include <algorithm>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

static std::string pct_decode(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size() &&
            isxdigit((unsigned char)s[i + 1]) && isxdigit((unsigned char)s[i + 2])) {
            char hx[3] = {s[i + 1], s[i + 2], 0};
            out += (char)strtol(hx, NULL, 16);
            i += 2;
        } else if (s[i] == '+') out += ' ';
        else out += s[i];
    }
    return out;
}

static std::vector<std::string> magnet_values(const std::string& uri, const char* key) {
    std::vector<std::string> out;
    size_t q = uri.find('?');
    if (q == std::string::npos) return out;
    std::string query = uri.substr(q + 1);
    size_t p = 0;
    while (p <= query.size()) {
        size_t e = query.find('&', p);
        if (e == std::string::npos) e = query.size();
        std::string part = query.substr(p, e - p);
        size_t eq = part.find('=');
        if (eq != std::string::npos && part.substr(0, eq) == key)
            out.push_back(pct_decode(part.substr(eq + 1)));
        if (e == query.size()) break;
        p = e + 1;
    }
    return out;
}

static int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    c = (char)tolower((unsigned char)c);
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static bool parse_btih(const std::string& xt, unsigned char hash[20]) {
    const std::string prefix = "urn:btih:";
    if (xt.compare(0, prefix.size(), prefix) != 0) return false;
    std::string v = xt.substr(prefix.size());
    if (v.size() == 40) {
        for (int i = 0; i < 20; ++i) {
            int a = hexval(v[i * 2]), b = hexval(v[i * 2 + 1]);
            if (a < 0 || b < 0) return false;
            hash[i] = (unsigned char)((a << 4) | b);
        }
        return true;
    }
    if (v.size() == 32) {
        static const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
        uint32_t acc = 0; int bits = 0; size_t n = 0;
        for (size_t i = 0; i < v.size(); ++i) {
            char c = (char)toupper((unsigned char)v[i]);
            const char* p = strchr(alphabet, c);
            if (!p) return false;
            acc = (acc << 5) | (uint32_t)(p - alphabet);
            bits += 5;
            if (bits >= 8) {
                bits -= 8;
                if (n >= 20) return false;
                hash[n++] = (unsigned char)(acc >> bits);
                acc &= (1u << bits) - 1u;
            }
        }
        return n == 20;
    }
    return false;
}

static bool send_all_fd(int fd, const unsigned char* data, size_t size) {
    size_t sent = 0;
    while (sent < size) {
        ssize_t n = send(fd, data + sent, size - sent, 0);
        if (n <= 0) return false;
        sent += (size_t)n;
    }
    return true;
}

static bool recv_exact_fd(int fd, unsigned char* data, size_t size) {
    size_t got = 0;
    while (got < size) {
        ssize_t n = recv(fd, data + got, size - got, 0);
        if (n <= 0) return false;
        got += (size_t)n;
    }
    return true;
}

static void put32be(unsigned char* p, uint32_t v) {
    p[0] = (unsigned char)(v >> 24);
    p[1] = (unsigned char)(v >> 16);
    p[2] = (unsigned char)(v >> 8);
    p[3] = (unsigned char)v;
}

static uint32_t get32be(const unsigned char* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static bool send_extended(int fd, unsigned char extId, const std::string& payload) {
    uint32_t len = (uint32_t)(2 + payload.size());
    unsigned char head[6];
    put32be(head, len);
    head[4] = 20;
    head[5] = extId;
    return send_all_fd(fd, head, sizeof(head)) &&
           send_all_fd(fd, (const unsigned char*)payload.data(), payload.size());
}

static bool read_message(int fd, std::vector<unsigned char>& body) {
    unsigned char h[4];
    if (!recv_exact_fd(fd, h, 4)) return false;
    uint32_t len = get32be(h);
    if (len == 0) { body.clear(); return true; }
    if (len > 256 * 1024) return false;
    body.resize(len);
    return recv_exact_fd(fd, body.data(), len);
}

static bool metadata_from_peer(const Peer& peer, const unsigned char infoHash[20],
                               const unsigned char peerId[20], std::string& info) {
    char ip[24];
    snprintf(ip, sizeof(ip), "%u.%u.%u.%u", peer.ip[0], peer.ip[1], peer.ip[2], peer.ip[3]);
    if (!peer.port) return false;
    int fd = net_connect(ip, peer.port, 5000);
    if (fd < 0) return false;

    unsigned char hs[68];
    memset(hs, 0, sizeof(hs));
    hs[0] = 19;
    memcpy(hs + 1, "BitTorrent protocol", 19);
    hs[25] = 0x10; // BEP 10 extension protocol
    memcpy(hs + 28, infoHash, 20);
    memcpy(hs + 48, peerId, 20);
    if (!send_all_fd(fd, hs, sizeof(hs))) { close(fd); return false; }

    unsigned char response[68];
    if (!recv_exact_fd(fd, response, sizeof(response)) ||
        response[0] != 19 || memcmp(response + 1, "BitTorrent protocol", 19) != 0 ||
        memcmp(response + 28, infoHash, 20) != 0 || !(response[25] & 0x10)) {
        close(fd); return false;
    }

    const std::string extHandshake = "d1:md11:ut_metadatai3ee1:v10:PS4Torrente";
    if (!send_extended(fd, 0, extHandshake)) { close(fd); return false; }

    struct timeval peerTv; peerTv.tv_sec = 2; peerTv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &peerTv, sizeof(peerTv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &peerTv, sizeof(peerTv));

    int remoteMetadataId = -1;
    time_t extDeadline = time(NULL) + 8;
    uint32_t metadataSize = 0;
    std::vector<unsigned char> msg;
    for (int tries = 0; tries < 8 && remoteMetadataId < 0 && time(NULL) < extDeadline; ++tries) {
        if (!read_message(fd, msg)) { close(fd); return false; }
        if (msg.size() < 2 || msg[0] != 20 || msg[1] != 0) continue;
        std::string payload((const char*)msg.data() + 2, msg.size() - 2);
        BNode root;
        if (!bdecode(payload, root) || root.type != BNode::DICT) continue;
        const BNode* md = root.find("m");
        const BNode* us = root.find("metadata_size");
        if (!md || md->type != BNode::DICT || !us || us->type != BNode::INT ||
            us->i <= 0 || us->i > 1024 * 1024) continue;
        const BNode* ut = md->find("ut_metadata");
        if (!ut || ut->type != BNode::INT || ut->i <= 0 || ut->i > 255) continue;
        remoteMetadataId = (int)ut->i;
        metadataSize = (uint32_t)us->i;
    }
    if (remoteMetadataId < 0 || metadataSize == 0) { close(fd); return false; }

    const uint32_t pieceSize = 16384;
    uint32_t pieceCount = (metadataSize + pieceSize - 1) / pieceSize;
    std::vector<unsigned char> data(metadataSize, 0);
    std::vector<unsigned char> have(pieceCount, 0);
    for (uint32_t i = 0; i < pieceCount; ++i) {
        char request[96];
        snprintf(request, sizeof(request), "d8:msg_typei0e5:piecei%ue", i);
        if (!send_extended(fd, (unsigned char)remoteMetadataId, request)) {
            close(fd); return false;
        }
    }

    uint32_t haveCount = 0;
    time_t metadataDeadline = time(NULL) + 12;
    for (int tries = 0; tries < (int)(pieceCount * 4 + 24) && haveCount < pieceCount && time(NULL) < metadataDeadline; ++tries) {
        if (!read_message(fd, msg)) break;
        if (msg.size() < 3 || msg[0] != 20 || msg[1] != 1) continue;
        std::string payload((const char*)msg.data() + 2, msg.size() - 2);
        BNode root;
        if (!bdecode(payload, root) || root.type != BNode::DICT) continue;
        const BNode* mt = root.find("msg_type");
        const BNode* pi = root.find("piece");
        if (!mt || mt->type != BNode::INT || mt->i != 1 ||
            !pi || pi->type != BNode::INT || pi->i < 0 || (uint64_t)pi->i >= pieceCount) continue;
        uint32_t index = (uint32_t)pi->i;
        if (have[index]) continue;
        size_t dataAt = 2 + root.end;
        uint32_t expected = (index + 1 == pieceCount) ? metadataSize - index * pieceSize : pieceSize;
        if (dataAt > msg.size() || msg.size() - dataAt < expected) continue;
        memcpy(data.data() + index * pieceSize, msg.data() + dataAt, expected);
        have[index] = 1;
        ++haveCount;
    }
    close(fd);
    if (haveCount != pieceCount) return false;
    unsigned char gotHash[20];
    sha1(data.data(), data.size(), gotHash);
    if (memcmp(gotHash, infoHash, 20) != 0) return false;
    info.assign((const char*)data.data(), data.size());
    return true;
}

bool magnet_resolve(const std::string& uri, const unsigned char peerId[20],
                    std::string& torrentBytes, std::string& error) {
    if (uri.compare(0, 8, "magnet:?") != 0) {
        error = "not a magnet URI"; return false;
    }
    std::vector<std::string> xts = magnet_values(uri, "xt");
    unsigned char infoHash[20];
    bool foundHash = false;
    for (size_t i = 0; i < xts.size(); ++i)
        if (parse_btih(xts[i], infoHash)) { foundHash = true; break; }
    if (!foundHash) {
        error = "magnet URI must contain a valid xt=urn:btih info-hash"; return false;
    }

    std::vector<std::string> trackers = magnet_values(uri, "tr");
    std::vector<std::string> usableTrackers;
    for (size_t i = 0; i < trackers.size(); ++i) {
        bool supported = trackers[i].compare(0, 7, "http://") == 0 ||
                         trackers[i].compare(0, 6, "udp://") == 0;
        if (supported && std::find(usableTrackers.begin(), usableTrackers.end(), trackers[i]) == usableTrackers.end())
            usableTrackers.push_back(trackers[i]);
    }
    if (usableTrackers.empty()) {
        error = "magnet has no supported HTTP/UDP tracker; DHT discovery is not implemented yet";
        return false;
    }

    std::vector<Peer> peers;
    for (size_t i = 0; i < usableTrackers.size() && i < 4 && peers.size() < 80; ++i) {
        AnnounceResult ar;
        if (!tracker_announce(usableTrackers[i], infoHash, peerId, 1, ar)) {
            logf_("magnet: tracker failed: %s", ar.error.c_str());
            continue;
        }
        for (size_t j = 0; j < ar.peers.size() && peers.size() < 80; ++j) {
            bool duplicate = false;
            for (size_t k = 0; k < peers.size(); ++k)
                if (peers[k].port == ar.peers[j].port && memcmp(peers[k].ip, ar.peers[j].ip, 4) == 0) { duplicate = true; break; }
            if (!duplicate) peers.push_back(ar.peers[j]);
        }
    }
    if (peers.empty()) {
        error = "trackers returned no peers; magnet metadata could not be found";
        return false;
    }

    std::string info;
    for (size_t i = 0; i < peers.size() && i < 10; ++i) {
        if (metadata_from_peer(peers[i], infoHash, peerId, info)) {
            std::string announce = usableTrackers[0];
            torrentBytes = "d8:announce" + std::to_string(announce.size()) + ":" + announce +
                           "4:info" + info + "e";
            error.clear();
            logf_("magnet: metadata resolved from peer %u.%u.%u.%u:%u (%u bytes)",
                  peers[i].ip[0], peers[i].ip[1], peers[i].ip[2], peers[i].ip[3],
                  peers[i].port, (unsigned)info.size());
            return true;
        }
    }
    error = "could not obtain verified metadata from peers (tracker peers may not support BEP 10/ut_metadata)";
    return false;
}
