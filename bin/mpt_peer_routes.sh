#!/bin/bash
# ***************** Csak IPv4 egyelore *****************
# beallitja/torli a path-on a peer-hez vezeto routing tabla bejegyzest

METHOD=$1
VERSION=$2
PEER_IP=$3
GATEWAY=$4
IFACE=$5

echo "peer_routes.sh"

if [ "$METHOD" == "add" ]; then
  if [ "$VERSION" == "4" ]; then 
    #route add -host $PEER_IP gw $GATEWAY dev $IFACE
    #echo "route add -host $PEER_IP gw $GATEWAY dev $IFACE"
    ip route add table 9999 $PEER_IP via $GATEWAY dev $IFACE
    echo "ip route add table 9999 $PEER_IP via $GATEWAY dev $IFACE"
  elif [ "$VERSION" == "6" ]; then 
    route -A inet6 add table 9999 ${PEER_IP}/128 gw $GATEWAY dev $IFACE
    echo "route -A inet6 add table 9999 ${PEER_IP}/128 gw $GATEWAY dev $IFACE"
  fi
elif [ "$METHOD" == "del" ]; then
  while ip rule delete from 0/0 to 0/0 table 9999 2>/dev/null; do true; done
  if [ "$VERSION" == "4" ]; then 
    #route del -host $PEER_IP gw $GATEWAY dev $IFACE
    #echo "route del -host $PEER_IP gw $GATEWAY dev $IFACE"
    ip route del table 9999 $PEER_IP via $GATEWAY dev $IFACE
    echo "ip route del table 9999 $PEER_IP via $GATEWAY dev $IFACE"
  elif [ "$VERSION" == "6" ]; then 
    route -A inet6 del table 9999 ${PEER_IP}/128 gw $GATEWAY dev $IFACE
    echo "route -A inet6 del table 9999 ${PEER_IP}/128 gw $GATEWAY dev $IFACE"
  fi
fi
