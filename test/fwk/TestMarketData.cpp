//
// Created by Rory McStay on 18/06/2021.
//

#include "TestMarketData.h"
#include "Utils.h"
#include "model/Instrument.h"
#include "model/Trade.h"
#include <cpprest/asyncrt_utils.h>


template<typename T>
const utility::datetime& get_time(const T& modelBase_) {
    return modelBase_->getTimestamp();
}


/// get object
template<typename T>
std::shared_ptr<T> get_object_from_file(const std::string& file_name) {
    std::string json_file = TESTDATA_LOCATION "/objects/" + file_name + ".json";
    auto out = std::make_shared<T>();
    auto json_val = read_json_file(json_file);
    out->fromJson(json_val);
    return out;
}


void TestMarketData::operator<<(const std::string &marketDataString) {

    auto params = Params(marketDataString);

    auto json = params.asJson();
    auto type = getEventTypeFromString(marketDataString);

    if (type == "EXECUTION") {
        auto execution = fromJson<model::Execution>(json);
        operator<<(execution);
    } else if (type == "QUOTE") {
        auto quote = fromJson<model::Quote>(json);
        operator<<(quote);
    } else if (type == "MARGIN") {
        auto margin = fromJson<model::Margin>(json);
        operator<<(margin);
    } else if (type == "TRADE") {
        auto trade = fromJson<model::Trade>(json);
        operator<<(trade);
    } else if (type == "POSITION") {
        auto position = fromJson<model::Position>(json);
        operator<<(position);
    } else if (type == "INSTRUMENT") {
        auto trade = fromJson<model::Instrument>(json);
        operator<<(trade);
    } else {
        throw std::runtime_error("Must specify update type, one off POSITION, TRADE, QUOTE, EXECUTION, MARGIN");
    }

}

TestMarketData::TestMarketData(const std::shared_ptr<Config>& ptr, const std::shared_ptr<InstrumentService>& instSvc_)
: MarketDataInterface(ptr, instSvc_)
,    _config(ptr)
,    _realtime(ptr->get<bool>("realtime", false))
,    _time(utility::datetime::utc_now()) 
,    _lastDispatch() {

}

TestMarketData::TestMarketData()
:    MarketDataInterface()
,    _config(nullptr) 
,    _realtime(false)
,    _time(utility::datetime::utc_now()) 
,    _lastDispatch() {


}


void TestMarketData::sleep(const utility::datetime& time_) const {
    if (_events == 0) {
        return;
    }
    auto now = utility::datetime::utc_now();
    if (not _lastDispatch.actual_time.is_initialized()) {
        LOGWARN("last dispatch time is uninitialised, will not sleep.");
        return;
    }
    auto mktTimeDiff = time_diff(time_, _lastDispatch.mkt_time);

    auto timeSinceLastDispatch = time_diff(now, _lastDispatch.actual_time);

    if (timeSinceLastDispatch < mktTimeDiff /*the amount of time passed, is less than in the market*/) {
        auto sleep_for = mktTimeDiff-timeSinceLastDispatch;
        LOGDEBUG(LOG_NVP("TimeNow", now.to_string(utility::datetime::ISO_8601))
              << LOG_NVP("MktTime", time_.to_string(utility::datetime::ISO_8601))
              << LOG_NVP("LastDispatch", _lastDispatch.actual_time.to_string(utility::datetime::ISO_8601))
              << LOG_NVP("LastDispatchMktTime", _lastDispatch.mkt_time.to_string(utility::datetime::ISO_8601))
              << LOG_VAR(mktTimeDiff)
              << LOG_VAR(timeSinceLastDispatch)
              << LOG_VAR(sleep_for));
        std::this_thread::sleep_for(std::chrono::milliseconds(mktTimeDiff-timeSinceLastDispatch));
    }

}

void TestMarketData::init() {
    // populate initial test data
    operator<<(get_object_from_file<model::Instrument>("opening_instrument"));
    operator<<(get_object_from_file<model::Position>("opening_position"));
    operator<<(get_object_from_file<model::Margin>("opening_margin"));

}

template<typename T>
void TestMarketData::onEvent(const T& event_) {
    auto& time = get_time(event_);
    if (_realtime) {
        sleep(time);
    }
    _lastDispatch.actual_time = utility::datetime::utc_now();
    _lastDispatch.mkt_time = _time;
    _events++;
}

void TestMarketData::operator<<(const std::shared_ptr<model::Quote> &quote_) {
    onEvent(quote_);
    std::vector<std::shared_ptr<model::Quote>> quotes = {quote_};
    MarketDataInterface::handleQuotes(quotes, "insert");
    callback();
}

void TestMarketData::operator<<(const std::shared_ptr<model::Trade> &trade_) {
    onEvent(trade_);
    std::vector<std::shared_ptr<model::Trade>> trades = {trade_};
    MarketDataInterface::handleTrades(trades, "insert");
    callback();
}

void TestMarketData::operator<<(const std::shared_ptr<model::Execution> &exec_) {
    onEvent(exec_);
    std::vector<std::shared_ptr<model::Execution>> execs = {exec_};
    MarketDataInterface::handleExecutions(execs, "insert");
    callback();
}

void TestMarketData::operator<<(const std::shared_ptr<model::Position> &pos_) {
    onEvent(pos_);
    std::vector<std::shared_ptr<model::Position>> positions = {pos_};
    MarketDataInterface::handlePositions(positions, "insert");
}

void TestMarketData::operator<<(const std::shared_ptr<model::Margin> &margin_) {
    onEvent(margin_);
    std::vector<std::shared_ptr<model::Margin>> margins= {margin_};
    MarketDataInterface::handleMargin(margins, "insert");
}

void TestMarketData::operator<<(const std::shared_ptr<model::Instrument> &instrument_) {
    onEvent(instrument_);
    std::vector<std::shared_ptr<model::Instrument>> insts = {instrument_};
    MarketDataInterface::handleInstruments(insts, "update");
}

void TestMarketData::operator<<(const std::shared_ptr<model::Order> &order_) {
    onEvent(order_);
    std::string action = "update";
    if (order_->getOrdStatus() == "Canceled") {
        action = "delete";
    } else if (order_->getOrdStatus() == "Filled" ||
            tradingo_utils::almost_equal(order_->getLeavesQty(), 0.0)) {
        action = "delete";
    } else if (order_->getOrdStatus() == "Rejected") {
        action = "delete";
    } else if (order_->getOrdStatus() == "New") {
        action = "insert";
    }
    auto order_copy = std::make_shared<model::Order>();
    auto json_buffer = order_->toJson();
    order_copy->fromJson(json_buffer);
    std::vector<std::shared_ptr<model::Order>> orders = {order_copy};
    MarketDataInterface::handleOrders(orders, action);
}
