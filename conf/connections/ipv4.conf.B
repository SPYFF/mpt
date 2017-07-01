;###################### Multipath Connection Information: ###############
[connection]
name               = MPT_connection_10.0.0.3
permission         = 3
ip_ver             = 4
ip_local           = 10.0.0.3
local_port         = 23456
ip_remote          = 10.0.0.2
remote_port        = 23456
remote_cmd_port    = 60456
path_count         = 2
network_count      = 2
status             = 0
reorder_window     = 900
max_buffdelay_msec = 300
auth_type          = 0
;auth_key           = 0

;########################## PATHS ############################

[path_0]
interface_name   = eth1
ip_ver           = 4
public_ipaddr    = 10.1.1.3
gw_ipaddr        = 10.1.1.3
remote_ipaddr    = 10.1.1.2
keepalive_time   = 5
dead_time        = 11
weight_out       = 10
cmd_default      = 1
status           = 0

[path_1]
interface_name   = eth2
ip_ver           = 4
public_ipaddr    = 10.2.2.3
gw_ipaddr        = 10.2.2.3
remote_ipaddr    = 10.2.2.2
keepalive_time   = 5
dead_time        = 11
weight_out       = 10
status           = 0

;####################### NETWORKS #####################################
[net_0]
ip_ver     = 4
src_addr   = 21.1.1.0/24
dst_addr   = 11.1.1.0/24

[net_1]
ip_ver     = 4
src_addr   = 22.1.1.0/24
dst_addr   = 12.1.1.0/24
