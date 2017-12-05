# MPT - Multi-Path Tunnel

## What is this?
This is a small tool for network layer multipath. Works like a multpath VPN application: create a logical tunnel interface over the native phisical interfaces and share the tunnel's traffic between them.

The datapath encapsulation of this application is completely standard! See [RFC 8086](https://tools.ietf.org/html/rfc8086) for the details of the GRE-in-UDP architecture. We have an additonal multipath signalig, which is not standard, but we have some initial effort on that (see [this internet draft](https://www.ietf.org/internet-drafts/draft-lencse-tsvwg-mpt-00.txt)). So this is a GRE-in-UDP VPN application with multipath extension.

## Compilation
1. Install the dependencies: 
```
$ sudo apt install gcc-multilib libssl-dev net-tools iproute make libev-dev
```
2. Clone the repository
3. Compile
```
$ cd mpt
$ make
```

## Configuration
Here is a guide for configure this application. There are some steps which makes this readme too long so please visit the link below.
More information: [http://irh.inf.unideb.hu/user/szilagyi/sites/default/files/mpt/usermanual.pdf](http://irh.inf.unideb.hu/user/szilagyi/sites/default/files/mpt/usermanual.pdf)

## Experimental feature
This branch of the software contains a very simple layer 4 protocol based path mapping feature. This brings the possibility to override the basic path weight based packet distribution for special TCP or UDP ports. 

Example path configurations (see the usermanual for details of the basic config):
```
[path_0]
interface_name   = eth1
ip_ver           = 4
public_ipaddr    = 10.1.1.2
gw_ipaddr        = 10.1.1.2
remote_ipaddr    = 10.1.1.3
keepalive_time   = 5
dead_time        = 11
weight_out       = 10
cmd_default      = 1
status           = 0
tcp_dst		 = 80 443 8080 23456
tcp_src		 = 60000 65000
udp_dst		 = 53
udp_src		 = 1568 10000

[path_1]
interface_name   = eth2
ip_ver           = 4
public_ipaddr    = 10.2.2.2
gw_ipaddr        = 10.2.2.2
remote_ipaddr    = 10.2.2.3
keepalive_time   = 5
dead_time        = 11
weight_out       = 10
status           = 0
tcp_dst		 = 21 22 68
```

In this example above every packets on the tunnel with 80, 443, 8080 and 23456 TCP destionation port goes through on the eth1 interface. The DNS resolutions goes out from the same interface as described in the 53 UDP destination port's line.
* `tcp_dst` - packets with the specified TCP destination ports goes out on this path
* `tcp_src` - packets with the specified TCP source ports goes out on this path
* `udp_dst` - packets with the specified UDP destination ports goes out on this path
* `udp_src` - packets with the specified UDP source ports goes out on this path

## Running the software
Start the `mptsrv` binary in root mode. Double check if the configuration is correct and match with the real world test setup.

## Resources
The intent of the software is nework layer multipath research. We have a few papers with the capabilities of the software, including new results in the research area:
* Aggregating throughput of many ethernet links
* Aggregating throughput of high bandwidth ethernet links (Gbit/s)
* Super-aggregating Wi-Fi and LTE and Wi-Fi and Ethernet
* Quick failover on link errors without connection loss
* Multipath HD video streaming
* Vertical handover between Wi-Fi and cellular networks
* Android implementation and QoE triggered application agnostic multipath 
* and few more approach...

MPT papers: [http://irh.inf.unideb.hu/user/szilagyi/mpt_papers](http://irh.inf.unideb.hu/user/szilagyi/mpt_papers)

