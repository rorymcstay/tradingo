//
// Created by rory on 12/10/2021.
//

#include "Functional.h"
#include <api/InstrumentApi.h>
#include <model/Instrument.h>

#include <utility>

#include "InstrumentService.h"


InstrumentService::InstrumentService(std::shared_ptr<api::ApiClient> apiClient_, std::shared_ptr<Config> config_)
:   _instrumentApi(std::make_shared<api::InstrumentApi>(apiClient_))
,   _config(std::move(config_)) {

    _instruments[_config->get("symbol")] = func::get_instrument(_instrumentApi, config_->get("symbol"));

}

const std::shared_ptr<model::Instrument>& InstrumentService::get(const std::string& symbol_, bool reload) {
    if (not reload && _instruments.find(symbol_) != _instruments.end())
        return _instruments[symbol_];
    _instruments[symbol_] = func::get_instrument(_instrumentApi, symbol_);
    return _instruments[symbol_];
}
