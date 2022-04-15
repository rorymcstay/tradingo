#include <boost/program_options.hpp>

#include "Strategy.h"
#include "Utils.h"
#include "Config.h"
#include "MarketData.h"
#include <openssl/hmac.h>
#include <pplx/threadpool.h>
#include "api/PositionApi.h"

#include <cpprest/oauth2.h>
#include <Context.h>

using namespace io::swagger::client;
namespace po = boost::program_options;

#define DEFAULT_ARGS \
    {"version", "1.0"}, \
    {"pplxThreadCount", "4"} \


int main(int argc, char **argv) {

    // setup CLI parser
    po::options_description desc("Allowed options");
    std::vector<std::string> config_files;
    desc.add_options()
            ("help", "produce help message")
            ("config-json", po::value<std::string>(), "config in json string")
            ("config", po::value<std::vector<std::string>>(&config_files), "config file, may be specified multiple times");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    auto defaults = std::make_shared<Config>(std::initializer_list<std::pair<std::string,std::string>>({DEFAULT_ARGS}));
    if (not config_files.empty()) {
        for (auto& filepath : config_files) {
            auto config = std::make_shared<Config>(filepath);
            LOGINFO("Applying config " << filepath);
            *defaults += (*config);
        }
    }
    if (vm.contains("config-json")) {
        auto json_str = vm.at("config-json").as<std::string>();
        web::json::value json_val;
        try {
            LOGINFO("Applying config overrides");
            json_val = web::json::value::parse(json_str);
        } catch (const std::exception& ex) {
            std::stringstream errmsg;
            errmsg << "Invalid config dict passed"
                << LOG_VAR(ex.what()) << LOG_VAR(json_str);
            LOGERROR(errmsg.str());
            return 1;
        }
        auto config = std::make_shared<Config>(json_val);
        LOGINFO("Using config " << vm.at("config-json").as<std::string>());
        *defaults += (*config);
    }
    LOGINFO("Config: " << defaults->toJson().serialize());

    // parse config and create context.
    auto pplxThreadCount = defaults->get<int>("pplxThreadCount");
    crossplat::threadpool::initialize_with_threads(pplxThreadCount);

    auto context = std::make_shared<Context<MarketData, api::OrderApi, api::PositionApi>>(defaults);
    context->init();
    context->initStrategy();


    // running loop
     while (context->strategy()->shouldEval()) {
         //LOGINFO(AixLog::Color::YELLOW << "======== START Evaluate ========" << AixLog::Color::none);
         //LOGINFO(AixLog::Color::YELLOW << "======== FINISH Evaluate ========" << AixLog::Color::none);
         std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    LOGINFO("exiting");
    return 0;
}
