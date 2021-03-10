// SPDX-FileCopyrightText: 2020 Bloomberg Finance LP
// SPDX-License-Identifier: GPL2.0-or-later

// dfusvc.cpp
#include <windows.h>
#include <iostream>
#include <memory>
#include <boost/thread/thread.hpp>
#include <boost/program_options.hpp>

#include "dfusvc_server.h"

int main(int argc, TCHAR * argv[])
{

    if (1 == argc) {
        std::cout << "Bloomberg device updater v0.1.4\n"
            << "Copyright 2020 Bloomberg Finance LP.\n"
            << "Some of this program's components are licensed under the GPLv2 and LGPLv2.1 licenses.\n"
            << "Source code can be obtained from https://github.com/bloomberg/blpdevupd.\n";
    }

    std::string pipename("");
    try
    {
        boost::program_options::options_description desc{ "Options" };
        desc.add_options()
            ("help, h", "Help screen")
            ("name", boost::program_options::value<std::string>()->default_value(""), "Pipe name");

        boost::program_options::variables_map vm;
        store(parse_command_line(argc, argv, desc), vm);
        notify(vm);

        if (vm.count("help")) {
            std::cout << desc << '\n';
            return 0;
        }

        if (vm.count("name")) {
            pipename = vm["name"].as<std::string>();
            std::cout << "Pipe name set by argument: " << vm["name"].as<std::string>() << std::endl;
        }
        else {
            std::cout << "Use default pipe name" << std::endl;
        }
    }
    catch (const boost::program_options::error& ex)
    {
        std::cout << "Error parsing input arguments" << std::endl;
        std::cout << ex.what() << '\n';
        return -1;
    }


    auto svc =  std::make_shared<dfusvc::DFUServiceServer>(pipename);
    if (svc->waitForConnection() != 0) {
        return -1;
    }
    std::cout << "Client connected" << std::endl;

    int ret = 0;
    while (1) {
        ret = svc->processRequest();
        if (ret != 0) {
            break;
        }
    }
           
    std::cout << "Exiting process" << std::endl;
    return ret;
}
