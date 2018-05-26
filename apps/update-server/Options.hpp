//
// Created by koncord on 26.05.18.
//

#pragma once

#include <string>

struct Options
{
    std::string cfg;
    std::string dataPath;

    static bool parseOptions(int argc, char **argv, Options &options);
};


