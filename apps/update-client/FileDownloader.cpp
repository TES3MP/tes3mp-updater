//
// Created by koncord on 06.01.18.
//

#include "FileDownloader.hpp"

#include <iostream>
#include <iomanip>

#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/connect.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/beast/version.hpp>

#include "URI.hpp"

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;
namespace ssl = boost::asio::ssl;


FileDownloader::FileDownloader(boost::asio::io_service &ios,
                               const std::shared_ptr<std::ostream> &stream,
                               size_t bufferSize) :
        ioservice(ios), resolv(ios), socket(ios), chunkSize(bufferSize), stream(stream), buffer(nullptr)
{
    _future = promise.get_future();
}

FileDownloader::~FileDownloader()
{
    delete[] buffer;
}

std::shared_ptr<FileDownloader> FileDownloader::downloadToFile(boost::asio::io_service &ioservice,
                                                               const std::string &url,
                                                               const std::string &path,
                                                               size_t bufferSize)
{
    auto dl = std::make_shared<FileDownloader>(ioservice, std::make_shared<std::ofstream>(path, std::ios::binary), bufferSize);
    dl->start(url);
    return dl;
}

std::shared_ptr<FileDownloader> FileDownloader::downloadToBuf(boost::asio::io_service &ioservice,
                                                              const std::string &url,
                                                              std::vector<char> &buf,
                                                              size_t chunkSize)
{
    namespace streams = boost::iostreams;
    using vector_inserter = streams::back_insert_device<std::vector<char>>;
    auto _stream = std::make_shared<streams::stream<vector_inserter>>(vector_inserter(buf));
    auto dl = std::make_shared<FileDownloader>(ioservice, _stream, chunkSize);
    dl->start(url);
    return dl;
}

void FileDownloader::initSSL()
{
    ssl::context ctx{ssl::context::tlsv12_client};
    sslStream = std::make_unique<ssl::stream<tcp::socket &>>(socket, ctx);
    // todo: get SSL cert from launch arguments
    sslStream->set_verify_mode(/*ssl::verify_fail_if_no_peer_cert*/ssl::verify_none);
    try
    {
        sslStream->handshake(ssl::stream_base::client);
    } catch (...)
    {
        promise.set_exception(std::current_exception());
    }
}

void FileDownloader::start(boost::beast::string_view url)
{
    URI uri = url.to_string();
    if (uri.scheme == "https")
        initSSL();

    std::string service;
    if (uri.port.empty())
    {
        if (!uri.scheme.empty())
            service = uri.scheme;
        else
            service = "http";
    }
    else
        service = uri.port;

    req.version(11);
    req.method(http::verb::get);
    //req.keep_alive(true);
    req.target(uri.getEncodedPath());
    req.set(http::field::host, uri.host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    tcp::resolver::query query(uri.host, service);
    resolv.async_resolve(query,
                         std::bind(
                                 &FileDownloader::on_resolve,
                                 shared_from_this(),
                                 std::placeholders::_1,
                                 std::placeholders::_2
                         ));
}

void FileDownloader::on_resolve(const boost::system::error_code &ec, const tcp::resolver::results_type &results)
{
    if (ec)
    {
        promise.set_exception(std::make_exception_ptr(boost::system::system_error(ec)));
        return;
    }

    boost::asio::async_connect(socket, results.begin(), results.end(),
                               std::bind(
                                       &FileDownloader::on_connect,
                                       shared_from_this(),
                                       std::placeholders::_1
                               ));
}

void FileDownloader::on_connect(const boost::system::error_code &ec)
{
    if (ec)
    {
        promise.set_exception(std::make_exception_ptr(boost::system::system_error(ec)));
        return;
    }

    auto handler = std::bind(
            &FileDownloader::on_request_sent,
            shared_from_this(),
            std::placeholders::_1,
            std::placeholders::_2);

    if (sslStream)
        http::async_write(*sslStream, req, handler);
    else
        http::async_write(socket, req, handler);
}

void FileDownloader::on_request_sent(const boost::system::error_code &ec, std::size_t bytes_transferred)
{
    if (ec)
    {
        promise.set_exception(std::make_exception_ptr(boost::system::system_error(ec)));
        return;
    }

    auto handler = std::bind(
            &FileDownloader::on_header_read,
            shared_from_this(),
            std::placeholders::_1,
            std::placeholders::_2
    );

    if (sslStream)
        http::async_read_header(*sslStream, streambuf, responseParser, handler);
    else
        http::async_read_header(socket, streambuf, responseParser, handler);
}

void FileDownloader::on_header_read(boost::system::error_code &ec, std::size_t bytes_transferred)
{
    std::cout << "-------------------------------- header --------------------------------\n";
    for (auto it = responseParser.get().begin(); it != responseParser.get().end(); ++it)
        std::cout << it->name() << " " << it->value() << std::endl;
    std::cout << "------------------------------------------------------------------------" << std::endl;

    curSize = 0;
    // if targetSize is 0 then the download will be chunked
    targetSize = std::stoi(responseParser.get().at("Content-Length").to_string());

    if (responseParser.get().at("Content-Type") == "text/html") // something wrong on the server side
    {
        // todo: more checks
        // just check result code. Probably, need to read body of errors and throw exceptions with them
        if(responseParser.get().result() == http::status::not_found)
        {
            promise.set_exception(std::make_exception_ptr(std::runtime_error("not found")));
            return;
        }
    }

    auto handler = std::bind(
            &FileDownloader::on_read,
            shared_from_this(),
            std::placeholders::_1,
            std::placeholders::_2
    );

    prepareBuffer();

    if (sslStream)
        http::async_read_some(*sslStream, streambuf, responseParser, handler);
    else
        http::async_read_some(socket, streambuf, responseParser, handler);
}

void FileDownloader::on_read(boost::system::error_code &ec, std::size_t bytes_transferred)
{
    if (ec)
    {
        promise.set_exception(std::make_exception_ptr(boost::system::system_error(ec)));
        return;
    }

    stream->write(buffer, bytes_transferred);
    stream->flush();
    curSize += bytes_transferred;

    if (responseParser.is_done())
    {
        on_done(ec);
    }
    else
    {
        prepareBuffer();
        auto handler = std::bind(
                &FileDownloader::on_read,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2
        );
        
        if (sslStream)
            http::async_read_some(*sslStream, streambuf, responseParser, handler);
        else
            http::async_read_some(socket, streambuf, responseParser, handler);
    }
}

void FileDownloader::on_done(boost::system::error_code &ec)
{
    const auto &message = responseParser.get();

    if (message.result() == http::status::moved_permanently ||
        message.result() == http::status::found)
    {
        // our request redirected, so we need to retrieve new location from HTTP fields and restart connection
        start(message.at(http::field::location));
        return;
    }

    // now it's safe to read data from stream
    promise.set_value();

    if (ec && ec != boost::system::errc::not_connected)
    {
        promise.set_exception(std::make_exception_ptr(boost::system::system_error(ec)));
        return;
    }

    socket.close(ec);
    if (ec && ec != boost::system::errc::not_connected)
    {
        promise.set_exception(std::make_exception_ptr(boost::system::system_error(ec)));
        return;
    }

    socket.shutdown(tcp::socket::shutdown_both, ec);
}

void FileDownloader::prepareBuffer()
{
    delete[] buffer;
    buffer = new char[chunkSize];
    responseParser.get().body().data = buffer;
    responseParser.get().body().size = chunkSize;
}
