//
// Created by koncord on 22.05.18.
//

#include <iostream>

#include <boost/filesystem.hpp>

#include <json.hpp>

#include <components/crc32.hpp>
#include <components/JSONEntry.hpp>
#include "Options.hpp"
#include "Helper.hpp"
#include "Downloader.hpp"
#include "AssetEntry.hpp"
#include "TimeUtils.hpp"

namespace fs = boost::filesystem;

int main_downloader(Options &options)
{
    nlohmann::json cache;
    Helper::loadJson(cache, options.cachePath);

    nlohmann::json &use_server = cache[options.useServer];

    Downloader downloader;

    std::vector<char> buf;
    auto file_list_dl = downloader.downloadToBuf(options.address + "/file_list.json", buf);
    file_list_dl->future()->get();
    auto file_list = nlohmann::json::parse(buf);

    for (nlohmann::json::iterator it = use_server.begin(); it != use_server.end(); ++it)
    {
        if (it.key()[0] != CACHE_KEY_PREFIX_SERIVCE) // skip service data
            continue;
        auto foundIt = file_list.find(it.key());
        if (foundIt != file_list.end() && foundIt.value() != it.value()) // stored file and file on the server are different
        {
            // remove file from the cache
            use_server.erase(it.key());
            Helper::deleteUnusedFile(cache, it.key(), it.value());
        }
    }

    // because downloader can throw exceptions we need to save cache
    Helper::saveJson(cache, options.cachePath);

    std::vector<std::shared_ptr<AssetEntry>> downloadList;

    for (const auto &f : JSONEntry::parse(file_list))
    {
        const std::string &serverAddr = f.first;

        for (const auto &jsonEntry : f.second)
        {
            // convert json entry to asset entry
            auto assetEntry = std::make_shared<AssetEntry>(jsonEntry);
            bool needToDownload = false;

            if (assetEntry->exists())
            {
                // todo: check if file is not fully downloaded and continue downloading
                std::ifstream file(assetEntry->fullpath(), std::ios::binary);
                if (!file)
                    throw std::invalid_argument("Invalid file");
                std::string hash = crc32Stream(file);
                if (hash != jsonEntry.checksum)
                {
                    std::cout << jsonEntry.file << ": the existing file is not valid. File checksum: \""
                              << hash << "\" Valid checksum: " << jsonEntry.checksum << std::endl;
                    needToDownload = true;
                }
            }
            else
            {
                std::cout << jsonEntry.file << ": not exists, adding to download list." << std::endl;
                needToDownload = true;
            }

            if (needToDownload)
            {
                // We need to make sure that target directory is exists before downloading
                fs::create_directories(assetEntry->path());
                auto download = downloader.downloadFile(assetEntry->downloadLink(), assetEntry->fullpath());
                assetEntry->setDownloader(download);
                downloadList.push_back(assetEntry);
            }
        }
    }

    auto fileIt = downloadList.begin();

    auto nextFile = [&fileIt, &downloadList]() {
        if (fileIt != downloadList.end())
            ++fileIt;
        else
            fileIt = downloadList.begin();
    };

    while (!downloadList.empty())
    {
        auto &v = *fileIt;

        switch (v->wait(1))
        {
            case AssetEntry::wait_status::timeout:
                nextFile();
                break;
            case AssetEntry::wait_status::fail:
                return 1;
        }

        std::string newHash;
        if (v->validateFile(&newHash))
            std::cout << v->fullpath() << ": downloaded" << std::endl;
        else
        {
            fs::remove_all(v->path());
            Helper::saveJson(cache, options.cachePath);
            if (!newHash.empty())
                throw std::runtime_error("Downloaded file invalid. File checksum: \""
                                         + newHash + "\" Valid checksum: " + v->hash());
        }

        // after downloading file it's safe to add it to list
        use_server[v->key()] = v->hash();

        fileIt = downloadList.erase(fileIt);
    }

    use_server[CACHE_KEY_TIMESTAMP] = TimeUtil::timestamp();
    Helper::saveJson(cache, options.cachePath);

    return 0;
}
