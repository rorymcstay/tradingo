//
// Created by rory on 01/01/2022.
//

#include <boost/program_options.hpp>

#include "fwk/TestEnv.h"
#include "Strategy.h"
#include "Utils.h"
#include "Config.h"
#include "MarketData.h"
#include <memory>
#include <openssl/hmac.h>
#include <pplx/threadpool.h>
#include "api/PositionApi.h"


#include <cpprest/oauth2.h>
#include <Context.h>
#include <Series.h>


using namespace io::swagger::client;
namespace po = boost::program_options;


int main(int argc, char **argv) {

    // setup CLI parser
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("config", po::value<std::string>(), "config file to load");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    auto config = Config(vm.at("config").as<std::string>());

    auto tick_storage = config.get("tickStorage") + "/trades_XBTUSD.json";
    auto series = Series<model::Trade>(tick_storage);
    std::cout << "Series length: " << series.size() << '\n';

}
