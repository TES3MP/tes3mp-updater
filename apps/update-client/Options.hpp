//
// Created by koncord on 22.05.18.
//

#pragma once

#include <string>

struct Options
{
    std::string address;
    std::string downloadPath;
    std::string useListPath;
    std::string useServer;
    enum Mode {
        DOWNLOADER = 0x1,
        CLEANER = 0x2
    };

    long timestamp;
    unsigned mode;

    static bool parseOptions(int argc, char **argv, Options &options);
};
