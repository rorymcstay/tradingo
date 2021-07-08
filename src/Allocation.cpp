//
// Created by Rory McStay on 07/07/2021.
//

#include "Allocation.h"

Allocation::Allocation()
:   _price(0.0)
,   _size(0.0)
,   _side("")
,   _order(nullptr) {

}

Allocation::Allocation(price_t price_, size_t qty_, const std::string &side_)
:   _price(price_)
,   _size(qty_)
,   _side(side_)
,   _order(nullptr) {

}
