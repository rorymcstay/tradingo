#include <boost/program_options.hpp>

#include "Strategy.h"
#include "Utils.h"
#include "Config.h"
#include "MarketData.h"

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
    std::string symbol = config->get("symbol");
    std::string url = config->get("connectionString");

    auto marketData = std::make_shared<MarketData>(url, symbol);
    auto apiConfig = std::make_shared<api::ApiConfiguration>();
    apiConfig->setBaseUrl(config->get("baseUrl"));
    apiConfig->setApiKey("apiKey", config->get("apiKey"));
    auto orderManager = std::make_shared<api::OrderApi>( std::make_shared<api::ApiClient>(apiConfig));
    auto strategy = std::make_shared<Strategy<api::OrderApi>>(marketData, orderManager);
    strategy->init(config);

    while (strategy->shouldEval()) {
        strategy->evaluate();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    INFO("exiting");
}