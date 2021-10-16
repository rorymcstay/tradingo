//
// Created by rory on 12/10/2021.
//

#ifndef TRADINGO_INSTRUMENTSERVICE_H
#define TRADINGO_INSTRUMENTSERVICE_H
#include <map>
#include <api/InstrumentApi.h>
#include <model/Instrument.h>
#include "Config.h"


class InstrumentService {
    /*
     * The instrument service is currently a very thin class initially
     * to allow adding static data for tests.
     *
     * This should be improved to read either a list of symbols to download
     * or just instrument data from json files.
     */
    std::shared_ptr<api::InstrumentApi> _instrumentApi;
    std::shared_ptr<Config> _config;
    std::map<std::string, api::Instrument> _instruments;
public:
    explicit InstrumentService(const std::shared_ptr<api::ApiClient>& apiClient_, std::shared_ptr<Config> config_);
    const model::Instrument& get(const std::string& symbol_, bool reload_=false);
    void add(const std::shared_ptr<model::Instrument>& inst_);
};

#endif //TRADINGO_INSTRUMENTSERVICE_H
