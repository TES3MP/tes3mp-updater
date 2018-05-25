//
// Created by koncord on 22.05.18.
//

#include <iostream>

#include <boost/filesystem.hpp>

#include <json.hpp>

#include <components/crc32.hpp>
#include "Options.hpp"
#include "Helper.hpp"
#include "Downloader.hpp"
#include "AssetEntry.hpp"
#include "TimeUtils.hpp"

namespace fs = boost::filesystem;

int main_downloader(Options &options)
{
    nlohmann::json use_list;
    Helper::loadJson(use_list, options.useListPath);

    nlohmann::json &use_server = use_list[options.useServer];

    Downloader downloader;

    std::vector<char> buf;
    auto file_list_dl = downloader.downloadToBuf(options.address + "/file_list.json", buf);
    file_list_dl->future()->get();
    auto file_list = nlohmann::json::parse(buf);

    for (nlohmann::json::iterator it = use_server.begin(); it != use_server.end(); ++it)
    {
        if (it.key()[0] != USE_LIST_KEY_PREFIX_SERIVCE) // skip service data
            continue;
        auto foundIt = file_list.find(it.key());
        if (foundIt != file_list.end() && foundIt.value() != it.value()) // stored file and file on the server are different
        {
            // remove file from the use_list
            use_server.erase(it.key());
            Helper::deleteUnusedFile(use_list, it.key(), it.value());
        }
    }

    // because downloader can throw exceptions we need to save use_list
    Helper::saveJson(use_list, options.useListPath);

    std::vector<std::shared_ptr<AssetEntry>> downloadList;

    for (auto it = file_list.begin(); it != file_list.end(); ++it)
    {
        auto entry = std::make_shared<AssetEntry>(it.key(), it.value());
        bool needToDownload = false;

        if (entry->exists())
        {
            // todo: check if file is not fully downloaded and continue downloading
            std::string hash = crc32File(entry->fullpath());
            if (hash != it.value())
            {
                std::cout << it.key() << ": the existing file is not valid. File checksum: \""
                          << hash << "\" Valid checksum: " << it.value() << std::endl;
                needToDownload = true;
            }
        }
        else
        {
            std::cout << it.key() << ": not exists, adding to download list." << std::endl;
            needToDownload = true;
        }

        if (needToDownload)
        {
            // We need to make sure that target directory is exists before downloading
            fs::create_directories(entry->path());
            auto download = downloader.downloadFile(entry->downloadLink(), entry->fullpath());
            entry->setDownloader(download);
            downloadList.push_back(entry);
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
            Helper::saveJson(use_list, options.useListPath);
            if (!newHash.empty())
                throw std::runtime_error("Downloaded file invalid. File checksum: \""
                                         + newHash + "\" Valid checksum: " + v->hash());
        }

        // after downloading file it's safe to add it to list
        use_server[v->key()] = v->hash();

        fileIt = downloadList.erase(fileIt);
    }

    use_server[USE_LIST_KEY_TIMESTAMP] = TimeUtil::timestamp();
    Helper::saveJson(use_list, options.useListPath);

    return 0;
}
