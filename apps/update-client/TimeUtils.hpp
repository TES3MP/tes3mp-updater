//
// Created by koncord on 22.05.18.
//

#pragma once

#include <string>
#include <chrono>
#include <iomanip>

struct TimeUtil
{

    /**
    *
    * @param timePointStr YYYY-mm-dd HH:MM:SS
    * @return
    */
    static long timePointFromString(const std::string &timePointStr)
    {
        using namespace std::chrono;
        std::tm tm = {};
        std::stringstream sstr(timePointStr);
        sstr >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        auto dur = system_clock::from_time_t(std::mktime(&tm)).time_since_epoch();
        return duration_cast<seconds>(dur).count();
    }

    static long timestamp()
    {
        using namespace std::chrono;
        auto dur = system_clock::now().time_since_epoch();
        return duration_cast<seconds>(dur).count();
    }
};
