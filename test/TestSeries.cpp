#include <gtest/gtest.h>
#include <chrono>
#include <cpprest/asyncrt_utils.h>

#include "Series.h"
#include "Utils.h"
#include <model/Quote.h>



using namespace io::swagger::client;
using namespace utility;

TEST(TestSeries, initialisation) {

    auto quote_file = TESTDATA_LOCATION"/quotes_XBTUSD.json";
    //auto quote_file = "/home/rory/.tradingo/data/tickRecorder/2021-10-05/quotes_XBTUSD.json";
    auto series = Series<model::Quote>(quote_file);
    ASSERT_EQ(series.size(), 1000);

    auto time_index = datetime::from_string("2021-10-05T00:00:19.678Z", datetime::date_format::ISO_8601);
    auto quote = std::make_shared<model::Quote>();
    auto quote_json = web::json::value::parse(R"({"askPrice":49244.5,"askSize":116600,"bidPrice":49244,"bidSize":1200,"symbol":"XBTUSD","timestamp":"2021-10-05T00:00:19.678Z"})");
    quote->fromJson(quote_json);
    auto quote_retrieved = series[time_index];

    ASSERT_EQ(quote->getTimestamp().to_string(utility::datetime::date_format::ISO_8601),
        quote_retrieved->getTimestamp().to_string(utility::datetime::date_format::ISO_8601));

}
