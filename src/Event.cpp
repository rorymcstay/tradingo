//
// Created by rory on 14/06/2021.
//
#include "Event.h"

void Event::setAction(const std::string &action_) {
    if (action_ == "update") {
        _actionType = ActionType::Update;
    } else if (action_ == "delete") {
        _actionType = ActionType::Delete;
    } else if (action_ == "insert") {
        _actionType = ActionType::Insert;
    } else if (action_ == "partial") {
        _actionType = ActionType::Partial;
    }
}
