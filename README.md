# Issues
1. json exception thrown whilst placing orders.

        2021-07-11 17-47-12.074 [Info] (placeAllocations) 	Processing allocation targetDelta=200 currentSize=-100 price=34049.5  |Strategy.h:143
        2021-07-11 17-47-12.075 [Info] (placeAllocations) 	Price level is occupied order->getPrice()='34049.5', order->getOrderQty()='100', currentOrder->getOrderID()='160b27cd-2589-48e8-ad71-891c9c25e7c4',  |Strategy.h:150
        2021-07-11 17-47-12.075 [Info] (placeAllocations) 	Chaging sides |Strategy.h:158
        terminate called after throwing an instance of 'web::json::json_exception'
        what():  * Line 1, Column 2 Syntax error: Malformed token

2. no OrigClOrdID field on model::Order object.

         2021-07-11 17-55-35.345 [Info] (placeAllocations) 	Amending up |Strategy.h:172
         2021-07-11 17-55-35.348 [Info] (placeAllocations) 	Allocations have been reflected. amend=1 new=0 cancel=0  |Strategy.h:220
         2021-07-11 17-55-35.467 [Info] (updateFromTask) 	APIException caught failed to action on order: apiException.what()='error calling order_amendBulk: Bad Request', Reason={"error":{"message":"origClOrdID required when sending clOrdID.","name":"ValidationError"}}  |Strategy.h:248

3. Duplicate ClOrdIDs being sent.
         
         2021-07-11 19-42-15.184 [Info] (placeAllocations) 	Allocations have been reflected. amend=0 new=1 cancel=0  |Strategy.h:220
         ApiException: {"error":{"message":"Duplicate clOrdID","name":"HTTPError"}} status_code='400'

4. Insufficient funds not handled gracefully -- This needs to be a strategy event to handle.

         ApiException: {"error":{"message":"Account has insufficient Available Balance, 112069 XBt required","name":"ValidationError"}} status_code='400'
