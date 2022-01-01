#!/bin/bash
source "$(dirname ${BASH_SOURCE[0]})/profile.env"

start_tradingo() {
    # exit on first failure

    set -e
    ping -c 5 testnet.bitmex.com
    ping -c 5 bitmex.com
    config_file=$STORAGE/${RUN_ID}/tradingo.cfg
    mkdir -p $STORAGE/$RUN_ID

    echo "# Tradingo Start config" > $config_file
    echo "# run_id=${RUN_ID}" >> $config_file
    echo "# tradingo config" >> $config_file
    CONNECTION_STRING=${CONNECTION_STRING} \
    BASE_URL=${BASE_URL} \
    API_KEY=${API_KEY} \
    API_SECRET=${API_SECRET} \
        envsubst < $INSTALL_LOCATION/etc/config/tradingo.cfg  \
    >> $config_file
    populate_common_params $config_file
    populate_strategy_params $INSTALL_LOCATION/etc/config/strategy/${STRATEGY}.cfg $config_file
    cat $config_file

    # start tradingo
    cd $STORAGE/$RUN_ID
    set +e
    tradingo --config $config_file
    aws s3 sync "$STORAGE/" "s3://$BUCKET_NAME/tradingo/"
}
start_tradingo
