import unittest
import time
import json

from tradingo_optimizer.test_case import TradingoTestCase, EventType

from tradingo_optimizer.test_case import TestEnvMessageSerialiser, \
        JSONTestSerialiser

#serialiser = TestEnvMessageSerialiser
serialiser = JSONTestSerialiser


class TradingoReplayTestCases(TradingoTestCase, serialiser):

    symbol = 'XBTUSD'

    def testing_order_value_exceeds_balance(self):
        has_balance = True
        iteration = 0
        while has_balance:
            iteration += 1
            try:
                current_quote = self.get_current_quote()
                price = current_quote.bid_price
                self.assert_margin(description=f'pre order {iteration=}')
                self.assert_position(description=f'pre order {iteration=}')
                order = self.place_order(price, 300.0)
                self.assert_margin(description=f'post order {iteration=}')
                self.assert_position(description=f'post order {iteration=}')
                self.await_execution(order, 'Filled')
                self.assert_margin(description=f'post order fill {iteration=}')
                self.assert_position(description="post order fill assertion {iteration=}")
            except Exception as ex:
                order = sorted(self.api.order.order_get_orders(filter=json.dumps({'ordStatus': 'Rejected'})), key = lambda o: o.timestamp)[-1]
                has_balance = False
            self._add_event(order, EventType.OUT_EVENT)
            self.record_instrument()
            time.sleep(1)
        has_balance = True
        iteration = 0
        while has_balance:
            iteration += 1
            try:
                current_quote = self.get_current_quote()
                self.assert_margin(description=f'pre sell order {iteration=}')
                self.assert_position(description=f'pre sell order {iteration=}')
                price = current_quote.ask_price
                order = self.place_order(price, -300.0)
                self.assert_margin(description=f'post order reducing position {iteration=}')
                self.assert_position(description=f"post order reducing position {iteration=}")
                self.await_execution(order, 'Filled')
                self.assert_margin(description=f'post fill reducing position {iteration=}')
                self.assert_position(description=f"post fill reducing position {iteration=}")

            except Exception as ex:
                order = sorted(self.api.order.order_get_orders(filter=json.dumps({'ordStatus': 'Rejected'})), key = lambda o: o.timestamp)[-1]
                has_balance = False
            self._add_event(order, EventType.OUT_EVENT)
            self.record_instrument()
            time.sleep(1)


    def testing_enter_position_extend_and_then_exit(self):
        # Enter into a position of 100
        current_quote = self.get_current_quote()
        price = current_quote.bid_price
        order = self.place_order(price, 100.0)
        self.record_instrument()
        self.assert_margin(description="Margin post order -> 100")
        self.assert_position(description="Position post order -> 100")
        self.await_execution(order, 'Filled')
        self.record_instrument()
        self.assert_margin(description="Margin post fill to qty 100")
        self.assert_position(description="Position post fill to qty 100")

        # Extend it to 200
        current_quote = self.get_current_quote()
        price = current_quote.bid_price
        order = self.place_order(price, 100.0)
        self.record_instrument()
        self.assert_margin(description="Margin post order -> 200")
        self.assert_position(description="Position post order -> 200")
        self.await_execution(order, 'Filled')
        self.record_instrument()
        self.assert_margin(description="Margin post fill to qty 200")
        self.assert_position(description="Position post fill to qty 200")

        # Reduce it to 100
        current_quote = self.get_current_quote()
        price = current_quote.ask_price
        order = self.place_order(price, -100.0)
        self.assert_margin(description="Margin post order -> 100")
        self.assert_position(description="Position post order -> 100")
        self.await_execution(order, 'Filled')
        self.record_instrument()
        self.assert_margin(description="Margin post fill -> 100")
        self.assert_position(description="Position post fill -> 100")

        # Close the position
        current_quote = self.get_current_quote()
        price = current_quote.ask_price
        order = self.place_order(price, -100.0)
        self.assert_margin(description="Margin post order -> close out")
        self.assert_position(description="Position post order -> close out")
        self.await_execution(order, 'Filled')
        self.record_instrument()
        self.assert_margin(description="Margin post fill -> close out")
        self.assert_position(description="Position post fill -> close out")


    @unittest.skip("long running")
    def test_in_and_out(self):

        for i in range(10):
            current_quote = self.get_current_quote()
            if i % 2 == 0:
                mtp = -1
                price = current_quote.bid_price
            else:
                mtp = 1
                price = current_quote.ask_price
            # Enter into a position of 100
            order = self.place_order(price, mtp*100.0)
            self.await_execution(order, 'Filled')
            self.assert_margin()
            self.assert_position()
            time.sleep(0.3)


if __name__ == '__main__':
    unittest.main()
