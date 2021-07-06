#include <boost/program_options.hpp>

#include "Strategy.h"
#include "Utils.h"
#include "Config.h"
#include "MarketData.h"
#include <openssl/hmac.h>
//#include "CPPLogger.hpp"

#include <cpprest/oauth2.h>
#include <Context.h>

using namespace io::swagger::client;
namespace ws = web::websockets;
namespace po = boost::program_options;



int main(int argc, char **argv) {

    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("config", po::value<std::string>(), "config file to load");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);


    if (!vm.contains("config")) {
        INFO("Config file not provided.");
        return 1;
    }
    auto configfile = vm.at("config").as<std::string>();
    auto config = std::make_shared<Config>(configfile);


    // marketData->init();
    // marketData->subscribe();
    // strategy->init(config);

    auto context = std::make_shared<Context<MarketData, api::OrderApi>>(config);
    context->marketData()->init();
    context->marketData()->subscribe();
    context->strategy()->init(config);

    /*
     while (strategy->shouldEval()) {
        strategy->evaluate();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }*/
    INFO("exiting");
    return 0;
}