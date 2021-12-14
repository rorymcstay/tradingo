#!/bin/bash
mkdir -p /home/rory/tickRecorder/log/
aws s3 sync /data/tickRecorder/storage/ s3://tick-storage/tickRecorder/storage &> /home/rory/tickRecorder/log/sync_with_s3_`date +%Y%m%d`.log
