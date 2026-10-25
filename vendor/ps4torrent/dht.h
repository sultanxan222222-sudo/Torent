#pragma once
#include <string>
#include <vector>
#include "tracker.h"

// BEP 5 IPv4 DHT get_peers lookup for a BitTorrent v1 info-hash.
bool dht_get_peers(const unsigned char infoHash[20], const unsigned char peerId[20],
                   std::vector<Peer>& peers, std::string& error);
