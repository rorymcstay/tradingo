#!/bin/bash
source "$(readlink -f $(dirname ${BASH_SOURCE[0]}))/profile.env"
mkdir -p /home/rory/tickRecorder/log/
aws s3 sync $TICK_STORAGE s3://$BUCKET_NAME/tickRecorder/storage &> /home/rory/tickRecorder/log/sync_with_s3_`date +%Y%m%d`.log
