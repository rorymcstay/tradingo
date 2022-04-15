# Tradingo

Tradingo is a bitmex derivatives exchange trading bot.
It trades currently is only tested with a single symbol
and its main strategy entity is restricted as such.
Testing and market interface components are generally built
agnositc to such requirement.

Trading strategies are implemented by inheriting from the
Strategy class. There are three event handlers to implement
`onExecution`, `onTrade` and `onQuote`. Each react to based
on what they are named.
Strategies can register implementations of the `Signal` class,
they will then evaluate before the strategies on either a callback
timer, or on quote event.

Strategies must be registered in the `src/strategies/Strategies.cpp`
file. A config file must then be created such as
    ```
    factoryMethod=RegisterBreakOutStrategy
    ... other config needed in strategy implementation ...
    ```
in `app/etc/config/strategy/<some_name>.cfg`. It may be run in the
`tradingo` process by setting `STRATEGY=<some_name>` in the current
environment.

The strategy has a set of allocations which is a continuous data structure 
indexed by price. The strategy modifies allocations along this index based 
on how the size, direction and at what level to order at. Active orders 
are amended should an allocation at a price level change. One price level 
per order.

Tradingo functionalities are packaged in two docker images. One is intended
for running live trading activities, and the other is for running replays
and benchmarks. For the replays and benchmarks, data is retrived from s3, for
the trade date set in the environment.
Replay output files are stashed into s3 after, along with the config used. If
there is a core generated, that is also delivered to s3.


# Time control
Compiling with -DREPLAY_MODE, enables the control of time.

1) realtime mode, replay will play over the file at speed dictated in datafiles
2) non-realtime mode, file is passed with no time

A signal can be either callback or timed
In realtime, all signals are callback and determine if they are to evaluate
themselves based on the market data update.

The main difference for seperation of the two docker images is the compilation mode.

In replay mode, must be used
signal-callback=true
realtime=true

# Building
Running `autogen.sh` should build tradingo on ubuntu platforms.
# Running tradingo locally
1. compile and install tradingo.
2. Do
   1. 
