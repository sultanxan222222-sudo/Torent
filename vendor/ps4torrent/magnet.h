#pragma once
#include <string>
#include <stdint.h>

// Resolve a BitTorrent v1 magnet using HTTP trackers and BEP 10 / ut_metadata peers.
// Returns a complete bencoded .torrent payload only after verifying the info-hash.
bool magnet_resolve(const std::string& uri, const unsigned char peerId[20],
                    std::string& torrentBytes, std::string& error);
