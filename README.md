# MPT - Multi-Path Tunnel

##What is this?
This is a small tool for network layer multipath. Works like a multpath VPN application: create a logical tunnel interface over the native phisical interfaces and share the tunnel's traffic between them.

The datapath encapsulation of this application is completely standard! See [RFC 8086](https://tools.ietf.org/html/rfc8086) for the details of the GRE-in-UDP architecture. We have an additonal multipath signalig, which is not standard, but we have some initial effort on that (see [this internet draft](https://www.ietf.org/internet-drafts/draft-lencse-tsvwg-mpt-00.txt)). So this is a GRE-in-UDP VPN application with multipath extension.

##Compilation
1. Install the dependencies: 
```
$ sudo apt install gcc-multilib libssl-dev net-tools iproute make libev-dev
```
2. Compile
```
$ cd mpt
$ make
```

##Configuration
Here is a guide for configure this application. There are some steps which makes this readme too length so please visit the link below.
More information: [http://irh.inf.unideb.hu/user/szilagyi/sites/default/files/mpt/usermanual.pdf](http://irh.inf.unideb.hu/user/szilagyi/sites/default/files/mpt/usermanual.pdf)

##Running the software
Start the `mptsrv` binary in root mode. Double check if the configuration is correct and match with the real world test setup.

##Resources
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

