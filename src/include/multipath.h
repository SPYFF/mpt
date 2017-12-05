/**
 * @file
 * @brief       MPT Data structures
 * @author      Almasi, Bela; Debrecen, Hungary
 * @copyright   Project maintained by Almasi, Bela; Debrecen, Hungary
 * @date        2012.08.15.
 */

#ifndef MULTIPATH_H
#define MULTIPATH_H

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include <sys/types.h>
#include <netdb.h>

#define bit_64 uint64_t
#define bit_32 uint32_t
#define bit_16 uint16_t
#define bit_8  uint8_t

/// MAX_PATH - the maximum number of paths per connection
#define MAX_PATH    20
/// MAX_INTNUM - the maximum number of network interfaces
#define MAX_INTNUM  20
/// MAX_NETWORKS - the maximum number of networks per connection_struct
#define MAX_NETWORKS 20
/// REORDER_SIZE - the size of the path's reorder buffer
#define REORDER_SIZE 10000 +1
/// CIRCLE_SIZE - the size of the circulatebuffer
#define CIRCLE_SIZE  2*REORDER_SIZE+2
/// PATH_SELECTION_LENGTH - the maximum length of the pathselectionindex vector
#define PATH_SELECTION_LENGTH 10100

// *************************************************************************************
/// Data structures for circulated buffer

typedef struct grebuff_struct
{
  	char buffer[1600];  ///< The buffer to store the GRE in UDP packet, coming on the physical interface
	struct timeval timestamp;  /// The timestamp of packet arrival
	bit_32	seq;  	  // The GRE sequence number of the buffered packet
	bit_16 grebufflen;   ///< The packet length (including the GRE header too). Zero means no packet in the buffer
	bit_16 grelen;  ///< The length of the GRE header in the packet
} grebuff_type;




/// Data structure of a single path in a connection
typedef struct path_struct
{
    bit_32 ip_private[4]; ///< The local private IP of the interface
    bit_32 ip_public[4]; ///< The local public IP address of the path, can be v4 or v6
    bit_32 ip_remote[4];///< The remote IP address of the path, can be v4 or v6
    bit_32 ip_gw[4];    ///< The IP address of the outgoing gateway, can be v4 or v6
//    bit_32 mac_local[2];///< The MAC address of the local interface ( for EUI-64 too)
//    bit_32 mac_gw[2];   ///< The MAC address of the outgoing gateway (for EUI-64)
    bit_32 packet_max;  ///< The maximum number of packets to send continously on the path
    bit_32 header[32];  ///< buffer to hold the header (Eth, IP, UDP) data for raw socket
    bit_16 weight_in;   ///< The weight value of the incoming traffic
    bit_16 weight_out;  ///< The weight value of the outgoing traffic
    bit_32 selection_increment; ///< The increment value for pathselection vector calculation
    bit_8 status;       ///< The status of the path
    bit_8 cmd_default;  ///< Value 1 means, that this path is the default for cmd communication
    bit_8 ip_version;   ///< The IP version of the path
    bit_8  keepalive;   ///< Keepalive message interval in secs
    bit_16 deadtimer;   ///< Keepalive timeout value in secs
    time_t last_keepalive; ///< Time of last keepalive received
    int    socket;      ///< The local UDP socket ID of the conn. (IPv6 and IPv4 too)
    struct sockaddr_in6 peer; ///< The sockaddr data for the peer
    struct sockaddr_in6 peer_cmd; ///< The sockaddr data for the peer
    char interface[128];///< The name of the local physical interface
    grebuff_type  *grebuffarray; ///< The buffers for GRE in UDP packets coming on the physical interface
    int grebuffindex;   ///< The index of the next packet to store
    void *connection;    ///< The connection, to which the path is connected
    pthread_t  socket_read;     ///< The thread id of the connection socket reader thread
    pthread_t  keepalive_send; ///< The thread id of the keepalive send thread
} path_type;

/// Data struct for a network connection
typedef struct network_struct
{
  int version;
  bit_32 source[4];
  bit_32 source_prefix_length;
  bit_32 destination[4];
  bit_32 destination_prefix_length;
} network_type;

/// Type of masks for ipv4
typedef bit_32 ipv4_masks_t[33];
/// Type of ipv6 mask
typedef bit_32 ipv6_mask_t[4];
/// Type of masks for ipv6
typedef ipv6_mask_t ipv6_masks_t[129];

/// Data structure of a connection
typedef struct connection_struct
{
    char   name[128];   ///< The name of the connection
    char   filename[128];    ///< The configuraton file of the connection
    bit_32 ip_local[4]; ///< The local  IP address of the tunnel can be v4 or v6
    bit_32 ip_remote[4];///< The remote IP address of the tunnel can be v4 or v6
    bit_16 port_local;  ///< The UDP local endpoint id (port number)
    bit_16 port_remote; ///< The UDP remote endpoint id (port number)
    bit_16 cmd_port_local;  ///< The UDP port for receiving commands (cmd port number)
    bit_16 cmd_port_remote; ///< The cmd remote UDP port number
//    bit_32 *cmd_ip_remote;  ///< The IP adddress of the remote peer to access with commands
    bit_8  gre_header[16]; ///< The GRE header for GRE in UDP encapsulation
    bit_8  gre_length;  ///< The length of the GRE header (in octets) in the GRE in UDP encapsulation
//    int    socket_raw;  ///< The local raw socket ID (if raw socket can be used)
    bit_64 conn_packet; ///< The number of packets sent on the connection
    bit_16 path_count;  ///< The number of the existing paths
    bit_16 network_count; ///< The number of defined networks
    bit_8  status;      ///< The status of the connection
    bit_8  permission;  ///< Allow connection updates (1: send bit, 2: receive bit)
    bit_8  ip_version;  ///< The IP version of the connection (4 or 6)
    bit_32 waitrand;    ///< Security token used at communication
    bit_8  waitround;   ///< Client phase in communication
    char   waithash[32]; ///< Expected checksum for receiving data (SHA-256 is 32 byte long)
    bit_32 reorder_window;  ///< 0=no packet reordering, otherwise the GRE sequence number is used to reorder the packets in the window, the window size is given here
    bit_32 circle_window; ///< The maximum size of the circle buffer. Usually circle_window = 2* reorder_window
    bit_8  auth_type;   ///< The authentication code for the connection
    char  auth_key[128];///< The key value of the authentication
    path_type mpath[MAX_PATH]; ///< The paths asociated to the connection
    path_type *default_cmd_path; ///< The default path for cmd communication
    network_type networks[MAX_NETWORKS]; ///< The networks connected by the connection
    grebuff_type *circlebuff[CIRCLE_SIZE];  ///< The circulate buffer for the connection
    path_type  *pathselectionlist[PATH_SELECTION_LENGTH]; ///< This vector contains the sequence of indices of paths, according to their weights out
    path_type  *tcp_src_lut[65536];
    path_type  *tcp_dst_lut[65536];
    path_type  *udp_src_lut[65536];
    path_type  *udp_dst_lut[65536];
    bit_16  pathselectionlength;  ///< The length of the pathselectionindex vector
    bit_16 path_index;  ///< The index of the actual path in the pathselectionlist, shows the path to send the next packet
    bit_32  seq_start;  ///< The GRE sequence number of the packet in the circlestart position, we are waiting for this seq
    bit_32  circlepackets;  ///< The number of packets in the circlebuff, circlepackets = 0 means empty buffer
    bit_16  circlestart;   ///< The index of the circlebuff, where the buffering was started (circle buffer start index)
    struct timeval max_buffdelay;   ///< The maximum time, for which the packet will be hels in the circlebuff
    struct timeval last_receive;   ///< The time of last data receive on the connection
    //bit_16  circleend;    ///< The index of the circlebuff, where the next packet will be placed. No need as GRE seq will determine the position
    struct connection_struct *next; ///< Pointer to the next connection element
    struct connection_struct *prev; ///< Pointer to the previous connection element
    pthread_t  circlebuffer_handler;  ///< The thread to read (packet data) from the circlebuffer and send it to the tunnel
} connection_type;

/// Data structure of tunnel interface
typedef struct tunnel_struct
{
    char    interface[128]; ///< The name of the tunnel interface e.g. tun1
    char    device[128];    ///< The name of the tunnel device
    int     fd;             ///< The file descriptor of the opened tunnel interface
    bit_32  mtu;            ///< The MTU value of the tunnel interface
    bit_32  ip4;            ///< The IPv4 address of the tunnel
    bit_16  ip4len;         ///< The prefix length of the IPv4 address
    bit_32  ip6[4];         ///< The IPv6 address of the tunnel
    bit_16  ip6len;         ///< The pref.length of the IPv6 address
    bit_16  cmd_port_rcv;   ///< The port number for multipath commands to accept
    int     cmd_socket_rcv; ///< The socket id of the cmd communication to accept
//    bit_16  cmd_port_snd;   ///< Port number to send multipath commands
//    int     cmd_socket_snd; ///< Socket id of the cmd communication to start
    pthread_t  tunnel_read; ///< The thread id of the tunnel reader thread
    pthread_t  cmd_read;    ///< The thread id of the cmd read thread
} tunnel_type;




// *********************************** Function headers *****************

/// Load the interface data (addresses) from file.
/// The first interface in the file must be the tunnel interface - other interfaces are not used any more

#endif // MULTIPATH_H
