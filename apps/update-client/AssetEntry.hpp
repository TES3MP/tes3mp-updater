//
// Created by koncord on 19.05.18.
//

#pragma once

#include <string>

#include "FileDownloader.hpp"

class AssetEntry
{
public:
    AssetEntry(const std::string &path, const std::string &hash);
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
    static void setHostPath(const std::string &str);

private:
    std::string _hash;
    std::string _key;
    std::string _path;
    std::string _filename;

    static std::string dataPath;
    static std::string hostPath;

    std::shared_ptr<FileDownloader> downloader;
};

