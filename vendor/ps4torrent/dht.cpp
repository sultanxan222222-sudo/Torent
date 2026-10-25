#include "dht.h"
#include "net.h"
#include "bencode.h"
#include "sha1.h"
#include "log.h"
#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <set>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

struct DhtNode {
    sockaddr_in addr;
    unsigned char id[20];
};

static void dht_put16(std::string& s, unsigned v) {
    s.push_back((char)(v >> 8)); s.push_back((char)v);
}
static void dht_put32(std::string& s, uint32_t v) {
    s.push_back((char)(v >> 24)); s.push_back((char)(v >> 16));
    s.push_back((char)(v >> 8)); s.push_back((char)v);
}
static uint32_t dht_get32(const unsigned char* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}
static std::string bstr(const char* key, const unsigned char* data, size_t n) {
    return std::to_string(strlen(key)) + ":" + key + std::to_string(n) + ":" +
           std::string((const char*)data, n);
}
static std::string node_key(const sockaddr_in& a) {
    char ip[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &a.sin_addr, ip, sizeof(ip));
    return std::string(ip) + ":" + std::to_string(ntohs(a.sin_port));
}
static bool add_peer(std::vector<Peer>& peers, const unsigned char* p) {
    Peer peer;
    memcpy(peer.ip, p, 4);
    peer.port = (uint16_t)(((unsigned)p[4] << 8) | p[5]);
    if (!peer.port || (peer.ip[0] == 0) || peer.ip[0] >= 224) return false;
    for (size_t i = 0; i < peers.size(); ++i)
        if (peers[i].port == peer.port && memcmp(peers[i].ip, peer.ip, 4) == 0) return false;
    peers.push_back(peer);
    return true;
}

bool dht_get_peers(const unsigned char infoHash[20], const unsigned char peerId[20],
                   std::vector<Peer>& peers, std::string& error) {
    peers.clear();
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { error = "DHT: не удалось создать UDP-сокет"; return false; }
    struct timeval tv; tv.tv_sec = 0; tv.tv_usec = 700000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    unsigned char seed[40], localId[20];
    memcpy(seed, peerId, 20); memcpy(seed + 20, infoHash, 20);
    sha1(seed, sizeof(seed), localId);
    std::vector<DhtNode> queue;
    const char* hosts[] = {"router.bittorrent.com", "router.utorrent.com", "dht.transmissionbt.com"};
    const int ports[] = {6881, 6881, 6881};
    for (int i = 0; i < 3; ++i) {
        struct in_addr ip;
        if (!net_resolve(hosts[i], &ip)) continue;
        DhtNode n; memset(&n, 0, sizeof(n));
        n.addr.sin_family = AF_INET; n.addr.sin_addr = ip; n.addr.sin_port = htons((uint16_t)ports[i]);
        memcpy(n.id, localId, 20);
        if (queue.empty() || node_key(queue[0].addr) != node_key(n.addr)) queue.push_back(n);
    }
    if (queue.empty()) {
        close(fd); error = "DHT: не удалось найти bootstrap-узлы (проверь DNS/интернет)";
        return false;
    }

    std::set<std::string> queried;
    unsigned txCounter = (unsigned)time(NULL) ^ ((unsigned)infoHash[0] << 8) ^ infoHash[19];
    time_t deadline = time(NULL) + 20;
    size_t cursor = 0;
    int replies = 0;
    while (cursor < queue.size() && cursor < 48 && peers.size() < 80 && time(NULL) < deadline) {
        DhtNode node = queue[cursor++];
        std::string nk = node_key(node.addr);
        if (!queried.insert(nk).second) continue;
        char tx[3] = {(char)('a' + ((txCounter >> 5) % 26)), (char)('a' + (txCounter % 26)), 0};
        ++txCounter;
        std::string packet = "d1:ad2:id20:";
        packet.append((const char*)localId, 20);
        packet += "9:info_hash20:";
        packet.append((const char*)infoHash, 20);
        packet += "e1:q9:get_peers1:t2:";
        packet.append(tx, 2);
        packet += "1:y1:qe";
        if (sendto(fd, packet.data(), packet.size(), 0,
                   (struct sockaddr*)&node.addr, sizeof(node.addr)) < 0) continue;

        unsigned char buf[4096];
        sockaddr_in from; socklen_t fromLen = sizeof(from);
        ssize_t nread = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr*)&from, &fromLen);
        if (nread < 8) continue;
        std::string response((const char*)buf, (size_t)nread);
        BNode root;
        if (!bdecode(response, root) || root.type != BNode::DICT) continue;
        const BNode* t = root.find("t");
        const BNode* y = root.find("y");
        const BNode* r = root.find("r");
        if (!t || t->type != BNode::STR || t->s != std::string(tx, 2) ||
            !y || y->type != BNode::STR || y->s != "r" || !r || r->type != BNode::DICT) continue;
        ++replies;

        const BNode* values = r->find("values");
        if (values && values->type == BNode::LIST) {
            for (size_t vi = 0; vi < values->l.size() && peers.size() < 80; ++vi)
                if (values->l[vi].type == BNode::STR && values->l[vi].s.size() == 6)
                    add_peer(peers, (const unsigned char*)values->l[vi].s.data());
        }
        const BNode* nodes = r->find("nodes");
        if (nodes && nodes->type == BNode::STR) {
            const std::string& compact = nodes->s;
            for (size_t off = 0; off + 26 <= compact.size() && queue.size() < 96; off += 26) {
                DhtNode next; memset(&next, 0, sizeof(next));
                memcpy(next.id, compact.data() + off, 20);
                memcpy(&next.addr.sin_addr.s_addr, compact.data() + off + 20, 4);
                next.addr.sin_family = AF_INET;
                next.addr.sin_port = htons((uint16_t)(((unsigned char)compact[off + 24] << 8) |
                                                       (unsigned char)compact[off + 25]));
                if (!next.addr.sin_port || next.addr.sin_addr.s_addr == 0) continue;
                std::string key = node_key(next.addr);
                if (queried.find(key) != queried.end()) continue;
                bool duplicate = false;
                for (size_t qi = cursor; qi < queue.size(); ++qi)
                    if (node_key(queue[qi].addr) == key) { duplicate = true; break; }
                if (!duplicate) queue.push_back(next);
            }
        }
    }
    close(fd);
    logf_("magnet: DHT queried %u nodes, %u replies, %u peers",
          (unsigned)queried.size(), (unsigned)replies, (unsigned)peers.size());
    if (peers.empty()) {
        error = "DHT: узлы ответили без пиров; попробуй позже или добавь magnet с трекерами";
        return false;
    }
    error.clear();
    return true;
}
