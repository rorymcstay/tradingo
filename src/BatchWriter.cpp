//
// Created by rory on 15/06/2021.
//

#include "BatchWriter.h"

std::ostream &operator<<(std::ostream &ss_, const model::ModelBase &modelBase) {
    ss_ << modelBase.toJson().serialize();
    return ss_;
}
