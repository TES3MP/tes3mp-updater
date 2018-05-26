//
// Created by koncord on 22.05.18.
//

#include "Options.hpp"
#include "TimeUtils.hpp"
#include <boost/program_options.hpp>
#include <iostream>

bool Options::parseOptions(int argc, char **argv, Options &options)
{
    namespace bpo = boost::program_options;
    typedef std::vector<std::string> StringsVector;

    bpo::options_description desc("Syntax: update-client <options>\nAllowed options");
    desc.add_options()
            ("help", "print help message")
            ("connect", bpo::value<std::string>(),
             "connect to download server (e.g. --connect=127.0.0.1:25560)")
            ("server", bpo::value<std::string>(),
             "address of the game server (used to linking data) (e.g. --server=127.0.0.1:25565)")
            ("path", bpo::value<std::string>(), "path to the directory with cache")
            ("clean", bpo::value<std::string>()->implicit_value(""), "removes unused files and empty directories");

    bpo::parsed_options valid_opts = bpo::command_line_parser(argc, argv)
            .options(desc).allow_unregistered().run();

    bpo::variables_map variables;

    // Runtime options override settings from all configs
    bpo::store(valid_opts, variables);
    bpo::notify(variables);


    if (variables.count("help"))
    {
        std::cout << desc << std::endl;
        return false;
    }

    if (variables.count("path") == 0)
    {
        std::cerr << "the option '--path' is required but missing" << std::endl;
        return false;
    }

    if (variables.count("connect"))
    {
        options.address = variables["connect"].as<std::string>(); // todo: validate address
        options.useServer = variables["server"].as<std::string>();
        if (variables.count("server") == 0)
        {
            std::cerr << "for \"connect\" option, you must also specify \"server\" option" << std::endl;
            return false;
        }
    }

    options.downloadPath = variables["path"].as<std::string>();
    options.useListPath = options.downloadPath + "/use_list.json";

    options.mode = 0;

    if (!options.address.empty())
        options.mode |= Options::Mode::DOWNLOADER;

    if (variables.count("clean"))
    {
        options.mode |= Options::Mode::CLEANER;

        std::string deleteOlderDate = variables["clean"].as<std::string>();

        using namespace std::chrono;
        auto now = system_clock::now();

        if (deleteOlderDate.empty())
        {
            // by default, delete servers older than 7 days
            auto deleteOlder7d = (now - hours(24 * 7)).time_since_epoch();
            options.timestamp = duration_cast<seconds>(deleteOlder7d).count();
        }
        else
        {
            options.timestamp = TimeUtil::timePointFromString(deleteOlderDate);
        }
    }

    return true;
}
