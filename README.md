# Issues


1. Insufficient funds not handled gracefully -- This needs to be a strategy event to handle.

         ApiException: {"error":{"message":"Account has insufficient Available Balance, 112069 XBt required","name":"ValidationError"}} status_code='400'
   
2. Changing sides, new orders are invalid and order not found for cancel
   
         2021-08-08 15-43-44.222 [Info] (placeAllocations) 	Processing allocation targetDelta=-2000 currentSize=0 price=33080.5  |Strategy.h:139
         2021-08-08 15-43-44.222 [Info] (placeAllocations) 	Price level is occupied order->getPrice()='33080.5', order->getOrderQty()='2000', currentOrder->getOrderQty()='0', currentOrder->getPrice()='33080.5', currentOrder->getOrderID()='48',  |Strategy.h:147
         2021-08-08 15-43-44.222 [Info] (placeAllocations) 	Chaging sides |Strategy.h:158
         2021-08-08 15-43-44.225 [Info] (order_cancel) 	ClOrdID=MCST29  not found to cancel. |TestOrdersApi.cpp:76
         2021-08-08 15-43-44.225 [Info] (placeAllocations) 	Error placing new orders {"clOrdID":"MCST331","cumQty":0,"leavesQty":2000,"ordStatus":"New","orderID":"354","orderQty":2000,"origClOrdID":"MCST29","price":33080.5,"side":"Sell","symbol":"XBTUSD","timestamp":"2021-07-09T04:07:47.629Z"}ex_.what()='Must specify only one of orderID or origClOrdId',  |Strategy.h:256
         2021-08-08 15-43-44.230 [Info] (operator()) 	Allocations::cancel: Cancelling allocationDelta alloc_->getPrice()='33080.5',  |Allocations.cpp:113
         2021-08-08 15-43-44.236 [Info] (placeAllocations) 	Allocations have been reflected. amend=0 new=1 cancel=1  |Strategy.h:262

# Time control
Compiling with -DREPLAY_MODE, enables the control of time.

1) realtime mode, replay will play over the file at speed dictated in datafiles
2) non-realtime mode, file is passed with no time

A signal can be either callback or timed
In realtime, all signals are callback and determine if they are to evaluate themselves
based on the market data update.

In replay mode, must be used
signal-callback=true
realtime=true

