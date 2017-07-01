####################### Interface Information: ###############################
# Here you can configure the tunnel interface, which is going to be handled  #
# by the MPT server. The general section contains the information            #
# necessary for the further interpretation. These are the following:         #
#                                                                            #
# accept_remote = [VALUE], where the VALUE determines the acceptance         #
# of a remote new connection request (0=don't accept; 1=accept)              #
#                                                                            #
# cmdport_local = [VALUE], where the VALUE determines the portnum            #
# we use to receive commands from a remote mptsrv or mpt client              #
#                                                                            #
# cmd_timeout = [VALUE], where the VALUE determines the timeout (seconds)    #
# for waiting the answer of the server when issuing an mpt command.          #
#                                                                            #
############################################################################## 

[general]
tunnel_num    = 1
; Accept remote new connection request
accept_remote = 1
cmdport_local = 60456
cmd_timeout = 25

[tun_0]
name      = tun0
; tunnel mtu: set to 1464 if all the paths use IPv4, otherwise set to 1440
mtu       = 1440
ipv4_addr = 10.0.0.2/24
ipv6_addr = fec0::2/64

