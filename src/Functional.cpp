//
// Created by rory on 11/10/2021.
//

#include "Functional.h"
#include "Allocation.h"
#include "Allocations.h"
#include "Utils.h"

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


price_t func::get_cost(price_t price_, qty_t qty_, double leverage_)
{ 
    return  qty_ * (1 / price_* leverage_);
}
