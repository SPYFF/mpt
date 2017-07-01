#!/bin/sh

OP=$1
IP=$2
INTERFACE=$3

ip addr $OP $IP dev $INTERFACE
