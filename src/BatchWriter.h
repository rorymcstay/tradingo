//
// Created by rory on 15/06/2021.
//

#ifndef TRADING_BOT_BATCHWRITER_H
#define TRADING_BOT_BATCHWRITER_H

#include <iomanip>
#include <thread>
#include <mutex>
#include <ModelBase.h>

#include <sstream>
#include <string>
#include <map>
#include <iostream>
#include <cstdlib>
#include <model/Trade.h>
#include <filesystem>
#include "Utils.h"

using namespace io::swagger::client;

template<typename Item>
class BatchWriter
{
    size_t _batchSize;
    std::fstream _filehandle;
    std::vector<Item> _batch;
    std::string _dateString;
    std::string _tableName;
    std::string _symbol;
    std::string _storage;
    std::string _fileLocation;

    void update_file_location();

public:
    BatchWriter(std::string tableName_, std::string symbol_, std::string storage_, int batchSize_);
    void write(Item item_);
    void write_batch();

};


template<typename T>
void BatchWriter<T>::update_file_location() {
    _dateString = formatTime(std::chrono::system_clock::now());
    std::string location = _storage+ "/" + _dateString;
    auto directoryLocation = std::filesystem::path(location);
    if (not std::filesystem::exists(directoryLocation))
    {
        LOGINFO("Creating new directory " << LOG_VAR(directoryLocation));
        std::filesystem::create_directories(directoryLocation);
    }
    _fileLocation = location + "/" + _tableName + "_" + _symbol + ".json";
}

template<typename T>
BatchWriter<T>::BatchWriter(std::string tableName_, std::string symbol_, std::string storage_, int batchSize_)
        :   _batchSize(batchSize_)
        ,   _filehandle()
        ,   _batch()
        ,   _dateString(formatTime(std::chrono::system_clock::now()))
        ,   _tableName(std::move(tableName_))
        ,   _symbol(std::move(symbol_))
        ,   _storage(std::move(storage_))
        ,   _fileLocation(_storage + "/" +_dateString + "/" + _tableName + "_"+ _symbol +".json")
{
    _batch.reserve(_batchSize);
    _filehandle.open(_fileLocation, std::ios::app);
    _filehandle << '\n';
    _filehandle.close();
}

template<typename T>
void BatchWriter<T>::write(T item_) {
    _batch.emplace_back(std::move(item_));
    if (_batch.size() > _batchSize)
        write_batch();
}

template<typename T>
void BatchWriter<T>::write_batch() {
    LOGINFO("Writing batch " << LOG_VAR(_tableName) << " to " << LOG_VAR(_fileLocation));
    _filehandle.open(_fileLocation, std::ios::app);
    for (auto& message : _batch) {
        _filehandle << message << "\n";
    }
    _filehandle.close();
    _batch.clear();
    update_file_location();
}


std::ostream& operator << (std::ostream& ss_, const model::ModelBase& modelBase);


#endif //TRADING_BOT_BATCHWRITER_H
