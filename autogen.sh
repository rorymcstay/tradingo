#!/bin/sh

rm -r build
rm compile_commands.json

mkdir build

(
cd build

cmake -DCPPREST_ROOT=/usr/ \
    -DCMAKE_COMPILE_COMMANDS=1 \
    "@" \
    ../
)

ln -s build/compile_commands.json compile_commands.json

(
cd build && make install -j6
)
