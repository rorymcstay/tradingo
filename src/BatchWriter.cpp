//
// Created by rory on 15/06/2021.
//
#include "BatchWriter.h"

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

void BatchWriter::update_file_location() {
    _dateString = formatTime(std::chrono::system_clock::now());
    std::string location = _storage+ "/" + _dateString;
    auto directoryLocation = std::filesystem::path(location);
    if (not std::filesystem::exists(directoryLocation))
    {
        std::filesystem::create_directories(directoryLocation);
    }
    _fileLocation = location + "/" + _tableName + "_" + _symbol + ".json";
}

BatchWriter::BatchWriter(std::string tableName_, std::string symbol_, std::string storage_)
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

void BatchWriter::write(std::shared_ptr<model::ModelBase> item_) {
    _batch.emplace_back(std::move(item_));
    if (_batch.size() > _batchSize)
        write_batch();
}

void BatchWriter::write_batch() {
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
