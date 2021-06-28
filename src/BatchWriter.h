//
// Created by rory on 15/06/2021.
//

#ifndef TRADING_BOT_BATCHWRITER_H
#define TRADING_BOT_BATCHWRITER_H

#include "Event.h"
#include "MarketData.h"
#include <iomanip>
#include <thread>
#include <model/Trade.h>
#include <filesystem>
#include <model/Instrument.h>
#include <model/Quote.h>
#include <ranges>
#include <mutex>
#include <cpprest/json.h>
#include <cpprest/ws_client.h>
#include <sstream>
#include <string>
#include <map>
#include <iostream>
#include <cstdlib>


class BatchWriter
{
    size_t _batchSize;
    std::fstream _filehandle;
    std::vector<std::shared_ptr<model::ModelBase>> _batch;
    std::string _dateString;
    std::string _tableName;
    std::string _symbol;
    std::string _storage;
    std::string _fileLocation;

    void update_file_location();

public:
    BatchWriter(std::string tableName_, std::string symbol_, std::string storage_);
    void write(std::shared_ptr<model::ModelBase> item_);
    void write_batch();

};


#endif //TRADING_BOT_BATCHWRITER_H