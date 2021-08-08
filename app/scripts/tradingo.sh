#!/bin/sh

source "$(dirname ${BASH_SOURCE[0]})/../etc/profile.env"

tradingo --config /etc/config/tradingo.cfg
