//
// Created by koncord on 22.05.18.
//

#pragma once

#include <string>
#include <json.hpp>

#define USE_LIST_KEY_PREFIX_SERIVCE '>'
#define USE_LIST_KEY_TIMESTAMP ">last_connect_timestamp"

class Helper
{
public:
    static bool deleteUnusedFile(const nlohmann::json &use_list, const std::string &path, const std::string &hash);
    static bool deleteOldServers(nlohmann::json &use_list, long stamp);

    static void saveJson(const nlohmann::json &use_list, const std::string &file);
    static void loadJson(nlohmann::json &use_list, const std::string &file);
};
