//
// Created by Rory McStay on 16/08/2021.
//

#include "gtest/gtest.h"
#include "fwk/TempStorage.h"

#include "Config.h"

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

    ASSERT_EQ(config1.get("kv2"), "newVal");
    ASSERT_EQ(config1.get("kv3"), "val3");
}

TEST(Config, read_from_file) {
    auto storage = TempStorage();
    std::ofstream configFile;
    configFile.open(storage.name() + "/config.cfg");
    configFile.write(
            "val1=10\n"
            "val2=20", std::ios::app);
    auto config = Config(storage.name() + "/config.cfg");
    ASSERT_EQ(config.get("val1"), "10");
    ASSERT_EQ(config.get("val2"), "20");
}