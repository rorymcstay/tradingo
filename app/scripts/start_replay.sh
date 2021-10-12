#!/bin/bash
source "$(dirname ${BASH_SOURCE[0]})/profile.env"

trade_date=$1

replay_tradingo_on() {
    # exit on first failure
    set -e

    # params
    export DATESTR=$1
    if [[ ! $DATESTR ]]; then
        echo "Datestring must not be empty"
        return 1
    fi
    mkdir -p "$TICK_STORAGE/$DATESTR"
    aws s3 cp "s3://$BUCKET_NAME/tickRecorder/storage/$1/quotes_$SYMBOL.json" "$TICK_STORAGE/$DATESTR/"
    aws s3 cp "s3://$BUCKET_NAME/tickRecorder/storage/$1/trades_$SYMBOL.json" "$TICK_STORAGE/$DATESTR/"

    config_file=$REPLAY_STORAGE/$RUN_ID/tradingo.cfg
    echo "config_file="$config_file
    export STORAGE=$REPLAY_STORAGE
    mkdir -p $STORAGE/$RUN_ID

    echo "# Tradingo Replay" > $config_file
    echo "# run_id=${RUN_ID}" >> $config_file
    echo "# replay config" >> $config_file
    echo "" >> $config_file
    TICK_STORAGE=$TICK_STORAGE \
    DATESTR=$DATESTR \
    REALTIME=${REALTIME:-false} \
    INSTALL_LOCATION=$INSTALL_LOCATION \
        envsubst < $INSTALL_LOCATION/etc/config/replayTradingo.cfg  \
    >> $config_file

    populate_common_params $config_file
    populate_strategy_params $INSTALL_LOCATION/etc/config/strategy/${STRATEGY}.cfg $config_file
    cat $config_file
    # run the replay
    cd $STORAGE
    set +e
    replayTradingo --config $config_file
    aws s3 sync "$STORAGE" "s3://$BUCKET_NAME/replays/"
}
replay_tradingo_on $trade_date
