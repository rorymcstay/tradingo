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

using timestamp_t = std::chrono::time_point<std::chrono::system_clock>;

std::ostream& operator << (std::ostream& ss_, const model::ModelBase& modelBase)
{
    ss_ << modelBase.toJson().serialize();
    return ss_;
}

std::string formatTime(timestamp_t time_)
{
    auto outTime = std::chrono::system_clock::to_time_t(time_);
    std::stringstream ss;
    ss << std::put_time(localtime(&outTime), "%Y-%m-%d");
    return ss.str();
}

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

    void update_file_location() {
        _dateString = formatTime(std::chrono::system_clock::now());
        std::string location = _storage+ "/" + _dateString;
        auto directoryLocation = std::filesystem::path(location);
        if (not std::filesystem::exists(directoryLocation))
        {
            std::filesystem::create_directories(directoryLocation);
        }
        _fileLocation = location + "/" + _tableName + "_" + _symbol + ".json";
    }

public:
    BatchWriter(std::string tableName_, std::string symbol_, std::string storage_)
            :   _batchSize(1000)
            ,   _filehandle()
            ,   _batch()
            ,   _dateString(formatTime(std::chrono::system_clock::now()))
            ,   _tableName(std::move(tableName_))
            ,   _symbol(std::move(symbol_))
            ,   _storage(std::move(storage_))
            ,   _fileLocation(_storage + "/" +_dateString + "/" + _tableName + "_"+ _symbol +".json")
    {
        _batch.reserve(_batchSize);
    }

    void write(std::shared_ptr<model::ModelBase> item_) {
        _batch.emplace_back(std::move(item_));
        if (_batch.size() > _batchSize)
            write_batch();
    }

    void write_batch() {
        std::cout << "Writing batch " << _tableName << '\n';
        _filehandle.open(_fileLocation, std::ios::app);
        for (auto& message : _batch)
        {
            _filehandle << *message << "\n";
        }
        _filehandle.close();
        _batch.clear();
        update_file_location();
    }

};


#endif //TRADING_BOT_BATCHWRITER_H
