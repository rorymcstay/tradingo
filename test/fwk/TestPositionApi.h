#ifndef TRADINGO_TESTPOSITIONAPI_H
#define TRADINGO_TESTPOSITIONAPI_H

#include <unordered_map>

#define _TURN_OFF_PLATFORM_STRING
#include <ApiClient.h>
#include <model/Position.h>


using namespace io::swagger::client;
class TestPositionApi {

    std::unordered_map<std::string, std::shared_ptr<model::Position>> _positions;

public:
    void addPosition(const std::shared_ptr<model::Position> position_) {
        _positions[position_->getSymbol()] = std::move(position_);
    }

public: 

    TestPositionApi(std::shared_ptr<io::swagger::client::api::ApiClient> ptr);
    void position_updateLeverage(const std::string& symbol_, double leverage_);
};

#endif
