#!/bin/bash

for f in eth0 eth1 eth2;do
    ethtool --offload $f rx off tx off
    ethtool -K $f gso off
done
