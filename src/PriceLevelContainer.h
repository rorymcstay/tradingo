#ifndef PRICE_LEVEL_CONTAINER_H
#define PRICE_LEVEL_CONTAINER_H
#include <algorithm>
#include <memory>
#include <boost/optional.hpp>
#include <utility>


#include "Utils.h"
#include "ApiException.h"


using namespace tradingo_utils;


template<typename T>
class PriceLevelContainer {
    std::vector<std::shared_ptr<T>> _data;
protected:
    price_t _tickSize;
    price_t _referencePrice;
    price_t _lowPrice;
    price_t _highPrice;
    qty_t _lotSize;
    size_t allocIndex(price_t price_);
    std::vector<index_t> _occupiedLevels;
    // set a price level occupied
    std::shared_ptr<T> occupy(price_t price_level_);
    // unoccupy a price level
    std::shared_ptr<T> unoccupy(price_t price_level_);

public:

    /// iterator over occupied levels of price space
    class iterator_type {

        std::vector<index_t>::const_iterator _index_iter;
        const PriceLevelContainer<T>* _series;

    public:

        iterator_type(
                std::vector<index_t>::const_iterator index_,
                const PriceLevelContainer<T>* series_
        )
        :   _index_iter(index_)
        ,   _series(series_) {}

        iterator_type(const iterator_type& iterator_type_)
        :   _index_iter(iterator_type_._index_iter)
        ,   _series(iterator_type_._series) {}

        iterator_type operator++() {
            _index_iter++;
            return *this;
        }

        bool operator!=(const iterator_type & other) const { 
            return other._index_iter != _index_iter;
        }

        const std::shared_ptr<T>& operator*() const {
            return _series->_data[*_index_iter];
        }

        T* operator->() const { 
            auto& data = _series->_data;
            return data[*_index_iter].get();
        }

    };
    friend iterator_type;
    
    /*
     * @param symbol_: the symbol
     * @param clOrdIdPrefix_: a prefix for all orders client order ids
     * @param clOidSeed_: deprecated - no longer used iirc
     * @param midPoint_: The mid point of index
     * @param tickSize_: The minumum price increment
     * @param lotSize_: the minimum order qty
     * */
    PriceLevelContainer(
                price_t midPoint_,
                price_t tickSize_,
                qty_t lotSize_);

    std::vector<std::shared_ptr<T>> allocations() {
        return _data;
    }
    /// round price to tick.
    price_t roundTickPassive(price_t price_);
    /// round quantity to lot size.
    qty_t roundLotSize(qty_t size_);
    /// get level for price
    const std::shared_ptr<T>& get(price_t price_);
    // wrapper on get for bracket notation
    const std::shared_ptr<T>& operator[] (price_t price_) {
        return get(price_);
    }


    /// iterators
    iterator_type begin() const;
    iterator_type end() const;
    size_t size() const ;
    /// return a vector of occupied levels
    std::vector<price_t> occupiedLevels() const {
        std::vector<price_t> out;
        std::transform(_occupiedLevels.begin(), _occupiedLevels.end(),
                        out.begin(), [](const std::shared_ptr<T>& alloc_) {
                                            return alloc_->getPrice();
                                        });
        return out;
    }


};

template<typename T>
PriceLevelContainer<T>::PriceLevelContainer(
                                  price_t midPoint_, price_t tickSize_,
                                  qty_t lotSize_)
    :   _data()
    ,   _tickSize(tickSize_)
    ,   _referencePrice(midPoint_)
    ,   _lowPrice(0.0)
    ,   _highPrice(0.0)
    ,   _lotSize(lotSize_)
    ,   _occupiedLevels()
{
    for (int i=0; i < 2*(size_t)(midPoint_/tickSize_); i++) {
        auto item = std::make_shared<T>();
        item->setPrice(i*tickSize_);
        _data.push_back(item);
    }

}

template<typename T>
const std::shared_ptr<T>& PriceLevelContainer<T>::get(price_t price_) {
    return _data[allocIndex(price_)];
}

/// return the number of ticks in price
template<typename T>
size_t PriceLevelContainer<T>::allocIndex(price_t price_) {
    int index = price_/_tickSize;
    return index;
}

template<typename T>
price_t PriceLevelContainer<T>::roundTickPassive(price_t price_) {
    return price_;
}

template<typename T>
qty_t PriceLevelContainer<T>::roundLotSize(qty_t size_) {
    if (size_ <= _lotSize)
        return 0.0;
    return size_ - ((int)size_ % (int)_lotSize);
}





template<typename T>
size_t PriceLevelContainer<T>::size() const {
    return _occupiedLevels.size();
}


template<typename T>
typename PriceLevelContainer<T>::iterator_type
PriceLevelContainer<T>::begin() const {
    return PriceLevelContainer<T>::iterator_type(_occupiedLevels.begin(), this);
}


template<typename T>
typename PriceLevelContainer<T>::iterator_type
PriceLevelContainer<T>::end() const {
    return PriceLevelContainer<T>::iterator_type(_occupiedLevels.end(), this);
}


template<typename T>
std::shared_ptr<T>
PriceLevelContainer<T>::unoccupy(price_t price_level_)  {
    auto ref = std::find(_occupiedLevels.begin(),
                         _occupiedLevels.end(),
                         price_level_);
    if (ref != _occupiedLevels.end()) {
        _occupiedLevels.erase(ref);
    }
    return get(price_level_);
}


template<typename T>
std::shared_ptr<T>
PriceLevelContainer<T>::occupy(price_t price_level_)  {
    auto ref = std::find(_occupiedLevels.begin(),
                         _occupiedLevels.end(),
                         price_level_);
    if (ref != _occupiedLevels.end()) {
        _occupiedLevels.insert(
            std::upper_bound( _occupiedLevels.begin(), _occupiedLevels.end(),
                        price_level_), price_level_);
    }
    return get(price_level_);
}

#endif // PRICE_LEVEL_CONTAINER_H
