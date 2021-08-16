//
// Created by Rory McStay on 16/08/2021.
//

#include "TempStorage.h"

TempStorage::~TempStorage() {
    boost::filesystem::remove_all(_path);
}

TempStorage::TempStorage()
        :   _path(boost::filesystem::temp_directory_path() / boost::filesystem::unique_path()) {
    boost::filesystem::create_directories(_path);
}

const std::string &TempStorage::name() { return _path.string(); }
