//
// Created by koncord on 22.05.18.
//

#include <boost/filesystem.hpp>

#include "Helper.hpp"
#include "AssetEntry.hpp"

namespace fs = boost::filesystem;

// todo: check the cache so that it doesn't contain ".." to avoid deleting invalid files (attack on the system)
bool Helper::deleteUnusedFile(const nlohmann::json &cache, const std::string &path, const std::string &hash)
{
    for (const auto &serv : cache)
    {
        auto foundIt = serv.find(path);
        if (foundIt != serv.end() && foundIt.value() == hash)
            return false; // file is used by servers
    }

    AssetEntry entry(path, hash);
    return fs::remove_all(entry.path()) > 0;
}

bool Helper::deleteOldServers(nlohmann::json &cache, long stamp)
{
    bool result = false;
    for (nlohmann::json::iterator it = cache.begin(); it != cache.end();)
    {
        if (it.value()[CACHE_KEY_TIMESTAMP] < stamp)
        {
            it = cache.erase(it);
            result = true;
        }
        else
            ++it;
    }
    return result;
}

void Helper::saveJson(const nlohmann::json &cache, const std::string &file)
{
    std::ofstream cache_file(file);
    cache_file << std::setw(4) << cache;
    cache_file.close();
}

void Helper::loadJson(nlohmann::json &cache, const std::string &file)
{
    std::ifstream cache_file(file);
    cache_file >> cache;
}
