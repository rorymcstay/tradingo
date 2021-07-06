//
// Created by Rory McStay on 05/07/2021.
//

#ifndef TRADINGO_BREAKOUTSTRATEGY_H
#define TRADINGO_BREAKOUTSTRATEGY_H

//#include <boost/accumulators/>
#include "Strategy.h"


template<typename TORDApi>
class BreakOutStrategy final : public Strategy<TORDApi> {
public:
    ~BreakOutStrategy();
    BreakOutStrategy(std::shared_ptr<MarketDataInterface> mdPtr_,  std::shared_ptr<TORDApi> od_);
private:
    void onExecution(const std::shared_ptr<Event>& event_) override;
    void onTrade(const std::shared_ptr<Event>& event_) override;
    void onBBO(const std::shared_ptr<Event>& event_) override;

};

template<typename TORDApi>
void BreakOutStrategy<TORDApi>::onExecution(const std::shared_ptr<Event> &event_) {

}

template<typename TORDApi>
void BreakOutStrategy<TORDApi>::onTrade(const std::shared_ptr<Event> &event_) {

}

template<typename TORDApi>
void BreakOutStrategy<TORDApi>::onBBO(const std::shared_ptr<Event> &event_) {

}

template<typename TORDApi>
BreakOutStrategy<TORDApi>::~BreakOutStrategy() {

}

template<typename TORDApi>
BreakOutStrategy<TORDApi>::BreakOutStrategy(std::shared_ptr<MarketDataInterface> mdPtr_,  std::shared_ptr<TORDApi> od_)
:   Strategy<TORDApi>(mdPtr_, od_) {

}


#endif //TRADINGO_BREAKOUTSTRATEGY_H
