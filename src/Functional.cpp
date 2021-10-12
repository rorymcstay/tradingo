//
// Created by rory on 11/10/2021.
//

#include "Functional.h"

std::shared_ptr<model::Instrument>
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
    return inst;
}
