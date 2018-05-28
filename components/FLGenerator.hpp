//
// Created by koncord on 26.05.18.
//

#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <future>

#include <json.hpp>

#include "JSONEntry.hpp"

class FLGenerator: public std::enable_shared_from_this<FLGenerator>
{
public:
    typedef std::future<JSONEntry> FutureEntry;
    typedef std::vector<std::shared_ptr<FutureEntry>> JSONEntries;
    FLGenerator(const std::string &dataPath, const std::string &defaultServer);
    ~FLGenerator();

    void start();

    nlohmann::json getJSON();

private:
    void doRun();
    JSONEntry doFile(const std::string &path);
    std::future<void> getJSON_internal();

    // Start thread
    void runThread();
    // Wait until thread finishes inserting asynchronous operations, but filling entries may not be finished by this time
    bool waitThread();

private:
    std::unordered_map<std::string, JSONEntries> entries;
    std::string dataPath;

    std::thread thread;
    std::atomic_bool threadFinished;
    std::shared_future<void> future;
    nlohmann::json file_list;
};


