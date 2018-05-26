//
// Created by koncord on 26.05.18.
//

#pragma once

#include <boost/system/error_code.hpp>
#include <iostream>

// Report a failure
inline void fail(boost::system::error_code ec, char const *what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}
