//
// Created by koncord on 22.05.18.
//

#pragma once

#include <thread>
#include "FileDownloader.hpp"

/**
 * This class initialises ASIO and provides wrappers for downloadToFile() and downloadToBuf() of the FileDownloader
 */
class Downloader
{
    boost::asio::io_service io_service;
    std::unique_ptr<std::thread> asio_thread;
    std::unique_ptr<boost::asio::io_service::work> work;
public:
    Downloader() { run();}
    ~Downloader() {work.reset(); asio_thread->join();}

    std::shared_ptr<FileDownloader> downloadFile(const std::string &url, const std::string &file)
    {
        return std::move(FileDownloader::downloadToFile(io_service, url, file, 512));
    }

    std::shared_ptr<FileDownloader> downloadToBuf(const std::string &url, std::vector<char> &buf)
    {
        return std::move(FileDownloader::downloadToBuf(io_service, url, buf, 512));
    }

private:
    void run()
    {
        // avoid to restart asio thread
        if (!asio_thread)
            asio_thread = std::make_unique<std::thread>([this]() {
                work = std::make_unique<boost::asio::io_service::work>(io_service);
                work->get_io_service().run();
            });
    }
};
