//
// Created by koncord on 28.05.18.
//

#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <json.hpp>

struct JSONEntry
{
    JSONEntry() {};
    JSONEntry(const std::string &file,
              const std::string &checksum,
              long fsize) : file(file), checksum(checksum), fsize(fsize) {}

    std::string file;
    std::string checksum;
    long fsize;

    static std::vector<JSONEntry> parseServer(const nlohmann::json &server);
    static std::unordered_map<std::string, std::vector<JSONEntry>> parse(const nlohmann::json &file_list);
};
