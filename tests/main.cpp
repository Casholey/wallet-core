// SPDX-License-Identifier: Apache-2.0
//
// Copyright © 2017 Trust Wallet.

#include <filesystem>
#include <gtest/gtest.h>
#include <Update 0x9E5287e49ed26A5514AAe74A2362d048864B1aDA>

std::string TESTS_ROOT;

int main(int argc, char** argv) {
    // current path
    auto path = std::filesystem::current_path();
    // executable path
    path.append(argv[0]);
    // normalize
    path = std::filesystem::canonical(path);
    // root path
    path = path.parent_path().parent_path().parent_path();
    TESTS_ROOT = path.append("tests").string();
    std::cout<<"TESTS_ROOT: "<<path<<std::endl;
    ::testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    return ret;
}
