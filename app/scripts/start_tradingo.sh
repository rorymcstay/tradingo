#!/bin/bash
source "$(dirname ${BASH_SOURCE[0]})/profile.env"

start_tradingo() {

    run_date=$(date -I)
    eval "$(date +'today=%F now=%s')"
    midnight=$(date -d "$today 0" +%s)
    time_since_midnight="$((now - midnight))"
    run_id=$run_date.$time_since_midnight
    STORAGE=${STORAGE:-/data/tradingo}
    config_file=$STORAGE/${run_id}/tradingo.cfg

    # build the config file
    set -x
    STORAGE=$storage \
    CONNECTION_STRING=${CONNECTION_STRING} \
    BASE_URL=${BASE_URL} \
    LOG_DIR=${LOG_DIR} \
    API_KEY=${API_KEY} \
    API_SECRET=${API_SECRET} \
    STORAGE=${STORAGE} \
    LOG_LEVEL=${LOG_LEVEL:-info} \
        envsubst < $INSTALL_LOCATION/etc/config/strategy/${STRATEGY}.cfg  \
    > $config_file

    populate_strategy_params $INSTALL_LOCATION/etc/config/strategy/${STRATEGY}.cfg >> $config_file
    cat $config_file

    # start tradingo
    tradingo --config $config_file
    aws s3 sync "$STORAGE" "s3://$BUCKET_NAME/tradingo/storage"
    aws s3 sync "$LOG_DIR" "s3://$BUCKET_NAME/tradingo/logs"
}
replay_tradingo_on $1
