//
// Created by Rory McStay on 16/08/2021.
//

#ifndef MY_PROJECT_TEMPSTORAGE_H
#define MY_PROJECT_TEMPSTORAGE_H

#include <boost/filesystem.hpp>
#include <filesystem>

class TempStorage {

    boost::filesystem::path _path;
public:
    TempStorage();
    ~TempStorage();
    const std::string& name();

};


#endif //MY_PROJECT_TEMPSTORAGE_H
