#!/bin/bash

METHOD=$1
VERSION=$2
IP=$3
PREFIX=$4
GATEWAY=$5

if [ "$METHOD" == "add" ]; then
  if [ "$VERSION" == "4" ]; then 
    #route add -net $IP/$PREFIX gw $GATEWAY
    #echo "route add -net $IP/$PREFIX gw $GATEWAY"
    ip route add table 9999 $IP/$PREFIX via $GATEWAY
    echo "ip route add table 9999 $IP/$PREFIX via $GATEWAY"
  elif [ "$VERSION" == "6" ]; then 
    ip -6 route add table 9999 $IP/$PREFIX via $GATEWAY
    echo "ip -6 route add table 9999 $IP/$PREFIX via $GATEWAY"
  fi
elif [ "$METHOD" == "del" ]; then
  while ip rule delete from 0/0 to 0/0 table 9999 2>/dev/null; do true; done
  if [ "$VERSION" == "4" ]; then 
    #route del -net $IP/$PREFIX gw $GATEWAY
    ip route del table 9999 $IP/$PREFIX via $GATEWAY
    echo "ip route add table 9999 $IP/$PREFIX via $GATEWAY"
  elif [ "$VERSION" == "6" ]; then 
    ip -6 route del table 9999 $IP/$PREFIX via $GATEWAY
  fi
fi
