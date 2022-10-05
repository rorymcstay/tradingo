//
// Created by rory on 11/10/2021.
//

#include "Functional.h"
#include "Allocation.h"
#include "Allocations.h"
#include "Utils.h"
#include <stdexcept>



model::Instrument
func::get_instrument(const std::shared_ptr<api::InstrumentApi> &_instrumentApi, const std::string &symbol_) {
    std::shared_ptr<model::Instrument> inst;
    auto instTask = _instrumentApi->instrument_get(symbol_, SEVENNULL).then(
            [&inst](pplx::task<std::vector<std::shared_ptr<model::Instrument>>> instr_) {
                try {
                    inst = instr_.get()[0];
                } catch (std::exception &ex) {
                    LOGERROR("Http exception raised " << LOG_VAR(ex.what()));
                    inst = nullptr;
                }
            });
    instTask.wait();
    if (inst)
        return *inst;
    else
        return model::Instrument();
}

price_t func::get_additional_cost(const std::shared_ptr<Allocation>& alloc_, double leverage_)
{ 
    // TODO Need to check if selling, are we paying with existing position.
    return func::get_cost(alloc_->getPrice(), alloc_->getTargetDelta(), leverage_);
}


price_t func::get_additional_cost(
        const std::shared_ptr<model::Position>& position_,
        qty_t qty_delta_,
        price_t price_) {

    price_t cost;
    auto currentQty = position_->getCurrentQty();
    auto leverage = position_->getLeverage();
    if (not almost_equal(currentQty, 0.0)
            and sgn(qty_delta_) == sgn(currentQty)) {
        // positive cost since extending position
        cost = func::get_cost(price_, qty_delta_, leverage);
    }
    else { // pay less than the cost, since remainder from our position
        if (std::abs(currentQty) > std::abs(qty_delta_)) {
            // paying entirely with position, so we gain the cost
            cost = func::get_cost(price_, -qty_delta_, leverage);
        } else {
            // pay partially with positon
            cost = func::get_cost(price_, currentQty - qty_delta_, leverage);
        }
    }
    return cost; 
}


price_t func::get_cost(price_t price_, qty_t qty_, double leverage_)
{
    if (tradingo_utils::almost_equal(leverage_, 0.0)) {
        throw std::runtime_error("Leverage cannot be 0.0");
    }
    if (tradingo_utils::almost_equal(price_, 0.0)) {
        return 0.0;
    }
    return  qty_ * (1 /( price_* leverage_)) * func::SATOSHI;
}
