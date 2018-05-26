//
// Created by koncord on 26.05.18.
//

#pragma once

#include <memory>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>

#include "Reporter.hpp"
#include "Session.hpp"


// Accepts incoming connections and launches the sessions
class Listener : public std::enable_shared_from_this<Listener>
{
    boost::asio::ip::tcp::acceptor acceptor;
    boost::asio::ip::tcp::socket _socket;
    std::shared_ptr<std::string const> doc_root;

public:
    Listener(boost::asio::io_context &ioc,
             boost::asio::ip::tcp::endpoint endpoint,
             std::shared_ptr<std::string const> const &doc_root,
             int backlog)
            : acceptor(ioc), _socket(ioc), doc_root(doc_root)
    {
        boost::system::error_code ec;

        acceptor.open(endpoint.protocol(), ec);
        if (ec)
        {
            fail(ec, "open");
            return;
        }

        // Allow address reuse
        acceptor.set_option(boost::asio::socket_base::reuse_address(true));
        if (ec)
        {
            fail(ec, "set_option");
            return;
        }

        // Bind to the server address
        acceptor.bind(endpoint, ec);
        if (ec)
        {
            fail(ec, "bind");
            return;
        }

        if(backlog <= 0 || backlog > boost::asio::socket_base::max_listen_connections)
            backlog = boost::asio::socket_base::max_listen_connections;

        // Start listening for connections
        acceptor.listen(backlog, ec);
        if (ec)
        {
            fail(ec, "listen");
            return;
        }
    }

    // Start accepting incoming connections
    void run()
    {
        if (!acceptor.is_open())
            return;
        do_accept();
    }

    void do_accept()
    {
        acceptor.async_accept(_socket, std::bind(&Listener::on_accept, shared_from_this(), std::placeholders::_1));
    }

    void on_accept(boost::system::error_code ec)
    {
        if (ec)
            fail(ec, "accept");
        else // Create the Session and run it
            std::make_shared<Session>(std::move(_socket), doc_root)->run();

        // Accept another connection
        do_accept();
    }

    static void spawn(boost::asio::io_context &ioc,
                      const boost::asio::ip::address &address, unsigned short port,
                      boost::beast::string_view dataRoot,
                      int maxClients)
    {
        std::make_shared<Listener>(ioc,
                                   boost::asio::ip::tcp::endpoint{address, port},
                                   std::make_shared<std::string>(dataRoot),
                                   maxClients)->run();
    }
};
