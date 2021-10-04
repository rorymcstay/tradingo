#!/bin/bash
source "$(dirname ${BASH_SOURCE[0]})/profile.env"

trade_date=$1

replay_tradingo_on() {
    # exit on first failure
    set -e

    set -x
    # params
    export DATESTR=$1
    if [[ ! $DATESTR ]]; then
        echo "Datestring must not be empty"
        return 1
    fi

    mkdir -p "$TICK_STORAGE/$DATESTR"
    aws s3 cp "s3://$BUCKET_NAME/tickRecorder/storage/$1/quotes_$SYMBOL.json" "$TICK_STORAGE/$DATESTR/"
    aws s3 cp "s3://$BUCKET_NAME/tickRecorder/storage/$1/trades_$SYMBOL.json" "$TICK_STORAGE/$DATESTR/"
    run_date=$(date -I)
    eval "$(date +'today=%F now=%s')"
    midnight=$(date -d "$today 0" +%s)
    time_since_midnight="$((now - midnight))"
    run_id=$run_date.$time_since_midnight

    mkdir -p $REPLAY_STORAGE/$run_id.${DATESTR}
    config_file="$REPLAY_STORAGE/$run_id.${DATESTR}/replay.cfg"

    export TICK_STORAGE=$TICK_STORAGE
    export REPLAY_STORAGE=$REPLAY_STORAGE \
    export DATESTR=$DATESTR \
    export RUN_ID=$run_id \
    export LOG_LEVEL=${LOG_LEVEL:-info}
    export REALTIME=${REALTIME:-false}
    export INSTALL_LOCATION=$INSTALL_LOCATION \
        envsubst < $INSTALL_LOCATION/etc/config/replayTradingo.cfg  \
    > $config_file
    cat $config_file

    echo $(populate_strategy_params $INSTALL_LOCATION/etc/config/strategy/${STRATEGY}.cfg) >> $config_file
    cat $config_file
    # run the replay
    replayTradingo --config $config_file
    aws s3 sync "$REPLAY_STORAGE" "s3://$BUCKET_NAME/replays/"
}
replay_tradingo_on $trade_date
