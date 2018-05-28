//
// Created by koncord on 22.05.18.
//

#include <set>
#include <iostream>

#include <json.hpp>

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

#include "Options.hpp"
#include "Helper.hpp"

namespace fs = boost::filesystem;

std::set<fs::path> getFiles(const fs::path &path)
{
    fs::recursive_directory_iterator it(path);
    fs::recursive_directory_iterator itEnd;
    std::set<fs::path> ret;

    while(it != itEnd)
    {
        if (!fs::is_directory(*it))
            ret.emplace(*it);
        ++it;
    }

    return ret;
}

void removeEmptyDirs(const fs::path &path)
{
    if (!fs::is_directory(path))
        return;

    fs::directory_iterator it(path);
    std::stack<fs::directory_iterator> itStack;
    fs::directory_iterator end;

    auto popDir = [&it, &itStack]() {
        if (itStack.empty())
            return;
        it = fs::directory_iterator(itStack.top());
        itStack.pop();
    };

    auto pushDir = [&it, &itStack]() {
        itStack.push(it);
        it = fs::directory_iterator(*it);
    };

    while (it != end)
    {
        if (!fs::is_directory(*it))
        {
            popDir();
            ++it;
            continue;
        }

        if (!fs::is_empty(*it))
        {
            pushDir();
            continue;
        }

        fs::remove_all(*it);
        std::cout << *it << " empty directory removed." << std::endl;
        if (++it == end)
        {
            popDir();
            continue;
        }
    }
}

int main_cleaner(Options &options)
{
    nlohmann::json cache;
    Helper::loadJson(cache, options.cachePath);

    Helper::deleteOldServers(cache, options.timestamp);
    Helper::saveJson(cache, options.cachePath);

    fs::path path(options.downloadPath);
    std::set<fs::path> pathes = getFiles(path);
    pathes.erase(options.cachePath); // remove cache.json from the list

    for (const auto &entry : pathes)
    {
        // todo: validate hash and path
        std::string hash = entry.parent_path().filename().string();
        std::string path = fs::relative(entry.parent_path().parent_path(), options.downloadPath).string();

        if (Helper::deleteUnusedFile(cache, path, hash))
            std::cout << "unused file: " << path << " with hash \"" << hash << "\" deleted" << std::endl;
    }

    for (const auto &entry : boost::make_iterator_range(fs::directory_iterator(path), {}))
        removeEmptyDirs(entry);
    return 0;
}
