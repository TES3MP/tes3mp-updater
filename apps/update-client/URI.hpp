//
// Created by koncord on 21.05.18.
//

#pragma once

#include <string>

struct URI
{
    std::string scheme, host, path, port;

    URI(const std::string &url) // NOLINT
    {
        size_t schemePos = url.find("://");
        size_t hostPos;

        if (schemePos != std::string::npos)
        {
            scheme = url.substr(0, schemePos);
            hostPos = url.find('/', schemePos + 3);
        }
        else
        {
            schemePos = 0;
            scheme = "http";
            hostPos = url.find('/');
        }

        if (hostPos != std::string::npos)
        {
            size_t portPos = url.find(':');
            if (portPos != std::string::npos)
            {
                host = url.substr(schemePos, portPos);
                port = url.substr(portPos + 1, hostPos - portPos - 1);
                path = url.substr(hostPos);
            }
            else
            {
                host = url.substr(schemePos, hostPos);
                path = url.substr(hostPos);
            }
        }
        else
            host = url;
    }

    /**
     * replaces space symbols to hex values
     * @return encoded path
     */
    std::string getEncodedPath() const
    {
        std::string result = path;
        while (true)
        {
            std::string::iterator f = std::find_if(result.begin(), result.end(), [](const auto &c) { return isspace(c); });
            if (f != result.end())
            {
                std::stringstream sstr;
                sstr << '%' << std::hex << (int) *f;
                result.replace(f, f + 1, sstr.str());
            }
            else
                break;
        }
        return result;
    }
};
