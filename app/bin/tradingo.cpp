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

    // setup CLI parser
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("config", po::value<std::string>(), "config file to load");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // check mandatory config file provided
    if (!vm.contains("config")) {
        LOGINFO("Config file not provided.");
        return 1;
    }

    // parse config and create context.
    auto config = std::make_shared<Config>(vm.at("config").as<std::string>());
    auto context = std::make_shared<Context<MarketData, api::OrderApi>>(config);
    context->init();
    context->initStrategy();

    // running loop
     while (context->strategy()->shouldEval()) {
         //LOGINFO(AixLog::Color::YELLOW << "======== START Evaluate ========" << AixLog::Color::none);
         context->strategy()->evaluate();
         //LOGINFO(AixLog::Color::YELLOW << "======== FINISH Evaluate ========" << AixLog::Color::none);
         //std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    LOGINFO("exiting");
    return 0;
}