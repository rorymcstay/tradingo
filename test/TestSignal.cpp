//
// Created by Rory McStay on 06/09/2021.
//



TEST(TestSignal, test_signal) {
    std::ifstream quoteFile;
    quoteFile.open("data/quotes_XBTUSD.json");
    LOGINFO( quote->toJson().serialize());

}