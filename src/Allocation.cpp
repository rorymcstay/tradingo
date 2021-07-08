//
// Created by Rory McStay on 07/07/2021.
//

#include "Allocation.h"

Allocation::Allocation()
:   _price(0.0)
,   _size(0.0)
,   _order(nullptr) {

}

Allocation::Allocation(price_t price_, size_t qty_)
:   _price(price_)
,   _size(qty_)
,   _order(nullptr) {

}
