#include <gtest/gtest.h>
#include <chrono>
#include <cpprest/asyncrt_utils.h>

#include "Series.h"
#include "Utils.h"
#include <model/Quote.h>

#include "fwk/TestEnv.h"


using namespace io::swagger::client;
using namespace utility;

datetime time_stamp(const std::string& timestamp_) {
    return datetime::from_string(timestamp_, datetime::date_format::ISO_8601);
}


TEST(TestSeries, initialisation) {

    auto quote_file = TESTDATA_LOCATION"/quotes_XBTUSD.json";
    //auto quote_file = "/home/rory/.tradingo/data/tickRecorder/2021-10-05/quotes_XBTUSD.json";
    auto series = TestEnv::QuoteSeries(quote_file, 100);
    ASSERT_EQ(series.size(), 1000);

    auto time_index = time_stamp("2021-10-05T00:00:19.678Z");
    auto quote = std::make_shared<model::Quote>();
    auto quote_json = web::json::value::parse(R"({"askPrice":49244.5,"askSize":116600,"bidPrice":49244,"bidSize":1200,"symbol":"XBTUSD","timestamp":"2021-10-05T00:00:19.678Z"})");
    quote->fromJson(quote_json);
    auto quote_retrieved = series[time_index];

    ASSERT_EQ(quote->getTimestamp().to_string(utility::datetime::date_format::ISO_8601),
        quote_retrieved->getTimestamp().to_string(utility::datetime::date_format::ISO_8601));

    int count = 0;
    for (auto& symbol : series) {
        count++;
        symbol->getSymbol();
    }

    ASSERT_EQ(count, 999);

    ASSERT_EQ(series["2021-10-05T00:00:19.678Z"]->getAskPrice(), 49244.5);
    ASSERT_EQ(series["2021-10-05T00:00:37.36Z"]->getAskPrice(), 49263.0);
    ASSERT_EQ(series["2021-10-05T00:00:56.103Z"]->getAskPrice(), 49261.5);

}

TEST(TestSeries, start_and_end_time) {

    auto quote_file = TESTDATA_LOCATION"/quotes_XBTUSD.json";
    //auto quote_file = "/home/rory/.tradingo/data/tickRecorder/2021-10-05/quotes_XBTUSD.json";
    auto series = TestEnv::QuoteSeries(quote_file);

    ASSERT_EQ(series.size(), 1000);
    ASSERT_EQ(series.begin()->getTimestamp().to_string(utility::datetime::date_format::ISO_8601),
        "2021-10-05T00:00:18.108Z");

    series.set_start("2021-10-05T00:00:19.678Z");
    series.set_end("2021-10-05T00:00:56.103Z");

    ASSERT_EQ(series.begin()->getTimestamp().to_string(utility::datetime::date_format::ISO_8601),
        "2021-10-05T00:00:19.678Z");
    ASSERT_EQ(series.end()->getTimestamp().to_string(utility::datetime::date_format::ISO_8601),
            "2021-10-05T00:00:56.103Z");
    int count  = 0;
    for (auto& symbol : series) {
        count++;
        symbol->getSymbol();
    }
    ASSERT_EQ(count, 938);

}
