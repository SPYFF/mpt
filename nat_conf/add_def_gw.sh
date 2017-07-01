#!/bin/bash

echo "route add -net default gw $1 dev $2"

route add -net default gw $1 dev $2