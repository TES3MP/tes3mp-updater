//
// Created by koncord on 26.05.18.
//

#pragma once

#include <memory>

#include <boost/beast/http/message.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>

#include "Reporter.hpp"
#include "RequestHandler.hpp"

class FLGenerator;
// Handles an HTTP server connection
class Session : public std::enable_shared_from_this<Session>
{
    // The function object is used to send an HTTP message.
    struct send_lambda
    {
        Session &self;

        explicit send_lambda(Session &self) : self(self) {}

        template<bool isRequest, class Body, class Fields>
        void operator()(boost::beast::http::message<isRequest, Body, Fields> &&msg) const
        {
            // The lifetime of the message has to extend for the duration of the async operation so
            // we use a shared_ptr to manage it.
            auto sp = std::make_shared<boost::beast::http::message<isRequest, Body, Fields>>(std::move(msg));

            // Store a type-erased version of the shared pointer in the class to keep it alive.
            self.res = sp;

            // Write the response
            boost::beast::http::async_write(self._socket, *sp,
                              boost::asio::bind_executor(self.strand,
                                                         std::bind(&Session::on_write, self.shared_from_this(),
                                                                   std::placeholders::_1, std::placeholders::_2,
                                                                   sp->need_eof())
                              )
            );
        }
    };

    boost::asio::ip::tcp::socket _socket;
    boost::asio::strand<boost::asio::io_context::executor_type> strand;
    boost::beast::flat_buffer buffer;
    std::shared_ptr<std::string const> doc_root;
    boost::beast::http::request<boost::beast::http::string_body> req;
    std::shared_ptr<void> res;
    send_lambda lambda;
    std::shared_ptr<FLGenerator> flGen;

public:
    explicit Session(boost::asio::ip::tcp::socket socket,
                     std::shared_ptr<std::string const> const &doc_root,
                     const std::shared_ptr<FLGenerator> &flGen)
            : _socket(std::move(socket)), strand(_socket.get_executor()), doc_root(doc_root), lambda(*this), flGen(flGen) {}

    // Start the asynchronous operation
    void run()
    {
        do_read();
    }

    void do_read()
    {
        // Make the request empty before reading, otherwise the operation behavior is undefined.
        req = {};

        boost::beast::http::async_read(_socket, buffer, req,
                         boost::asio::bind_executor(strand, std::bind(&Session::on_read, shared_from_this(),
                                                                      std::placeholders::_1, std::placeholders::_2)
                         )
        );
    }

    void on_read(boost::system::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // This means they closed the connection
        if (ec == boost::beast::http::error::end_of_stream)
            return do_close();

        if (ec)
            return fail(ec, "read");

        // Send the response
        handle_request(*doc_root, std::move(req), lambda, flGen);
    }

    void on_write(boost::system::error_code ec, std::size_t bytes_transferred, bool close)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "write");

        if (close)
        {
            // We should close the connection, usually because the response indicated
            // the "Connection: close" semantic.
            return do_close();
        }

        // We're done with the response so delete it
        res = nullptr;

        // Read another request
        do_read();
    }

    void do_close()
    {
        // Send a TCP shutdown
        boost::system::error_code ec;
        _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
    }
};
