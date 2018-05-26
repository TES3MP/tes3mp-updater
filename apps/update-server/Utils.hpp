//
// Created by koncord on 26.05.18.
//

#pragma once

#include <boost/beast/core/string.hpp>

class Utils
{
public:
    static unsigned char hexToInt(boost::beast::string_view hex);
    static std::string decodeURL(boost::beast::string_view url);

    // Append an HTTP rel-path to a local filesystem path.
    // The returned path is normalized for the platform.
    static std::string pathCat(boost::beast::string_view base, boost::beast::string_view path);
    static boost::beast::string_view mimeType(boost::beast::string_view path);
};