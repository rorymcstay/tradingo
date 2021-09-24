# Tradingo

Tradingo is a bitmex derivatives exchange trading bot. It trades a single symbol. Strategy is inherited and registered
in the strategies file. It then may be run in the binary specifying the registry method in the config file.

A strategy can register signals, which are implemented by inheriting the Signal class. Signals are either on a timer callback
and read quotes, or are on a callback on the MarketData to any event.

The strategy has a set of allocations which is a continuous data structure indexed by price. The strategy modifies allocations along this index based on how
the size, direction and at what level to order at. Active orders are amended should an allocation at a price level change. One price level per order.

Tradingo docker image provides all the functionality including replay, benchmarking and trading. For the former two, data
is retrived from s3. Replay output files are stashed into s3 after, along with the config.

AWS ECS tasks are defined from the docker-compose.yml file using the `ecs-cli compose` tool. It is parameterised by environment
variables and a python script spawns ecs taks based on the market data it finds in an s3 bucket.


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

rory@ldtradingoapp:~/src/tradingo/build$ replay_tradingo_on() {  export DATESTR=$1; envsubst < /usr/etc/config/replayTradingo.cfg  > /tmp/replay.cfg; cat /tmp/replay.cfg; replayTradingo --config /tmp/replay.cfg &> /tmp/log/replay_$1.log; }
rory@ldtradingoapp:~/src/tradingo/build$ for date in $(ls /data/tickRecorder/storage); do replay_tradingo_on $date; done

# Replay Modes
Cmake option `-DREPLAY_MODE=1` enables time control in `Signal`. It may be paired with the following configuration items for a signal `<name>`.

* `<name>-callback`: 
* `override-signal-callback`:
* `realtime`: 
