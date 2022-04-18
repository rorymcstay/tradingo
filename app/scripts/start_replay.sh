#!/bin/bash
source "$(dirname ${BASH_SOURCE[0]})/profile.env"

export LOG_LEVEL=${LOG_LEVEL:-"warning"}

replay_tradingo_on() {

    # params
    export DATESTR=$1

    if [[ ! $DATESTR ]]; then
        echo "Datestring must not be empty"
        return 1
    fi
    mkdir -p "$TICK_STORAGE/$DATESTR"
    tick_location="$TICK_STORAGE/$DATESTR/"

    # exit on first failure
    set -e

    # do this and do not fail if unsuccesful.
    if [[ ! -f $tick_location/instruments_$SYMBOL.json ]]; then
        aws s3 cp "s3://$BUCKET_NAME/tickRecorder/storage/$1/instruments_$SYMBOL.json" $tick_location
    fi
    if [[ ! -f $tick_location/quotes_$SYMBOL.json ]]; then
        aws s3 cp "s3://$BUCKET_NAME/tickRecorder/storage/$1/quotes_$SYMBOL.json" $tick_location
    fi
    if [[ ! -f $tick_location/trades_$SYMBOL.json ]]; then
        aws s3 cp "s3://$BUCKET_NAME/tickRecorder/storage/$1/trades_$SYMBOL.json" $tick_location
    fi

    common_config=$REPLAY_STORAGE/$RUN_ID/common.json
    config_file=$REPLAY_STORAGE/$RUN_ID/replayTradingo.json
    echo "config_file="$config_file
    export STORAGE=$REPLAY_STORAGE
    mkdir -p $STORAGE/$RUN_ID

    TICK_STORAGE=$TICK_STORAGE \
    DATESTR=$DATESTR \
    LIB_NAME_PREFIX="test" \
    INSTALL_LOCATION=$INSTALL_LOCATION \
        envsubst < $INSTALL_LOCATION/etc/config/replayTradingo.json \
    > $config_file

    populate_common_params $common_config
    # run the replay in the RUN_ID directory to capture core files.

    set +e
    cd $STORAGE/$RUN_ID
    echo replayTradingo --config $config_file "${@:2}"
    replayTradingo --config $common_config --config $config_file "${@:2}"
    ls -l $STORAGE/$RUN_ID/
    aws s3 sync "$STORAGE/$RUN_ID" "s3://$BUCKET_NAME/replays/$RUN_ID"

}
replay_tradingo_on "$@"
