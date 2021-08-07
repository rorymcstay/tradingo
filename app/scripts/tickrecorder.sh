#!/bin/sh

tick_record() {
    tickRecorder --config /home/rory/tickRecorder.cfg
    return $? 
}
tick_record
