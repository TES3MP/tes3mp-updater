//
// Created by koncord on 19.05.18.
//

#include "AssetEntry.hpp"

#include <iostream>
#include <fstream>

#include <boost/filesystem.hpp>

#include "components/crc32.hpp"

std::string AssetEntry::dataPath;
std::string AssetEntry::hostPath;

AssetEntry::AssetEntry(const std::string &path, const std::string &hash) :
        _hash(hash), _key(path)
{
    size_t pos = path.find_last_of('/');
    if (pos != path.npos)
    {
        _filename = path.substr(pos + 1);
        this->_path = path.substr(0, pos);
    }
    else
        _filename = path;
}

std::string AssetEntry::key() const
{
    return _key;
}

std::string AssetEntry::path() const
{
    std::string result = dataPath + '/';
    if (!_path.empty())
        result += _path + '/';
    result += _filename + '/' + _hash;
    return result;
}

std::string AssetEntry::fullpath() const
{
    return path() + '/' + _filename;
}

std::string AssetEntry::hash() const
{
    return _hash;
}

bool AssetEntry::exists() const
{
    return boost::filesystem::exists(fullpath());
}

std::string AssetEntry::downloadLink() const
{
    std::string result = hostPath + '/';
    if (!_path.empty())
        result += _path + '/';
    result += _filename;
    return result;
}

void AssetEntry::setDataPath(const std::string &str)
{
    dataPath = str;
}

void AssetEntry::setHostPath(const std::string &str)
{
    hostPath = str;
}

void AssetEntry::setDownloader(const std::shared_ptr<FileDownloader> &dl)
{
    downloader = dl;
}

std::shared_ptr<FileDownloader> AssetEntry::getDownloader()
{
    return downloader;
}

AssetEntry::wait_status AssetEntry::wait(int sec)
{
    try
    {
        switch (downloader->future()->wait_for(std::chrono::seconds(sec)))
        {

            case std::future_status::ready:
                return AssetEntry::wait_status::ready;
            case std::future_status::timeout:
                return AssetEntry::wait_status::timeout;
        }
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
    return AssetEntry::wait_status::fail;
}

bool AssetEntry::validateFile(std::string *newHash)
{
    try
    {
        // probably file is safe to read after wait returns ready,
        // but for cases when validateFile called without it needs to recheck
        downloader->future()->get();
    }
    catch (boost::system::system_error &e)
    {
        std::cerr << "Error (" << e.code() << "): " << e.what() << std::endl;
        return false;
    }
    catch (std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }

    // maybe throwing exception there isn't good idea
    std::ifstream file(fullpath(), std::ios::binary);
    if (!file)
        throw std::invalid_argument("Invalid file");
    std::string _newHash = crc32Stream(file);
    if (_newHash != _hash)
    {
        if(newHash != nullptr)
            *newHash = _newHash;
        return false;
    }

    return true;
}
