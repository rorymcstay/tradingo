#!/bin/sh

source "$(dirname ${BASH_SOURCE[0]})/profile.env"

tick_record() {
    tickRecorder --config $INSTALL_LOCATION/etc/config/tickRecorder.cfg
    return $? 
}
tick_record
