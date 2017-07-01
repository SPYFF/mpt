#!/bin/bash

INTERFACE=$1
MARK=$2

if [ "$MARK" == "down" ]; then
	ifdown $INTERFACE
else
	ifup $INTERFACE
	sleep 10
fi
