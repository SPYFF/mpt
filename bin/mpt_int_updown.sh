#!/bin/bash

INTERFACE=$1
MARK=$2

if [ "$MARK" == "down" ]; then
	ip link set dev $INTERFACE down
else
	ip link set dev $INTERFACE up
	sleep 10
fi
