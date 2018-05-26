//
// Created by koncord on 26.05.18.
//

#pragma once

#include <tuple>

#include <boost/beast/core/string.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include "Utils.hpp"

// This function produces an HTTP response for the given request. The type of the response object depends on the
// contents of the request, so the interface requires the caller to pass a generic lambda for receiving the response.
template<class Body, class Allocator, class Send>
void handle_request(boost::beast::string_view doc_root,
                    boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req,
                    Send &&send)
{

    auto const response_msg = [&req] (boost::beast::string_view message, boost::beast::http::status status = boost::beast::http::status::ok) {
        boost::beast::http::response<boost::beast::http::string_body> res{status, req.version()};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = message.to_string();
        res.prepare_payload();
        return res;
    };

    // Returns a bad request response
    auto const bad_request = [&req, response_msg](boost::beast::string_view why) {
        return response_msg(why.to_string(), boost::beast::http::status::bad_request);
    };

    // Returns a not found response
    auto const not_found = [&req, response_msg](boost::beast::string_view target) {
        return response_msg("The resource '" + target.to_string() + "' was not found.", boost::beast::http::status::not_found);
    };

    // Returns a server error response
    auto const server_error = [&req, response_msg](boost::beast::string_view what) {
        return response_msg("An error occurred: '" + what.to_string() + "'", boost::beast::http::status::internal_server_error);
    };

    // Make sure we can handle the method
    if (req.method() != boost::beast::http::verb::get &&
        req.method() != boost::beast::http::verb::head)
        return send(bad_request("Unknown HTTP-method"));

    // Request path must be absolute and not contain "..".
    if (req.target().empty() || req.target()[0] != '/' ||
        req.target().find("..") != boost::beast::string_view::npos)
        return send(bad_request("Illegal request-target"));

    // Build the path to the requested file
    std::string path = Utils::pathCat(doc_root, req.target());
    if (req.target().back() == '/')
    {
        //path.append("index.html");
        return send(response_msg("<h1>Welcome to the tes3mp update server</h1>"));
    }

    // Attempt to open the file
    boost::beast::error_code ec;
    boost::beast::http::file_body::value_type body;
    body.open(Utils::decodeURL(path).c_str(), boost::beast::file_mode::scan, ec);

    std::cout << path << std::endl;

    // Handle the case where the file doesn't exist
    if (ec == boost::system::errc::no_such_file_or_directory)
        return send(not_found(req.target()));

    // Handle an unknown error
    if (ec)
        return send(server_error(ec.message()));

    // Cache the size since we need it after the move
    auto const size = body.size();

    // Respond to HEAD request
    if (req.method() == boost::beast::http::verb::head)
    {
        boost::beast::http::response<boost::beast::http::empty_body> res{boost::beast::http::status::ok, req.version()};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, Utils::mimeType(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return send(std::move(res));
    }

    // Respond to GET request
    boost::beast::http::response<boost::beast::http::file_body> res {
            std::piecewise_construct,
            std::make_tuple(std::move(body)), std::make_tuple(boost::beast::http::status::ok, req.version())
    };
    res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(boost::beast::http::field::content_type, Utils::mimeType(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return send(std::move(res));
}
