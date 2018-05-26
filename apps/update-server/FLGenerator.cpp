//
// Created by koncord on 26.05.18.
//

#include <components/crc32.hpp>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include "FLGenerator.hpp"

namespace fs = boost::filesystem;

FLGenerator::FLGenerator(const std::string &dataPath, const std::string &defaultServer) : dataPath(dataPath), threadFinished(0)
{
    entries[defaultServer] = JSONEntries(0);
}

FLGenerator::~FLGenerator()
{
    /*if (thread.joinable())
        thread.join();*/
}

JSONEntry FLGenerator::doFile(const std::string &path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw std::invalid_argument("Invalid file");

    JSONEntry entry;

    file.seekg(0, file.end);
    entry.fsize = file.tellg();
    file.seekg(0, file.beg);

    entry.checksum = crc32Stream(file);

    entry.file = path.substr(dataPath.size() + 1);

    return std::move(entry);
}

void FLGenerator::runThread()
{
    thread = std::thread(&FLGenerator::doRun, shared_from_this());
}

void FLGenerator::doRun()
{
    if (threadFinished)
        return;

    JSONEntries &set = entries.begin()->second;

    fs::recursive_directory_iterator end;
    for (auto it = fs::recursive_directory_iterator(fs::path(dataPath)); it != end; ++it)
    {
        if (!fs::is_directory(it->path()))
        {
            auto entry = std::async(&FLGenerator::doFile, shared_from_this(), it->path().string());
            set.push_back(std::make_shared<FutureEntry>(std::move(entry)));
        }
    }

    threadFinished = true;
}

bool FLGenerator::waitThread()
{
    if (thread.joinable())
    {
        thread.join();
        return true;
    }

    return false;
}

nlohmann::json FLGenerator::getJSON()
{
    runThread();

    if (!waitThread())
        throw std::runtime_error("Internal error");

    JSONEntries vec = entries.begin()->second;
    JSONEntries::iterator it = vec.begin();

    nlohmann::json obj;
    auto &entry = obj[entries.begin()->first];

    while (!vec.empty())
    {
        const std::shared_ptr<FutureEntry> &en = *it;

        if (en->wait_for(std::chrono::milliseconds(500)) == std::future_status::timeout)
        {
            if (std::next(it) == vec.end())
                it = vec.begin();
            else
                ++it;
            continue;
        }

        JSONEntry val = en->get();

        nlohmann::json e;
        e["fsize"] = val.fsize;
        e["checksum"] = val.checksum;
        entry[val.file] = e;

        it = vec.erase(it);

        if (it == vec.end())
            it = vec.begin();
    }

    return obj;
}