#pragma once
#include <string>
#include <vector>

struct CatalogEntry {
    std::string id;
    std::string title;
    std::string description;
    std::string category;
    std::string platform;
    std::string cover;
    std::string sourceType;
    std::string sourceUri;
};

bool catalog_load();
const std::vector<CatalogEntry>& catalog_items();
const std::string& catalog_last_error();
