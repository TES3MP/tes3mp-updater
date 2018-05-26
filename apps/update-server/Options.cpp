//
// Created by koncord on 26.05.18.
//


#include "Options.hpp"

#include <iostream>

#include <boost/program_options.hpp>

bool Options::parseOptions(int argc, char **argv, Options &options)
{
    namespace bpo = boost::program_options;
    typedef std::vector<std::string> StringsVector;

    bpo::options_description desc("Syntax: update-server <options>\nAllowed options");
    desc.add_options()
            ("help", "print help message")
            ("config", bpo::value<std::string>(), "path to config file (e.g. --config=updater.json)")
            ("dataPath", bpo::value<std::string>(), "path to data");

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

    options.cfg = variables["config"].as<std::string>();

    if(variables.count("dataPath"))
        options.dataPath = variables["dataPath"].as<std::string>();

    return true;
}