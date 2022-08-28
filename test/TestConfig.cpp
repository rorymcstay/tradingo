//
// Created by Rory McStay on 16/08/2021.
//

#include "gtest/gtest.h"
#include "fwk/TempStorage.h"

#include "Config.h"
#include "Utils.h"
#include "OrderBook.h"

TEST(TestConfig, plus_equal_operator) {
    Config config1 {
            {"kv1", "val1"},
            {"kv2", "val2"}
    };
    Config config2 {
            {"kv3", "val3"},
            {"kv2", "newVal"}
    };

    config1 += config2;

    ASSERT_EQ(config1.get<std::string>("kv2"), "newVal");
    ASSERT_EQ(config1.get<std::string>("kv3"), "val3");
}

TEST(Config, DISABLED_read_from_file) {
    auto storage = TempStorage();
    std::ofstream configFile;
    configFile.open(storage.name() + "/config.cfg");
    std::string st= "val1=10\nval2=20";
    configFile.write(st.c_str(), std::ios::app);
    configFile.close();
    auto config = Config(storage.name() + "/config.cfg");
    ASSERT_EQ(config.get<std::string>("val1"), "10");
    ASSERT_EQ(config.get<std::string>("val2"), "20");
    ASSERT_EQ(config.get<int>("val2"), 20);
}


TEST(increment_date_str, test) {
    ASSERT_EQ(tradingo_utils::datePlusDays("2022-01-01", 1), "2022-01-02");
    ASSERT_EQ(tradingo_utils::datePlusDays("2022-10-31", 1), "2022-11-01");
}


TEST(tradingo_order_book_funcs, all) {
    price_t _tickSize = 0.5;

}
