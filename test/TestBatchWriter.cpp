//
// Created by Rory McStay on 12/08/2021.
//

#include <gtest/gtest.h>

#define _TURN_OFF_PLATFORM_STRING
#include <model/Order.h>

#include "fwk/TempStorage.h"
#include "BatchWriter.h"


using namespace io::swagger::client;

TEST(BatchWriter, Order_smoke_test) {

    auto storage = TempStorage();
    auto printer = [](const std::shared_ptr<model::ModelBase>& order_) {
        return order_->toJson().serialize();
    };
    auto batchWriter = BatchWriter<std::shared_ptr<model::Order>>(
         "quotes", "XBTUSD", storage.name(), 5, printer, false);
    std::string toWrite = R"({"clOrdID":"MCST8","cumQty":0,"leavesQty":900,"ordStatus":"Replaced","orderID":"","orderQty":900,"origClOrdID":"MCST7","price":42780.5,"side":"Sell","symbol":"XBTUSD","timestamp":"2021-08-07T00:29:49.762Z"})";
    auto inserted = 0;
    for (int i(0); i < 5; i++) {
        inserted++;
        auto order = std::make_shared<model::Order>();
        auto data = web::json::value::parse(toWrite);
        order->fromJson(data);
        batchWriter.write(order);
    }
    batchWriter.write_batch();

    std::ifstream fileToCheck;
    fileToCheck.open(batchWriter.location());
    std::string line;
    auto count = 0;
    std::getline(fileToCheck, line);
    ASSERT_EQ(line, "{\"clOrdID\":\"MCST8\",\"cumQty\":0,\"leavesQty\":900,\"ordStatus\":\"Replaced\",\"orderID\":\"\",\"orderQty\":900,\"origClOrdID\":\"MCST7\",\"price\":42780.5,\"side\":\"Sell\",\"symbol\":\"XBTUSD\",\"timestamp\":\"2021-08-07T00:29:49.762Z\"}");
    while (std::getline(fileToCheck, line)) {
        count++;
        ASSERT_EQ(line, toWrite);
    }
    //ASSERT_EQ(count, inserted);
}
