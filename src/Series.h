#ifndef TRADINGO_SERIES_H
#define TRADINGO_SERIES_H

#include <cpprest/asyncrt_utils.h>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <fstream>
#include <ModelBase.h>
#include "Utils.h"



template<typename T>
class Series {

    std::vector<std::shared_ptr<T>> _data;
    std::vector<long> _index;
    utility::datetime::interval_type _start;
    utility::datetime::interval_type _end;
    double _resolution;
    long _size;
public:
    explicit Series(
            const std::string& dataFile_,
            int resolution_=1);

    int size() const;

    std::shared_ptr<T> operator[] (const utility::datetime& time_) const;

    std::shared_ptr<T> operator[] (const std::string& time_) const;
};
template <typename T>
Series<T>::Series(const std::string& dataFile_, int resolution_) {
    std::ifstream dataFile;

    int time_index = resolution_;

    dataFile.open(dataFile_);
    std::vector<std::shared_ptr<T>> tmpdata;
    if (not dataFile.is_open()) {
        std::stringstream msg;
        msg << "File " << LOG_VAR(dataFile_) << " does not exist.";
        throw std::runtime_error(msg.str());
    }
    std::shared_ptr<T> record = getEvent<T>(dataFile);

    tmpdata.push_back(record);
    for (;record = getEvent<T>(dataFile); record) {
        tmpdata.push_back(record);
    }
    std::sort(tmpdata.begin(), tmpdata.end(),
              [](const std::shared_ptr<T>& it1, const std::shared_ptr<T>& it2) {
                return it1->getTimestamp() < it2->getTimestamp();
              }
    );
    _start = tmpdata[0]->getTimestamp().to_interval();
    _end = tmpdata.back()->getTimestamp().to_interval();
    auto size = _end - _start;
    _data = std::vector<std::shared_ptr<T>>(size, nullptr);
    _size = tmpdata.size();
    for (auto item : tmpdata) {
        long index = _end - item->getTimestamp().to_interval();
        _data[index] = item;
        _index.push_back(index);
    }
}

template <typename T>
std::shared_ptr<T> Series<T>::operator[](const utility::datetime& time_) const {
    if (time_.to_interval() < _start) {
        throw std::runtime_error("time out of bounds");
    } else if (time_.to_interval() > _end) {
        throw std::runtime_error("time out of bounds");
    }
    auto index = _end - time_.to_interval();
    for (int i(index); i--; i >= 0) {
        if (_data.at(index))
            return _data[index];
    }
    throw std::runtime_error("Series is empty");
}

template <typename T> int Series<T>::size() const { return _size; }

template <typename T>
std::shared_ptr<T> Series<T>::operator[](const std::string& time_) const {
    auto time = utility::datetime::from_string(time_, utility::datetime::date_format::ISO_8601);
    return operator[](time);
}

template<typename T>
std::shared_ptr<T> getEvent(std::ifstream &fileHandle_) {
    std::string str;
    auto quote = std::make_shared<T>();
    if (not std::getline(fileHandle_, str))
        return nullptr;
    if (str.empty())
        return getEvent<T>(fileHandle_);
    auto json = web::json::value::parse(str);
    quote->fromJson(json);
    return quote;
}

#endif //TRADINGO_TESTENV_H
