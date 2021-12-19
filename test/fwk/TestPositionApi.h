#ifndef TRADINGO_TESTPOSITIONAPI_H
#define TRADINGO_TESTPOSITIONAPI_H

#include <unordered_map>

#define _TURN_OFF_PLATFORM_STRING
#include <ApiClient.h>


using namespace io::swagger::client;
class TestPositionApi {

    std::unordered_map<std::string, std::shared_ptr<model::Position>> _positions;

public:
    double addPosition(const std::shared_ptr<model::Position> position_) {
        return _positions[position_->getSymbol()] = position_;
    }

public: 

    TestPositionApi(std::shared_ptr<io::swagger::client::api::ApiClient> ptr);
    void position_updateLeverage(const std::string& symbol_, double leverage_);
};

#endif
