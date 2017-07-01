#!/bin/sh

INTERFACE=$2
IP=$3
MTU=$4

case "$1" in
    ipv4)
        ifconfig $INTERFACE $IP mtu $MTU
        ;;
    ipv6)
        ifconfig $INTERFACE inet6 add $IP
        ;;
    *)
        echo "Wrong parameter $1"
        exit 1
        ;;
esac

exit 0
