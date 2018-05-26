//
// Created by koncord on 26.05.18.
//

#include "Utils.hpp"

#include <sstream>

boost::beast::string_view Utils::mimeType(boost::beast::string_view path)
{
    using boost::beast::iequals;
    auto const pos = path.rfind(".");
    if (pos != boost::beast::string_view::npos)
    {
        if (iequals(path.substr(pos), ".htm")) return "text/html";
        if (iequals(path.substr(pos), ".html")) return "text/html";
        if (iequals(path.substr(pos), ".json")) return "application/json";
    }
    return "application/octet-stream";
}

std::string Utils::pathCat(boost::beast::string_view base, boost::beast::string_view path)
{
    if (base.empty())
        return path.to_string();
    std::string result = base.to_string();
#if BOOST_MSVC
    char constexpr path_separator = '\\';
        if(result.back() == path_separator)
            result.resize(result.size() - 1);
        result.append(path.data(), path.size());
        for(auto& c : result)
            if(c == '/')
                c = path_separator;
#else
    char constexpr path_separator = '/';
    if (result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
#endif
    return result;
}

std::string Utils::decodeURL(boost::beast::string_view url)
{
    std::string result;
    boost::beast::string_view::const_iterator beginBlock = url.begin();
    for (auto it = url.begin(); it != url.end(); ++it)
    {
        if (*it == '%')
        {
            result.append(beginBlock, it);
            char v[2]{*(++it), *(++it)};
            unsigned char value = hexToInt(v); // todo: validate URI value
            result.push_back(value);
            beginBlock = ++it;
        }
    }

    if (result.empty())
        return url.to_string();
    else
    {
        if (beginBlock != url.end())
            result.append(beginBlock, url.end());
        return result;
    }
}

unsigned char Utils::hexToInt(boost::beast::string_view hex)
{
    std::stringstream sstr;
    int val;
    sstr << std::hex << hex;
    sstr >> val;
    return (unsigned char) val;
}
