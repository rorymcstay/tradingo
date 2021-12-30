//
// Created by rory on 16/06/2021.
//

#include "gtest/gtest.h"

// cpprestsdk
#include <pplx/threadpool.h>


int main(int argc, char **argv) {

    crossplat::threadpool::initialize_with_threads(2);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


