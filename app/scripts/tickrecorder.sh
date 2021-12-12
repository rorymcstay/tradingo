#!/bin/bash

source "$(dirname ${BASH_SOURCE[0]})/profile.env"

tick_record() {
    mkdir -p $STORAGE
    config_file=$STORAGE/tickRecorder.cfg
    STORAGE=$STORAGE \
    INSTALL_LOCATION=$INSTALL_LOCATION \
    SYMBOL=$SYMBOL \
        envsubst < $INSTALL_LOCATION/etc/config/tickRecorder.cfg  \
    >> $config_file
    tickRecorder --config $config_file
    return $? 
}
tick_record
