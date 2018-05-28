#include <iostream>
#include <fstream>
#include <thread>

#include <boost/asio/io_context.hpp>

#include <json.hpp>
#include <components/JSONEntry.hpp>

#include "Options.hpp"
#include "Listener.hpp"
#include "components/FLGenerator.hpp"

int main(int argc, char **argv)
{
    Options options;
    if (!Options::parseOptions(argc, argv, options))
        return 0;
    nlohmann::json config;

    {
        if (options.cfg.empty())
            options.cfg = "update-server.json";
        std::ifstream cfg_file(options.cfg);
        cfg_file >> config;
    }

    auto const threads = std::max<int>(1, config["maxThreads"]);

    boost::asio::io_context ioc{threads};

    std::string dataPath = options.dataPath;
    if (options.dataPath.empty())
        config["dataPath"].get<std::string>();

    if (dataPath.empty())
        throw std::runtime_error("dataPath is not present in the config neither in the running options");


    std::shared_ptr<FLGenerator> file_listGenerator = nullptr;

    if (config["fileListGenerator"]["use"] == true)
    {
        file_listGenerator = std::make_shared<FLGenerator>(dataPath, config["fileListGenerator"]["downloadAddress"]);
        file_listGenerator->start();
    }

    Listener::spawn(ioc, boost::asio::ip::make_address(config["address"].get<std::string>()), config["externalPort"],
                    dataPath, config["maxClients"], file_listGenerator);

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for (auto i = threads - 1; i > 0; --i)
    {
        v.emplace_back([&ioc] {
            ioc.run();
        });
    }

    std::cout << "Listening on: " << config["address"].get<std::string>() << ":" << config["externalPort"] << std::endl;
    std::cout << "Data path: " << dataPath << std::endl;

    ioc.run();

    return 0;
}
