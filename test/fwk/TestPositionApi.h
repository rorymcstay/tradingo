#ifndef TRADINGO_TESTPOSITIONAPI_H
#define TRADINGO_TESTPOSITIONAPI_H

#include <model/Position.h>
#include <api/PositionApi.h>

class TestPositionApi {

public:
    TestPositionApi(std::shared_ptr<io::swagger::client::api::ApiClient> ptr);
}

#endif
