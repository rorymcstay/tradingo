#!/bin/bash
source "$(dirname ${BASH_SOURCE[0]})/profile.env"

replay_tradingo_on() {

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
    DATESTR=$1 \
    RUN_ID=$run_id \
    INSTALL_LOCATION=$INSTALL_LOCATION \
    MOVING_AVG_CALLBACK=${MOVING_AVG_CALLBACK:-false} \
    MOVING_AVG_INTERVAL=${MOVING_AVG_INTERVAL:-1000} \
    MOVING_AVG_SHORT_TERM=${MOVING_AVG_SHORT_TERM:-1000} \
    MOVING_AVG_LONG_TERM=${MOVING_AVG_LONG_TERM:-8000} \
    SYMBOL=${SYMBOL:-XBTUSD} \
        envsubst < $INSTALL_LOCATION/etc/config/replayTradingo.cfg  \
    > $REPLAY_STORAGE/replay.cfg
    cat /tmp/replay.cfg

    mkdir -p /tmp/log/tradingo/replay/$run_id
    replayTradingo --config $REPLAY_STORAGE/replay.cfg &> "/tmp/replay_$1.log"
    aws s3 sync "$REPLAY_STORAGE" "s3://$BUCKET_NAME/replays/"
}
replay_tradingo_on $1
