//
// Created by koncord on 22.05.18.
//

#include <boost/filesystem.hpp>

#include "Helper.hpp"
#include "AssetEntry.hpp"

namespace fs = boost::filesystem;

// todo: check the use_list so that it doesn't contain ".." to avoid deleting invalid files (attack on the system)
bool Helper::deleteUnusedFile(const nlohmann::json &use_list, const std::string &path, const std::string &hash)
{
    for (const auto &serv : use_list)
    {
        auto foundIt = serv.find(path);
        if (foundIt != serv.end() && foundIt.value() == hash)
            return false; // file is used by servers
    }

    AssetEntry entry(path, hash);
    return fs::remove_all(entry.path()) > 0;
}

bool Helper::deleteOldServers(nlohmann::json &use_list, long stamp)
{
    bool result = false;
    for (nlohmann::json::iterator it = use_list.begin(); it != use_list.end();)
    {
        if (it.value()[USE_LIST_KEY_TIMESTAMP] < stamp)
        {
            it = use_list.erase(it);
            result = true;
        }
        else
            ++it;
    }
    return result;
}

void Helper::saveJson(const nlohmann::json &use_list, const std::string &file)
{
    std::ofstream use_list_file(file);
    use_list_file << std::setw(4) << use_list;
    use_list_file.close();
}

void Helper::loadJson(nlohmann::json &use_list, const std::string &file)
{
    std::ifstream use_list_file(file);
    use_list_file >> use_list;
}
