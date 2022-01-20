#ifndef SIMULATOR_TEST_DOMAIN_H
#define SIMULATOR_TEST_DOMAIN_H
#include <string>

struct TestOrder {

    std::string symbol;
    long order_id;
    std::string cl_ord_id;
    double price;
    double qty;
    std::string status;
    double leaves_qty;
    double last_px;
    double last_qty;
    double cum_qty;
 
};

#endif
