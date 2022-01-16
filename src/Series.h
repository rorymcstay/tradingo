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

using index_t = long;

template<typename T>
class Series {


    std::vector<std::shared_ptr<T>> _data;
    std::vector<index_t> _index;
    utility::datetime::interval_type _start_interval;
    utility::datetime::interval_type _end_interval;
    utility::datetime _start_time;
    utility::datetime _end_time;
    index_t _resolution;
    index_t _size;
public:

    class iterator_type {

        std::vector<index_t>::const_iterator _index_iter;
        const Series<T>* _series;

        struct proxy {
            const iterator_type _client;
            T* operator->() const { 
                auto& data = _client._series->_data;
                return data[*(_client._index_iter)].get();
            }
        };
    public:

        iterator_type(
                std::vector<index_t>::const_iterator index_,
                const Series<T>* series_
        )
        :   _index_iter(index_)
        ,   _series(series_) {}

        iterator_type(const iterator_type& iterator_type_)
        :   _index_iter(iterator_type_._index_iter)
        ,   _series(iterator_type_._series) {}

        iterator_type operator++() { _index_iter++; return *this; }
        bool operator!=(const iterator_type & other) const { 
            return _series->_data[*_index_iter]->getTimestamp() != other->getTimestamp(); 
        }
        const std::shared_ptr<T>& operator*() const { return _series->_data[*_index_iter]; }
        const proxy operator->() const { 
            return proxy { *this };
        }

    };
    friend iterator_type;


    Series(
        const std::string& dataFile_,
        index_t resolution_=1);

    index_t size() const;
    std::shared_ptr<T> operator[] (const utility::datetime& time_) const;
    std::shared_ptr<T> operator[] (const std::string& time_) const;

    iterator_type begin() const;
    iterator_type end() const;
    iterator_type operator++();
    bool operator!=(const iterator_type& other) const;

    void set_start(const std::string& start_);
    void set_end(const std::string& end_);

private:
    index_t index_of(const utility::datetime& date_) const;
    index_t make_index(const utility::datetime& date_) const;

};


template <typename T>
Series<T>::Series(const std::string& dataFile_, index_t resolution_)
:   _resolution(resolution_) {
    std::ifstream dataFile;

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
                return it1->getTimestamp().to_interval() < it2->getTimestamp().to_interval();
              }
    );
    _start_interval = tmpdata[0]->getTimestamp().to_interval();
    _start_time = tmpdata[0]->getTimestamp();
    _end_interval = tmpdata.back()->getTimestamp().to_interval();
    _end_time = tmpdata.back()->getTimestamp();
    auto size = (_end_interval - _start_interval + 1)/resolution_;
    _data = std::vector<std::shared_ptr<T>>(size, nullptr);
    for (auto item : tmpdata) {
        index_t index = Series<T>::make_index(item->getTimestamp());
        _data[index] = item;
        _index.push_back(index);
    }
}


template <typename T>
std::shared_ptr<T> Series<T>::operator[](const utility::datetime& time_) const {
    auto index = index_of(time_);
    return _data[index];
}

template <typename T>
index_t Series<T>::index_of(const utility::datetime& time_) const {
    if (time_.to_interval() < _start_interval) {
        throw std::runtime_error("time out of bounds");
    } else if (time_.to_interval() > _end_interval) {
        throw std::runtime_error("time out of bounds");
    }
    auto index = std::min(make_index(time_), index_t(_data.size() - 1));
    while (index >= 0) {
        if (_data.at(index))
            return index;
        index--;
    }
    throw std::runtime_error("Series is empty");
}


template <typename T>
index_t Series<T>::make_index(const utility::datetime& time_) const {
    return (time_.to_interval() - _start_interval)/_resolution;
}

template <typename T> index_t Series<T>::size() const { return _index.size(); }


template <typename T>
std::shared_ptr<T> Series<T>::operator[](const std::string& time_) const {
    auto time = utility::datetime::from_string(time_, utility::datetime::date_format::ISO_8601);
    return operator[](time);
}

template <typename T>
void Series<T>::set_start(const std::string& start_) {
    auto time = utility::datetime::from_string(start_, utility::datetime::date_format::ISO_8601);
    _start_time = time;
}

template <typename T>
void Series<T>::set_end(const std::string& end_) {
    auto time = utility::datetime::from_string(end_, utility::datetime::date_format::ISO_8601);
    _end_time = time;
}


template <typename T>
typename Series<T>::iterator_type Series<T>::begin() const {
    auto index = index_of(_start_time);
    auto it = _index.begin();
    while (*it < index) {
        it++;
    }
    typename Series<T>::iterator_type begin_ (it, this);
    return begin_; 
}


template <typename T>
typename Series<T>::iterator_type Series<T>::end() const {
    auto index = index_of(_end_time);
    auto it = _index.end() - 1;
    while (*it > index) {
        it--;
    }

    typename Series<T>::iterator_type end_ (it, this);
    return end_; 
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
