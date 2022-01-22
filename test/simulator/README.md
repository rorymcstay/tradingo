To build run

    ./autogen-aero.sh

to run tests

    ./bin/test_orderbook

to run trading application

    ./bin/book

logs of application are written to XYZ_exec



Question 1: 
* Order Add complexity 
    1) LookUp trader - O(1) -> worst case add new trader O(n) ... could be simplified to reject unregistered and more realistic ... n in number of traders.
    2) Insert exec message into queue -> O(1)
    3) Insert order into hashmap -> O(1) 
    4) Insert into ordered Queue -> O(log n) ... n in number of orders of that side
    5) Update level O(1) ... vector of levels
    => O(T) + O(log n) where T is number of traders and n is number of orders of that side.

* Order cancel complexity
    1) Lookup trader -> O(1)
    2) Lookup trader again -> O(1)
    3) Lookup order -> O(1)
    4) Update level O(1) ... vector of levels
    => constant time complexity

Use of hashmap in theory has worst case complexity of O(1), but in this case generally we wont have collisions as we have unique Order/Trade IDs we can
rely on.

Question 2:

Orders could be stored in a level class, which is an ordered binary tree describing queue position
Vector of levels indexed by ticksize increments could then be looked up in constant time giving
good lookup of book characteristics.

Data could be stored partitioned by exec type and order status, so that we could reduce the amount
of space required to store these textual fields.

