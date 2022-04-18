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
Strategies are selected with the 'factoryMethod' key in config.

The strategy has a set of allocations which is a continuous data structure 
indexed by price. The strategy modifies allocations along this index based 
on how the size, direction and at what level to order at. Active orders 
are amended should an allocation at a price level change. One price level 
per order.

## Configuration
Config is provided with

1) `--config` option. Specified zero or more times. Each config is applied
    to the defaults in order. Path is to a json or cfg file.
2) `--config-json` flag to provide json string directly on command line.

## Initial replay settings
When running a replay, json string may be provided for `--initial-position`
and `--initial-margin`.

Tradingo functionalities are packaged in two docker images. One for backtesting
and one for trading, both use `tradingo-base` docker image for its build dependencies.

# Time control
Compiling with `-DREPLAY_MODE`, simulates the time the strategy has based on
replay data provided.

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

# Tests
Run tests with `make test` in the build directory. Compile with `-DWITH_REGRESSION_TESTS`
to enable simulated replay tests generated with `tradingo-optimizer`.
`testTradingo` and `testTradino-lr` are the main unit test binaries.
