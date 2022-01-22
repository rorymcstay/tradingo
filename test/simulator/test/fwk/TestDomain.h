#ifndef SIMULATOR_TEST_DOMAIN_H
#define SIMULATOR_TEST_DOMAIN_H
#include <string>

#define GETTERSETTER(type_, field_ ,default_) \
    type_ _##field_ = default_; \
    const type_& get##field_() const { \
        return _##field_; \
    } \
    void set##field_(const type_& val_) { \
        _##field_ = val_; \
    }


struct TestOrder {

    TestOrder()
    :   _Symbol("")
    ,   _Side("")
    ,   _Account()
    ,   _OrderID("")
    ,   _ClOrdID()
    ,   _LeavesQty()
    ,   _LastPx()
    ,   _CumQty()
    {}

    GETTERSETTER(std::string, Symbol, "XBTUSD");
    GETTERSETTER(std::string, Side, "");
    GETTERSETTER(double, Account, 0.0);
    GETTERSETTER(std::string, OrderID, "");
    GETTERSETTER(std::string, ClOrdID, "");
    GETTERSETTER(std::string, OrdStatus, "");
    GETTERSETTER(double, Price, 0.0);
    GETTERSETTER(double, AvgPx, 0.0);
    GETTERSETTER(double, OrderQty, 0.0);
    GETTERSETTER(double, LeavesQty, 0.0);
    GETTERSETTER(double, LastPx, 0.0);
    GETTERSETTER(double, CumQty, 0.0);
 
};

#endif
