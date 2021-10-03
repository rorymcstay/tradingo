#!/bin/bash
source "$(dirname ${BASH_SOURCE[0]})/profile.env"

replay_tradingo_on() {

    # params
    DATESTR=$1

    mkdir -p "$TICK_STORAGE/$1"
    aws s3 cp "s3://$BUCKET_NAME/tickRecorder/storage/$1/quotes_$SYMBOL.json" "$TICK_STORAGE/$1/"
    aws s3 cp "s3://$BUCKET_NAME/tickRecorder/storage/$1/trades_$SYMBOL.json" "$TICK_STORAGE/$1/"
    run_date=$(date -I)
    eval "$(date +'today=%F now=%s')"
    midnight=$(date -d "$today 0" +%s)
    time_since_midnight="$((now - midnight))"
    run_id=$run_date.$time_since_midnight

    set -x
    TICK_STORAGE=$TICK_STORAGE
    REPLAY_STORAGE=$REPLAY_STORAGE \
    DATESTR=$DATESTR \
    RUN_ID=$run_id \
    LOG_LEVEL=${LOG_LEVEL:-info}
    REALTIME=${REALTIME:-false}
    INSTALL_LOCATION=$INSTALL_LOCATION \
        envsubst < $INSTALL_LOCATION/etc/config/replayTradingo.cfg  \
    > $REPLAY_STORAGE/${RUN_ID}.${DATESTR}/replay.cfg


    populate_strategy_params $INSTALL_LOCATION/etc/config/strategy/${STRATEGY}.cfg >> $config_file
    cat $REPLAY_STORAGE/${RUN_ID}.${DATESTR}/replay.cfg
    mkdir -p /tmp/log/tradingo/replay/$run_id
    # run the replay
    replayTradingo --config $REPLAY_STORAGE/${RUN_ID}.${DATESTR}/replay.cfg
    aws s3 sync "$REPLAY_STORAGE" "s3://$BUCKET_NAME/replays/"
}
replay_tradingo_on $1
