//
// Created by koncord on 21.05.18.
//

#pragma once

#include <string>
#include <iostream>
#include <boost/crc.hpp>
#include <sstream>

static std::string crc32Stream(std::istream &dataStream)
{
    boost::crc_32_type result;

    do
    {
        char buffer[1024];
        dataStream.read(buffer, 1024);
        result.process_bytes(buffer, dataStream.gcount());
    } while (dataStream);

    std::stringstream sstr;
    sstr << std::hex << result.checksum();
    return sstr.str();
}
