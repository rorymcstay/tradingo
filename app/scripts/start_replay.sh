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

    run_id=${RUN_ID:-"$run_date.$time_since_midnight"}
    mkdir -p $REPLAY_STORAGE/$run_id.${DATESTR}
    config_file="$REPLAY_STORAGE/$run_id.${DATESTR}/replay.cfg"
    echo $config_file

    TICK_STORAGE=$TICK_STORAGE \
    REPLAY_STORAGE=$REPLAY_STORAGE \
    DATESTR=$DATESTR \
    RUN_ID=$run_id \
    LOG_LEVEL=${LOG_LEVEL:-info} \
    REALTIME=${REALTIME:-false} \
    INSTALL_LOCATION=$INSTALL_LOCATION \
        envsubst < $INSTALL_LOCATION/etc/config/replayTradingo.cfg  \
    > $config_file
    cat $config_file

    populate_strategy_params $INSTALL_LOCATION/etc/config/strategy/${STRATEGY}.cfg >> $config_file
    cat $config_file
    # run the replay
    replayTradingo --config $config_file
    aws s3 sync "$REPLAY_STORAGE" "s3://$BUCKET_NAME/replays/"
}
replay_tradingo_on $trade_date
