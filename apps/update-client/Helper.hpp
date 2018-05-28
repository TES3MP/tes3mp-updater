//
// Created by koncord on 22.05.18.
//

#pragma once

#include <string>
#include <json.hpp>

#define CACHE_KEY_PREFIX_SERIVCE '>'
#define CACHE_KEY_TIMESTAMP ">last_connect_timestamp"

class Helper
{
public:
    static bool deleteUnusedFile(const nlohmann::json &cache, const std::string &path, const std::string &hash);
    static bool deleteOldServers(nlohmann::json &cache, long stamp);

    static void saveJson(const nlohmann::json &cache, const std::string &file);
    static void loadJson(nlohmann::json &cache, const std::string &file);
};
