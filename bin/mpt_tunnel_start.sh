#!/bin/sh

INTERFACE=$2
IP=$3
MTU=$4

case "$1" in
    ipv4)
        #ifconfig $INTERFACE $IP mtu $MTU
	ip addr add $IP dev $INTERFACE
        ;;
    ipv6)
        #ifconfig $INTERFACE inet6 add $IP
	ip addr add $IP dev $INTERFACE
        ;;
    *)
        echo "Wrong parameter $1"
        exit 1
        ;;
esac

ip link set $INTERFACE up mtu $MTU
ip rule add from all lookup 9999
ip rule add oif $INTERFACE uidrange 0-99999 lookup 9999

exit 0
