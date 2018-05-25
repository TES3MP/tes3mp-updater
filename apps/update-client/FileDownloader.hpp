//
// Created by koncord on 06.01.18.
//

#ifndef FILEDOWNLOADER_DOWNLOADER_HPP
#define FILEDOWNLOADER_DOWNLOADER_HPP

#include <future>
#include <string>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <boost/asio/streambuf.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/buffer_body.hpp>
#include <fstream>

#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/array.hpp>

class FileDownloader : public std::enable_shared_from_this<FileDownloader>
{
public:
    /**
     * @param ioservice
     * @param stream can be any device that supported back inserting
     * @param bufferSize - is size of buffer
     **/
    FileDownloader(boost::asio::io_service &ioservice, const std::shared_ptr<std::ostream> &stream, size_t bufferSize);
    ~FileDownloader();

    /**
     * This function asynchronously downloading data
     * @param url
     */

    void start(boost::beast::string_view url);

    static std::shared_ptr<FileDownloader> downloadToFile(boost::asio::io_service &ioservice,
                                                          const std::string &url,
                                                          const std::string &path,
                                                          size_t bufferSize = 1024);

    static std::shared_ptr<FileDownloader> downloadToBuf(boost::asio::io_service &ioservice,
                                                         const std::string &url,
                                                         std::vector<char> &buf,
                                                         size_t chunkSize = 1024);

    /**
     *
     * @return total bytes loaded and target size
     */
    std::tuple<size_t, size_t> getProgress() const { return std::make_tuple(curSize.load(), targetSize); };

    /**
     * When promise sets value, you can safely retrieve data
     */
    std::future<void> *future() { return &_future; }

private:
    boost::asio::io_service &ioservice;
    boost::asio::ip::tcp::resolver resolv;
    boost::asio::ip::tcp::socket socket;
    size_t chunkSize;
    std::shared_ptr<std::ostream> stream;
    char *buffer;
    std::unique_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket &>> sslStream;
    boost::beast::http::response_parser<boost::beast::http::buffer_body> responseParser;
    boost::asio::streambuf streambuf;
    boost::beast::http::request<boost::beast::http::empty_body> req;
    std::promise<void> promise;
    std::future<void> _future;
    std::atomic<size_t> curSize;
    size_t targetSize;

    void initSSL();
    void prepareBuffer();

    void on_resolve(const boost::system::error_code &ec, const boost::asio::ip::tcp::resolver::results_type &results);
    void on_connect(const boost::system::error_code &ec);
    void on_request_sent(const boost::system::error_code &ec, std::size_t bytes_transferred);
    void on_header_read(boost::system::error_code &ec, std::size_t bytes_transferred);
    void on_read(boost::system::error_code &ec, std::size_t bytes_transferred);
    void on_done(boost::system::error_code &ec);
};

#endif //FILEDOWNLOADER_DOWNLOADER_HPP
