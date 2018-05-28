//
// Created by koncord on 19.05.18.
//

#pragma once

#include <string>

#include <components/JSONEntry.hpp>

#include "FileDownloader.hpp"

class AssetEntry
{
public:
    AssetEntry(const JSONEntry &entry, const std::string &host = "");
    AssetEntry(const std::string &path, const std::string &hash, const std::string &host = "", long fsize = -1);

    bool exists() const;
    std::string downloadLink() const;
    std::string fullpath() const;
    std::string path() const;

    std::string hash() const;
    std::string key() const;

    void setDownloader(const std::shared_ptr<FileDownloader> &dl);
    std::shared_ptr<FileDownloader> getDownloader();
    bool validateFile(std::string *newHash = nullptr);

    enum class wait_status
    {
        ready,
        timeout,
        fail
    };
    wait_status wait(int sec);

    static void setDataPath(const std::string &str);

private:
    std::string _hash;
    std::string _key;
    std::string _path;
    std::string _filename;
    long fsize;

    static std::string dataPath;
    std::string hostPath;

    std::shared_ptr<FileDownloader> downloader;
};

