#pragma once
#include <string>
#include <stdint.h>

// Resolve BitTorrent v1 magnets using HTTP/UDP trackers and IPv4 DHT (BEP 5),
// then obtain BEP 10/ut_metadata pieces and verify the info-hash.
bool magnet_resolve(const std::string& uri, const unsigned char peerId[20],
                    std::string& torrentBytes, std::string& error);
