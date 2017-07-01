#!/bin/bash

METHOD=$1
VERSION=$2
IP=$3
PREFIX=$4
GATEWAY=$5

if [ "$METHOD" == "add" ]; then
  if [ "$VERSION" == "4" ]; then 
    route add -net $IP/$PREFIX gw $GATEWAY
    echo "route add -net $IP/$PREFIX gw $GATEWAY"
  elif [ "$VERSION" == "6" ]; then 
    ip -6 route add $IP/$PREFIX via $GATEWAY
    echo "ip -6 route add $IP/$PREFIX via $GATEWAY"
  fi
elif [ "$METHOD" == "del" ]; then
  if [ "$VERSION" == "4" ]; then 
    route del -net $IP/$PREFIX gw $GATEWAY
  elif [ "$VERSION" == "6" ]; then 
    ip -6 route del $IP/$PREFIX via $GATEWAY
  fi
fi