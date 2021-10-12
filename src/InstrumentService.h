//
// Created by rory on 12/10/2021.
//

#ifndef TRADINGO_INSTRUMENTSERVICE_H
#define TRADINGO_INSTRUMENTSERVICE_H
#include <unordered_map>
#include <api/InstrumentApi.h>
#include <model/Instrument.h>
#include "Config.h"

class InstrumentService {

    std::shared_ptr<api::InstrumentApi> _instrumentApi;
    std::shared_ptr<Config> _config;
    std::unordered_map<std::string, std::shared_ptr<api::Instrument>> _instruments;
public:
    explicit InstrumentService(std::shared_ptr<api::ApiClient> apiClient_, std::shared_ptr<Config> config_);


    const std::shared_ptr<model::Instrument>& get(const std::string& symbol_, bool reload_=false);
};

#endif //TRADINGO_INSTRUMENTSERVICE_H
