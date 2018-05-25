//
// Created by koncord on 21.05.18.
//

#pragma once

#include <string>
#include <fstream>
#include <boost/crc.hpp>
#include <sstream>

static std::string crc32File(const std::string &f)
{
    std::ifstream file(f, std::ios::binary);

    boost::crc_32_type result;
    if (!file)
        throw std::invalid_argument("Invalid file");

    do
    {
        char buffer[1024];
        file.read(buffer, 1024);
        result.process_bytes(buffer, file.gcount());
    } while (file);

    std::stringstream sstr;
    sstr << std::hex << result.checksum();
    return sstr.str();
}