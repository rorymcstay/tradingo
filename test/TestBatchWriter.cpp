//
// Created by Rory McStay on 12/08/2021.
//


#include <gtest/gtest.h>

#include "BatchWriter.h"

#include <boost/filesystem.hpp>
#include <model/Order.h>
#include <filesystem>


using namespace io::swagger::client;

class TempStorage {

    boost::filesystem::path _path;
public:
    TempStorage()
    :   _path(boost::filesystem::temp_directory_path() / boost::filesystem::unique_path()) {
        boost::filesystem::create_directories(_path);
    }
    ~TempStorage() {
        boost::filesystem::remove_all(_path);
    }
    const std::string& name() { return _path.string(); }

};

TEST(BatchWriter, Order_smoke_test) {

    auto storage = TempStorage();
    auto printer = [](const std::shared_ptr<model::ModelBase>& order_) {
        return order_->toJson().serialize();
    };
    auto batchWriter = BatchWriter<std::shared_ptr<model::Order>>(
         "quotes", "XBTUSD", storage.name(), 5, printer);
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
    while (std::getline(fileToCheck, line)) {
        count++;
        ASSERT_EQ(line, toWrite);
    }
    ASSERT_EQ(count, inserted);



}