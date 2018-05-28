//
// Created by koncord on 28.05.18.
//

#include <iostream>
#include "JSONEntry.hpp"

std::vector<JSONEntry> JSONEntry::parseServer(const nlohmann::json &server)
{
    std::vector<JSONEntry> files;

    for (auto it2 = server.begin(); it2 != server.end(); ++it2)
    {
        const auto &val = it2.value();
        files.emplace_back(val["file"], val["checksum"], val["fsize"]);
    }

    return files;
}

std::unordered_map<std::string, std::vector<JSONEntry>> JSONEntry::parse(const nlohmann::json &file_list)
{
    std::unordered_map<std::string, std::vector<JSONEntry>> entries;

    for (auto it = file_list.begin(); it != file_list.end(); ++it)
        entries[it.key()] = parseServer(it.value());

    return entries;
}
