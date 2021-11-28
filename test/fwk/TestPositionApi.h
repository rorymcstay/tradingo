#ifndef TRADINGO_TESTPOSITIONAPI_H
#define TRADINGO_TESTPOSITIONAPI_H

#include <unordered_map>

#define _TURN_OFF_PLATFORM_STRING
#include <ApiClient.h>


using namespace io::swagger::client;
class TestPositionApi {

    std::unordered_map<std::string, double> _leverages;

public:
    double getLeverage(const std::string& symbol_) const { return _leverages.at(symbol_); }

public: 

    TestPositionApi(std::shared_ptr<io::swagger::client::api::ApiClient> ptr);
    void position_updateLeverage(const std::string& symbol_, double leverage_);
};

#endif
