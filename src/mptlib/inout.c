/**
 * @file
 * @brief Screen and file input/output routines
 * @copyright Project maintained by Almasi, Bela; Debrecen, Hungary
 * @author Almasi, Bela; Debrecen, Hungary
 * @date 2012.08.15.
 */

//#define MPT_DEBUG

#include <dirent.h>
#include <inttypes.h>
#include <sys/types.h>
#include <string.h>
#include "multipath.h"
#include "mp_local.h"
#include "auth.h"
#include "iniparser.h"



static void _setup_con(dictionary * conf, connection_type * conn);
static void _setup_pth(dictionary * conf, char *section, path_type * path);
static void _setup_net(dictionary * conf, char *section,
		       network_type * net);


// initialize netmask values (arrays)
void initialize()
{
    int i;
    int maskv4 = 0xFFFFFFFF;
    ipv6_mask_t maskv6;

    mp_global_conn = NULL;
    DEBUG("Initialize IPv4 masks...");
    for (i = 32; i >= 0; i--) {
	ipv4_masks[i] = ntohl(maskv4);
	maskv4 <<= 1;
//	printf("i=%2d: %08x\n", i, ipv4_masks[i]);
    }
    DEBUG("IPv4 masks initialization is done.");

    DEBUG("Initialize IPv6 masks...");
    maskv6[0] = maskv6[1] = maskv6[2] = maskv6[3] = 0xFFFFFFFF;
    i = 128;
    for (;;) {
	ipv6_masks[i][0] = ntohl(maskv6[0]);
	ipv6_masks[i][1] = ntohl(maskv6[1]);
	ipv6_masks[i][2] = ntohl(maskv6[2]);
	ipv6_masks[i][3] = ntohl(maskv6[3]);
//DEBUG("i=%3d: %08x %08x %08x %08x\n", i, ipv6_masks[i][0],
//	       ipv6_masks[i][1], ipv6_masks[i][2], ipv6_masks[i][3]);
	--i;
	if (i < 0) {
	    break;
	}
	maskv6[i / 32] <<= 1;
    }
    DEBUG("IPv6 masks initialization is done.");

}

// function get_privateip4
// Determines the private (locally assigned) ip address of an interface
// Params:
//      privateip: The output string, containing the local IP address
//      intname: The name of the interface, for which the global IP address must be determined (can be NULL)
// Returns: 1 on sucess, 0 otherwise
int get_privateip4(char *result, const char *intname)
{
    int sock;
    struct sockaddr_in *p;
    struct ifreq req;

    if (!intname) return 0;

    memset(&req, 0, sizeof(struct ifreq));
    strcpy(req.ifr_name, intname);

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        fprintf(stderr, "Socket creation error. Private IP address can not be determined (int: %s) \n", intname);
	    return 0;
    }

    if (ioctl(sock, SIOCGIFADDR, &req) == -1) {
        fprintf(stderr, "Socket IOCTL error. Private IP address can not be determined (int: %s) \n", intname);
	    return 0;
    }
    p = ( (struct sockaddr_in *)&req.ifr_addr );
    inet_ntop(AF_INET, &p->sin_addr, result, 64);
//    set_ipstr(result, (struct sockaddr_in *)(&(req.ifr_addr)))->sin_addr, 4)
 DEBUG("Private IP address of interface %s : #%s# \n", intname, result);
    return 1;
}


// function wgetgetglobalip4
// Determines the global ip address of an interface (maybe behind a NAT-Box) using web-site download
// Params:
//      globalip: The output string, containing the global IP address
//      intname: The name of the interface, for which the global IP address must be determined (can be NULL)
//      gateway: String containing the gateway to use (can be NULL)
// Returns: 1 on sucess, 0 otherwise
int wgetglobalip4(char *globalip, char *intname, char *gateway) { //http get

    void removenoipchar(char *str) {
        char *pr = str, *pw = str;
        while (*pr) {
            while ( (*pr) && ( (*pr != '.') && ((*pr > '9') || (*pr < '0')) ) ) pr++;
            *pw = *pr;
            pw++;
            pr++;
        }
        *pw = '\0';
    }

    struct sockaddr_in6 peer;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s, glen, ret;
    char buf[5000], cmd[200];
    char *server = "checkip.dyndns.org";
    char serverip[128];

#define GETSTR "GET / HTTP/1.1\nUser-Agent: Wget/1.15 (linux-gnu)\nAccept: */*\nHost: checkip.dyndns.org\nConnection: Keep-Alive\n\n\n"

    ret = 0;
DEBUG("Starting globl IP address reading from %s (interface: %s )\n", server, intname);
   /* Obtain address(es) matching host/port */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;    /* Using AF_UNSPEC will allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */

    s = getaddrinfo(server, "80", &hints, &result);
    if (s != 0) {
        printf("Can not obtain address for server %s  Global address can not be determined!\n", server);
        return 0;
    }

   /* getaddrinfo() returns a list of address structures.
       Try each address until we successfully connect(2).
       If socket(2) (or connect(2)) fails, we (close the socket
       and) try the next address. */

   for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1)
            continue;
//DEBUG("Before setsockopt\n");
       if (intname) setsockopt(sfd, SOL_SOCKET, SO_BINDTODEVICE, intname, strlen(intname) );
//DEBUG("Before connect\n");
        //set the socket in non-blocking
        fcntl(sfd, F_SETFL, O_NONBLOCK);
        inet_ntop(AF_INET, rp->ai_addr, serverip, 128);
    DEBUG("Serverip: %s\n", serverip);
        if (gateway) { sprintf(cmd, "route add -host %s gw %s dev %s ", serverip, gateway, intname); system(cmd);}
        connect(sfd, rp->ai_addr, rp->ai_addrlen);
        usleep(5000000);
        fcntl(sfd, F_SETFL, 0);
        if (!getpeername(sfd, (struct sockaddr *)&peer, (socklen_t *)&glen))
            break;                  /* Success */

        if (gateway) { sprintf(cmd, "route del -host %s gw %s ", serverip, gateway); system(cmd);}
//DEBUG("After connect\n");
       close(sfd);
    }

   if (rp == NULL) {               /* No address succeeded */
        printf("Could not connect to %s (interface: %s)\n", server, intname);
        ret = 0;
        return(ret) ;
    }

   freeaddrinfo(result);           /* No longer needed */
   strcpy(buf, GETSTR);
//DEBUG("Before send\n");
   send(sfd, buf, strlen(buf), 0);

   struct timeval tv;
   memset(&tv, 0, sizeof(tv));     // set socket timeout
   tv.tv_sec = 15; // 15 sec
   setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));
   memset(buf, 0, sizeof(buf));
//DEBUG("Before recv\n");
   glen = recv(sfd, buf, 5000, 0);
   close(sfd);

   if (glen > 0) {
         char *ips, *ipe;
         ips = NULL; ipe = NULL;
         ips = strstr(buf, "body");
         if (ips) ipe = strstr(ips+4, "body");
         if (ipe) {
            ipe = '\0';
            removenoipchar(ips);
        DEBUG("Public IP address of interface %s: #%s#\n", intname, ips);
            strcpy(globalip, ips);
            ret = 1;
            goto ROUTE_DEL ;
         }
         else {
            printf("No body part in the HTTP answer\n");
         }
   }  // if glen > 0

   printf("No IP address got from %s (interface: %s)\n", server, intname);
   DEBUG("\n%s\n", buf);
ROUTE_DEL:
   if (gateway) { sprintf(cmd, "route del -host %s gw %s ", serverip, gateway); system(cmd);}
   return ret;

}



// function get_globalip4
// Determines the global ip address of an interface (maybe behind a NAT-Box)
// Params:
//      globalip: The output string, containing the global IP address
//      intname: The name of the interface, for which the global IP address must be determined (can be NULL)
//      gateway: String containing the gateway to use (can be NULL)
// Returns: 1 on sucess, 0 otherwise
int get_globalip4(char *globalip, char *intname, char *gateway) { //get from opendns resolver1.opendns.com
    char hextodec(char c) {
       char b = tolower(c);
       if ( ( b >= '0') && (b<= '9') ) return (b - '0');
       return (b-'a'+10);
    }

  //  char  dnstxt[] = "03c601200001000000000001046d796970076f70656e646e7303636f6d00000100010000291000000000000000";
    char  dnstxt[]="dc8601000001000000000000046d796970076f70656e646e7303636f6d0000010001";
    char  myname[] = "\4myip\7opendns\3com";

    struct sockaddr_in myip_resolver;
 //   struct sockaddr_in sender;
    // struct sockaddr_in peer;
    char  serveriptxt[] = "208.67.222.222";
    int sfd, i, s, blen, f; // ssize;
    char sbuf[2000], rbuf[2000], cmd[200];
    struct timeval tv;
    memset(&tv, 0, sizeof(tv));     // set socket timeout
    tv.tv_sec = 5; // max 5 sec waiting foe answer
    tv.tv_usec = 10;

   memset(&myip_resolver, 0, sizeof(myip_resolver));
   inet_pton(AF_INET, serveriptxt, &myip_resolver.sin_addr );
   myip_resolver.sin_port = htons(53);

   sfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
   if (sfd < 0) {
        printf("Socket open error. Global IP address can not be determined!\n");
        return 0;
   }
//   if (intname) setsockopt(sfd, SOL_SOCKET, SO_BINDTODEVICE, intname, strlen(intname) ); // bind to the interface
//DEBUG("get_globalip: socket binded\n");
   if ( (gateway) && (intname)) { sprintf(cmd, "route add -host %s gw %s dev %s ", serveriptxt, gateway, intname); system(cmd);}
   blen = connect(sfd, (struct sockaddr *) &myip_resolver, sizeof(myip_resolver));
   if (blen < 0)
   {
        printf("Cannot connect to DNS server %s:%d. Global IP address can not be determined! (errno:%d) \n", serveriptxt, 53, errno);
        goto DELGW ;
   }

   s=0;
   memset(sbuf, 0, sizeof(sbuf));
   for (i=0; i<strlen(dnstxt); i++) { // the dnstxt contains hex values to send, convert it to byte values
        if (i%2) {
             sbuf[s] += (unsigned)hextodec(dnstxt[i]); //printf(" %2x", sbuf[s]);
             s++;
        }
        else { sbuf[s] = (unsigned)16*hextodec(dnstxt[i]); }
   }

DEBUG("Starting globl IP address reading from resolver1.opendns.com (interface: %s )\n", intname);

//   if ( (gateway) && (intname)) { sprintf(cmd, "route add -host %s gw %s dev %s ", serveriptxt, gateway, intname); system(cmd);}
   for (i=0; i<3; i++) { // try to get the global IP maximum 5 times
        // blen = send(sfd, sbuf, s, 0); //
        blen = sendto( sfd, sbuf, s, 0, (struct sockaddr *)&myip_resolver, sizeof(myip_resolver));
   // if (blen<0) {printf("blen:%d, errn:%d", blen, errno);}
        setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));
        memset(rbuf, 0, sizeof(rbuf));
        blen = 0;
        blen = recv(sfd, rbuf, sizeof(rbuf), 0); //
        // blen = recvfrom(sfd, rbuf, (size_t) sizeof(rbuf), 0, (struct sockaddr *) &peer, (socklen_t *)&psize);
        if (blen>0) break; // got the answer
        printf(" get_globalip: trying to read global ip address again (Int: %s)... \n", intname);
        usleep(10000);
    }
//   if (gateway) { sprintf(cmd, "route del -host %s gw %s ", serveriptxt, gateway); system(cmd);}
DELGW:
   if ( (gateway) && (intname)) { sprintf(cmd, "route del -host %s gw %s dev %s ", serveriptxt, gateway, intname); system(cmd);}

   if (blen<=0){
        printf("No answer from DNS server. Global IP address can not be determined! (Int: %s) \n", intname);
        printf("Trying tp get with HTTP from checkip.dyndns.org.\n");
        return (wgetglobalip4(globalip, intname, gateway));
   }

   i=0;
   for (i=0; i<blen; i++) {
       if (!strncmp(&rbuf[i], myname, sizeof(myname))) break;
// DEBUG("i:%d %s\n", i, &rbuf[i]);
   }

   if (i >= blen){
        printf("No myip.opendns.com in the DNS answer. \nGlobal IP address can not be determined! (Int: %s) \n", intname);
        return 0;
   }

   for ( f=0; (i<blen) && (f<3); i++)  { // we have to find the second 00 01 00 01 byte sequence
        if ( (f<2) && (!memcmp(&rbuf[i], "\0\1\0\1", 4)) ) f++;
        if ( (f==2) && (!memcmp(&rbuf[i], "\0\4", 2)) ) { f++; break; }
   }

   if (f < 3){
        printf("IPv4 address not fount in the DNS answer. Global IP address can not be determined! (Int: %s) \n", intname);
        return 0;
   }
   i += 2;
   memset(sbuf, 0, sizeof(sbuf));
   inet_ntop(AF_INET, &rbuf[i], sbuf, sizeof(sbuf));
DEBUG("Public IP address of interface %s: #%s#\n", intname, sbuf);
   if (globalip) strcpy(globalip, sbuf);
   return 1;
}




/**
 * @file
 * Function calls to manage connections:
 * @dot
 * digraph connections {
 *      size = "6,6";
 *      CMD_REPLICATE2 [label="CMD_REPLICATE"];
 *      conn_load_dir -> conn_load -> {conn_new, conn_parser};
 *      conn_reload_dir -> conn_reload -> {conn_parser, conn_activate};
 *      conn_activate -> {conn_diff, conn_replicate, conn_start};
 *      conn_replicate -> handshake -> CMD_REPLICATE;
 *      CMD_REPLICATE2 -> cmd_read_thread -> do_command;
 *      do_command -> {conn_mirror, conn_activate};
 * }
 * @enddot
 */

/**
 * Print out debug informations about the tunnel information
 * @param tun The tunnel interface
 */
void tunnel_print(FILE *stream)
{
    tunnel_type * tunp;
    char ipstr[256];
    if (stream == NULL) stream = stdout;
    tunp = &tun;
    fprintf(stream, "\n");
    fprintf(stream, "Tunnel information:\n");
    fprintf(stream, "    Tunnel interface : %s\n", tunp->interface);
    fprintf(stream, "    Tunnel device    : %s\n", tunp->device);
    fprintf(stream, "    Tunnel file desc.: %d\n", tunp->fd);
    fprintf(stream, "    Tunnel int. MTU  : %d\n", tunp->mtu);
    inet_ntop(AF_INET, &tunp->ip4, ipstr, 255);
    if (tunp->ip4len)
	fprintf(stream, "    Tunnel ipv4 addr.: %s/%d\n", ipstr, tunp->ip4len);
    inet_ntop(AF_INET6, &tunp->ip6, ipstr, 255);
    if (tunp->ip6len)
	fprintf(stream, "    Tunnel ipv6 addr.: %s/%d\n", ipstr, tunp->ip6len);
    fprintf(stream, "    CMD server port number  : %d\n", tunp->cmd_port_rcv);
//    printf("    CMD client port number  : %d\n", tun->cmd_port_snd );
//    printf("    CMD client socket id    : %d\n", tun->cmd_socket_snd );
    fprintf(stream, "    Global server    : %d\n", mp_global_server);
    fprintf(stream, "\n");

}



/**
 * Print out debug information about a connection
 * @param conn The connection
 */
void conn_print_item(FILE * stream, connection_type * conn)
{
    char pipstr[128], lipstr[128], ripstr[128], gwstr[128];
//    unsigned char *mac;
    int i, af, start;
    path_type *p;
    if (!conn){
        printf("conn_print_item called with NULL argument - returning\n");
        return;
    }

    strcpy(lipstr, "");
    strcpy(ripstr, "");
    set_ipstr(lipstr, conn->ip_local, conn->ip_version);
    set_ipstr(ripstr, conn->ip_remote, conn->ip_version);
    fprintf(stream, "\n");
    fprintf(stream, "Multipath Connection Information:\n");
    fprintf(stream, "Connection name  : %s \n", conn->name);
    fprintf(stream, "    Config file           : %s\n", conn->filename);
    fprintf(stream, "    IP version            : %d\n", conn->ip_version);
    fprintf(stream, "    Local  Tunnel address : %s\n", lipstr);
    fprintf(stream, "    Local  UDP Port       : %d\n", conn->port_local);
    fprintf(stream, "    Local  CMD UDP Port   : %d\n", conn->cmd_port_local);
    fprintf(stream, "    Remote Tunnel address : %s\n", ripstr);
    fprintf(stream, "    Remote UDP Port       : %d\n",
	    conn->port_remote);
//    fprintf(stream, "    Socket ID             : %d\n", conn->socket);
    fprintf(stream, "    Remote CMD UDP Port   : %d\n",
	    conn->cmd_port_remote);
    fprintf(stream, "    Number of Paths       : %d\n", conn->path_count);
    fprintf(stream, "    Number of networks    : %d\n",
	    conn->network_count);
    fprintf(stream, "    Packets sent out      : %" PRIu64 "\n",
	    conn->conn_packet);
    fprintf(stream, "    Connection update     : %d\n", conn->permission);
    fprintf(stream, "    Authentication type   : %d\n", conn->auth_type);
    //fprintf(stream, "    Authentication code   : "); printHash(conn->auth_key, keySize(conn->auth_type));
    fprintf(stream, "    Connection reorder win: %d\n", conn->reorder_window);
    fprintf(stream, "    Max. delay in rcv.buff: %d(sec) %d(msec)\n", (int)conn->max_buffdelay.tv_sec, (int)conn->max_buffdelay.tv_usec/1000);
    fprintf(stream, "    Connection status     : %d\n", conn->status);
    if (conn->path_count <= 0)
	return;

    p = conn->mpath;
    for (i = 0; i < conn->path_count; i++) {
	strcpy(lipstr, "");
	strcpy(ripstr, "");
	strcpy(pipstr, "");
	strcpy(gwstr, "");

	if (p->ip_version == 6) {
	    af = AF_INET6;
	    start = 0;
	} else {
	    af = AF_INET;
	    start = 3;
	}
	inet_ntop(af, &p->ip_public[start], lipstr, 128);
	inet_ntop(af, &p->ip_private[start], pipstr, 128);
	inet_ntop(af, &p->ip_remote[start], ripstr, 128);
	inet_ntop(af, &p->ip_gw[start], gwstr, 128);

	fprintf(stream, "  Path %d information:\n", i);
	fprintf(stream, "     Interface name:    : %s\n", p->interface);
//        fprintf(stream, "     IP version         : %d \n", p->ip_version);
//	mac = (unsigned char *) p->mac_local;
//	fprintf(stream,	"     Local  MAC address : %02X:%02X:%02X:%02X:%02X:%02X\n",
//		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	fprintf(stream, "     Public IP address  : %s\n", lipstr);
	fprintf(stream, "     Private IP address : %s\n", pipstr);
//	mac = (unsigned char *) p->mac_gw;
//	fprintf(stream,	"     Gateway MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
//		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	fprintf(stream, "     Gateway IP address : %s\n", gwstr);
	fprintf(stream, "     Remote IP address  : %s\n", ripstr);
	fprintf(stream, "     Incoming weight    : %d\n", p->weight_in);
	fprintf(stream, "     Outgoing weight    : %d\n", p->weight_out);
//	fprintf(stream, "     Path window size   : %d\n", p->packet_max);
    fprintf(stream, "     Keepalive time     : %d\n", p->keepalive);
    fprintf(stream, "     Dead timer         : %d\n", p->deadtimer);
	fprintf(stream, "     Path Status        : %d\n", p->status);

	p++;
    }
    fprintf(stream, "\n");


    {				//anonym block begin
	if (conn->network_count) fprintf(stream, "Networks:\n");
	for (i = 0; i < conn->network_count; i++) {
	    if (p->ip_version == 6) {
		af = AF_INET6;
		start = 0;
	    } else {
		af = AF_INET;
		start = 3;
	    }
	    inet_ntop(af, &conn->networks[i].source[start], lipstr, 128);
	    inet_ntop(af, &conn->networks[i].destination[start], ripstr,
		      128);
	    fprintf(stream, "-------- Network %d ----------\n", i);
	    fprintf(stream, "    IP version       : %d\n",  conn->networks[i].version);
	    fprintf(stream, "    Source           : %s/%d\n", lipstr, conn->networks[i].source_prefix_length);
	    fprintf(stream, "    Destination      : %s/%d\n", ripstr, conn->networks[i].destination_prefix_length);
	}
    }				//anonym block end

}

/**
 * Print out all connection's information
 * @param stream the output stream, will be stdout if NULL
 */
void connection_print(FILE *stream)
{
    int i;
    connection_type * conn, *cp;
    conn = mp_global_conn;
    if (stream == NULL) stream = stdout;
    i = 0;
    cp = conn;
    while (cp) {
        i++;
        cp = cp->next;
    }
    fprintf(stream, "Number of connections: %d \n", i);
    while (conn) {
        conn_print_item(stream, conn);
        conn = conn->next;
    }
}


// connection_save - saves the conn connection to the file descriptor fd
void connection_save(FILE *fd, connection_type *conn)
{
    path_type *p;
    network_type *n;
    char conf_value[255];
    int path_count, network_count, index;

    fprintf(fd,
	    ";;;;;;;#################### Multipath Connection Information: ###############\n");
    fprintf(fd, "\n[connection]\n");

    fprintf(fd, "%-16s = %s\n", "name", conn->name);
    fprintf(fd, "%-16s = %d\n", "permission", conn->permission);
    fprintf(fd, "%-16s = %d\n", "ip_ver", conn->ip_version);

    set_ipstr(conf_value, conn->ip_local, conn->ip_version);
    fprintf(fd, "%-16s = %s\n", "ip_local", conf_value);
    fprintf(fd, "%-16s = %d\n", "local_port", conn->port_local);
    fprintf(fd, "%-16s = %d\n", "local_cmd_port", conn->cmd_port_local);

    set_ipstr(conf_value, conn->ip_remote, conn->ip_version);
    fprintf(fd, "%-16s = %s\n", "ip_remote", conf_value);
    fprintf(fd, "%-16s = %d\n", "remote_port", conn->port_remote);
    fprintf(fd, "%-16s = %d\n", "remote_cmd_port", conn->cmd_port_remote);

    path_count = conn->path_count;
    fprintf(fd, "%-16s = %d\n", "path_count", conn->path_count);

    network_count = conn->network_count;
    fprintf(fd, "%-16s = %d\n", "network_count", conn->network_count);
    fprintf(fd, "%-16s = %d\n", "status", conn->status);
    fprintf(fd, "%-16s = %d\n", "reorder_window", conn->reorder_window);
    fprintf(fd, "%-16s = %d\n", "max_buffdelay_msec", (int)conn->max_buffdelay.tv_sec*1000 + (int)conn->max_buffdelay.tv_usec/1000 );
    fprintf(fd, "%-16s = %d\n", "auth_type", conn->auth_type);
 //   fprintf(fd, "%-16s = %s\n", "auth_key", conn->auth_key);


    p = conn->mpath;
    fprintf(fd,
	    "\n########################## PATHS ############################\n");
    for (index = 0; index < path_count; ++index, ++p) {
	fprintf(fd, "\n[path_%d]\n", index);
	fprintf(fd, "%-16s = %s\n", "interface_name", p->interface);
	fprintf(fd, "%-16s = %d\n", "ip_ver", p->ip_version);
//	mac_ntop((char *) p->mac_gw, conf_value);
//	fprintf(fd, "%-16s = %s\n", "local_macaddr", conf_value);
	set_ipstr(conf_value, p->ip_private, p->ip_version);
	fprintf(fd, "%-16s = %s\n", "private_ipaddr", conf_value);
	set_ipstr(conf_value, p->ip_public, p->ip_version);
	fprintf(fd, "%-16s = %s\n", "public_ipaddr", conf_value);
//	mac_ntop((char *) p->mac_gw, conf_value);
//	fprintf(fd, "%-16s = %s\n", "gw_macaddr", conf_value);
	set_ipstr(conf_value, p->ip_gw, p->ip_version);
	fprintf(fd, "%-16s = %s\n", "gw_ipaddr", conf_value);
	set_ipstr(conf_value, p->ip_remote, p->ip_version);
	fprintf(fd, "%-16s = %s\n", "remote_ipaddr", conf_value);
    fprintf(fd, "%-16s = %d\n", "keepalive_time", p->keepalive);
    fprintf(fd, "%-16s = %d\n", "dead_time", p->deadtimer);
	fprintf(fd, "%-16s = %d\n", "weight_in", p->weight_in);
	fprintf(fd, "%-16s = %d\n", "weight_out", p->weight_out);
//	fprintf(fd, "%-16s = %d\n", "window_size", p->packet_max);
	fprintf(fd, "%-16s = %d\n", "status", p->status);

    }

    n = conn->networks;
    fprintf(fd,
	    "\n####################### NETWORKS #####################################\n");
    for (index = 0; index < network_count; ++index, ++n) {
	fprintf(fd, "\n[net_%d]\n", index);

	fprintf(fd, "%-16s = %d\n", "ip_ver", n->version);
	set_ipstr(conf_value, n->source, n->version);
	fprintf(fd, "%-16s = %s/%d\n", "src_addr", conf_value, n->source_prefix_length);
	set_ipstr(conf_value, n->destination, n->version);
	fprintf(fd, "%-16s = %s/%d\n", "dst_addr", conf_value, n->destination_prefix_length);
    }
}

/**
 * Write out the changes of all connection in the specified file
 *
 * @param filename      The file contains the connections
 */
void conn_save_filename(char *filename)
{
    FILE *fd;
  //  char lipstr[128], ripstr[128], gwstr[128];
  //  unsigned char *mac;
  //  int  num = 0;
    connection_type *conn;
   // dictionary *conf;

    for (conn = mp_global_conn; conn != NULL ; conn = conn->next) {
	      if (filename) {
                if (strcmp(conn->filename, filename) == 0) {
                    chdir("conf/connections");
                    fd = fopen(filename, "w");
                    chdir("../..");
                    if (fd == NULL) {
                        fprintf(stderr, "\nFile write open error: %s\n", filename);
                        return;
                    }
                    connection_save(fd, conn);
                    fclose(fd);
                    break;
                }
	      }
	      else {
                    chdir("conf/connections");
                    fd = fopen(conn->filename, "w");
                    chdir("../..");
                    if (fd == NULL) {
                        fprintf(stderr, "\nFile write open error: %s\n", filename);
                        return;
                    }
                    connection_save(fd, conn);
                    fclose(fd);
          }
    }
    return;
}

/**
 * Load all connections from the configuration directory
 * @param conn_start    Can be the last item of the connections list to speed up iteration, but can be the head. If it is NULL, than @ref mp_global_conn will be used.
 */
void connection_load_dir(connection_type * conn_start)
{
    DIR *dp;
    struct dirent *ep;

    dp = opendir("conf/connections");
    if (!dp) {
	fprintf(stderr, "Failed to open directory");
	return;
    }
    conn_load("conf.template", &template_conn);

    while ((ep = readdir(dp))) {
	// open only .conf files
        if (strlen(ep->d_name) < 5 || strcmp(ep->d_name + strlen(ep->d_name) - 5, ".conf") != 0)
            continue;

        conn_load(ep->d_name, NULL);  // load config file, append to connection list

    } // while ep...
    closedir(dp);
}

/**
 * Read a connection from the configuration file opened and save values in the @ref connection_struct
 * @param conn  The destination connection, must contain the name of the file in the filename field
 */
void conn_parser(connection_type * conn)
{
    char file_path[255];
    dictionary *conf;
    //sprintf(file_path, "conf/connections/%s", conn->filename);
    chdir("conf/connections");
    conf = iniparser_load(conn->filename);
    chdir("../..");


    if (conf == NULL) {
	fprintf(stdout, "cannot parse file: %s\n", file_path);
	exerror("Critical error.", 3);
    }
    conn_setup_fromdict(conn, conf);
}

/**
 * Create a new empty connection and append it to the end of the list
 * @return Pointer to the new connection
 */
connection_type *conn_new(void)
{
    connection_type *cp, *conn;

    cp = (connection_type *) malloc(sizeof(connection_type));
    memset(cp, 0, sizeof(connection_type));

    // NULL means use global connections list
    if (!mp_global_conn) {
            mp_global_conn = cp;
    } else {
            conn = mp_global_conn;
            while (conn->next) conn = conn->next;
            conn->next = cp;
            cp->prev = conn;
    }

    cp->status = CONN_STAT_EMPTY;
    return cp;
}

/**
 * Load all connections from a specified file
 * @param filename      The file to read
 * @param conn_start    Can be the last item of the connections list to speed up iteration, but can be the head. If it is NULL, than @ref mp_global_conn will be used.
 */
void conn_load(char *filename, connection_type * conn_start)
{
    FILE *fd;
    connection_type *conn;
    char lipstr[128], ripstr[128];
    char line[256];
   // int num;

    // search end
    if (conn_start) {
        conn = conn_start;
    } else {
        conn = conn_new();
    }

    strcpy(lipstr, "");
    strcpy(ripstr, "");
    chdir("conf/connections");
    fd = fopen(filename, "r");
    chdir("../..");
    if (fd == NULL) {
	sprintf(line, "\nFile read open error: %s\n", filename);
	exerror(line, 3);
    }
DEBUG("Loading connection information from file %s\n", filename);
    strcpy(conn->filename, filename);
    fclose(fd);
DEBUG("Starting to parse connection file %s\n", filename);
    conn_parser(conn);
//    conn_print_item(stdout, conn);
}



/**
 * Used for conn_create() to merge the server's template configuration with the one received from client.
 *
 * @param dst   The connection based on the server's configuration template
 * @param src   The configuration received from the remote host
 */
void conn_merge(connection_type * dst, connection_type *ctemplate, connection_type * peer)
{
    path_type *ppath, *cpath, *dpath;
    int i, ipv, ips;
    char ipstr[128];
    // copy some settings
    memcpy(&dst->name, peer->name, sizeof(peer->name));
    set_ipstr(ipstr, peer->ip_local, peer->ip_version);
    for (i=0; i<strlen(ipstr); i++) if ((ipstr[i] == ':') || (ipstr[i] == '.')) ipstr[i] = '_';
    sprintf(dst->filename, "peer_%s.conf", ipstr);
    dst->permission = ctemplate->permission; //permisson keeps template value
    dst->ip_version = peer->ip_version;
    if (dst->ip_version == 4) {
        dst->ip_local[2] = 0xFFFF0000;
        dst->ip_local[3] = tun.ip4;
    }
    else {
        memcpy(dst->ip_local, tun.ip6, SIZE_IN6ADDR);
    }
    dst->port_local = ctemplate->port_local;
    dst->cmd_port_local = tun.cmd_port_rcv;
    memcpy(dst->ip_remote, peer->ip_local, SIZE_IN6ADDR);
    dst->port_remote = peer->port_local;
    dst->cmd_port_remote = peer->cmd_port_local;
    // tpc = ctemplate->path_count;
		//smaller path count win
    dst->path_count = peer->path_count < ctemplate->path_count ? peer->path_count : ctemplate->path_count;
    dst->network_count = (0 == peer->network_count ? 0 : 1) ;
    dst->status = ctemplate->status;
    dst->reorder_window = peer->reorder_window > ctemplate->reorder_window ? peer->reorder_window : ctemplate->reorder_window;
    dst->max_buffdelay = ctemplate->max_buffdelay;
    dst->auth_type = peer->auth_type;
    memcpy(&dst->auth_key, &peer->auth_key, keySize(peer->auth_type));

    // ********************** Connection ready, paths comes
    /**********  The paths of ctemplate and peer must be ordered so, that the versions of the paths are the same ***/

    for(i=0; i<dst->path_count; i++)
    {   ppath = &peer->mpath[i];
        ipv=ppath->ip_version;
        ips = SIZE_IN6ADDR;
        cpath = &ctemplate->mpath[i];
        dpath = &dst->mpath[i];
        dpath->ip_version = ipv;
        if (cpath != dpath) {
            strcpy(dpath->interface, cpath->interface);
            memcpy(dpath->ip_private, cpath->ip_private, ips);
            memcpy(dpath->ip_public,  cpath->ip_public,  ips);
            memcpy(dpath->ip_gw,      cpath->ip_gw,      ips);
        }
        memcpy(dpath->ip_remote, ppath->ip_public, ips);
        dpath->keepalive = ppath->keepalive;
        dpath->deadtimer = ppath->deadtimer;
        dpath->weight_out = ppath->weight_in;
        dpath->weight_in  = ppath->weight_out;
        dpath->status = cpath->status;
    }

    //  Paths are ready, networks comes
    if (peer->network_count) {
        dst->networks[0].version = peer->ip_version;
        memset(dst->networks[0].source, 0, SIZE_IN6ADDR);
        dst->networks[0].source_prefix_length = 0;
        memcpy(dst->networks[0].destination, peer->ip_local, SIZE_IN6ADDR);
        if (peer->ip_version == 4) {
            dst->networks[0].destination_prefix_length = 32;
        } else {
            dst->networks[0].destination_prefix_length = 128;
        }
    }

}



int connection_write_memory(char *memptr, connection_type *conn, bit_32 memsize)
{
    char *buff = memptr;
    FILE * fd;
    fd = fmemopen(buff, memsize, "wb");
    memset(buff, 0, memsize);
    connection_save(fd, conn);
    fflush(fd);
    fclose(fd);
    return(strlen(memptr));
}


int connection_read_memory(connection_type *conn, char *memptr, bit_32 memsize)
{
    FILE * fd;
    char *buff = memptr;
    dictionary *conf;

    fd = fmemopen(buff, memsize, "r");
    if (!fd) {
        printf("connection_read_memory:  fmemopen failed\n");
        return(0);
    }
//DEBUG("before iniparser load\n");
    conf = iniparser_load_descriptor(fd, NULL);
    fclose(fd);
    if (!conf) {
         printf("connection_read_memory error: cannot parse ini file from memory\n");
         return(0);
    }
//DEBUG("after iniparser load\n");
    conn_setup_fromdict(conn, conf);
    return(1);
}

// read the connection, path and network configuration from a dictionary
void conn_setup_fromdict(connection_type *conn, dictionary *conf)
{
    char section[255];
    int index;
    path_type *p;
    network_type *n;

    _setup_con(conf, conn);
    p = conn->mpath;
    for (index = 0; index < conn->path_count; ++index, ++p) {
        sprintf(section, "path_%d", index);
        p->connection = conn;
        _setup_pth(conf, section, p);
        if (p->cmd_default) conn->default_cmd_path = p;
//        p->peer.sin6_family = AF_INET6;
//        p->peer.sin6_port = htons(conn->port_remote);
//        memcpy(&p->peer.sin6_addr, p->ip_remote, SIZE_IN6ADDR);
    }
    n = conn->networks;
    for (index = 0; index < conn->network_count; ++index, ++n) {
        sprintf(section, "net_%d", index);
        _setup_net(conf, section, n);
    }
}


void _setup_con(dictionary * conf, connection_type * conn)
{
    char *read_str;
    char nullip[128];
    int32_t af, start;

    read_str = iniparser_getstring(conf, "connection:name", "connection");
    strcpy(conn->name, read_str);

    conn->permission = iniparser_getint(conf, "connection:permission", 0);
    conn->ip_version = iniparser_getint(conf, "connection:ip_ver", 6);
    if (conn->ip_version == 6) {
        af = AF_INET6;
        start = 0;
        strcpy(nullip, "::");
    } else {
        af = AF_INET;
        conn->ip_version = 4;
        start = 3;
        strcpy(nullip, "0.0.0.0");
        conn->ip_local[2] = 0xFFFF0000;
        conn->ip_remote[2] = 0xFFFF0000;
    }
    read_str = iniparser_getstring(conf, "connection:ip_local", nullip);
    inet_pton(af, read_str, &(conn->ip_local[start]));

    conn->port_local = iniparser_getint(conf, "connection:local_port", 0);
    conn->cmd_port_local = iniparser_getint(conf, "connection:local_cmd_port", 65001);

    read_str =
	iniparser_getstring(conf, "connection:ip_remote", "nullip");
    inet_pton(af, read_str, &(conn->ip_remote[start]));

/******* we do not use remote_cmd_ip
    read_str =
	iniparser_getstring(conf, "connection:remote_cmd_ip", nullip);
	conn->cmd_ip_remote = NULL;
    if (strcmp(read_str, nullip)){
        conn->cmd_ip_remote = malloc(SIZE_IN6ADDR);
        memset(conn->cmd_ip_remote, 0, SIZE_IN6ADDR);
        if (strchr(read_str, ':')) { // IPv6 address
            inet_pton(AF_INET6, read_str, conn->cmd_ip_remote);
        } else {
            conn->cmd_ip_remote[2] = 0xFFFF0000;
            inet_pton(AF_INET, read_str, &conn->cmd_ip_remote[3]);
        }
    }
***************/

    conn->port_remote =
	iniparser_getint(conf, "connection:remote_port", 65000);
    conn->cmd_port_remote =
	iniparser_getint(conf, "connection:remote_cmd_port", 65050);
    conn->path_count = iniparser_getint(conf, "connection:path_count", 0);
    conn->network_count =
	iniparser_getint(conf, "connection:network_count", 0);
    conn->status = iniparser_getint(conf, "connection:status", CONN_STAT_DOWN);
    conn->reorder_window = iniparser_getint(conf, "connection:reorder_window", 0);

    read_str = iniparser_getstring(conf, "connection:auth_key", DEFAULT_KEY);
    strcpy(conn->auth_key, read_str);
DEBUG("Key: %s\n", conn->auth_key);
    conn->auth_type = iniparser_getint(conf, "connection:auth_type", 0);
    memset(&conn->max_buffdelay, 0, sizeof(struct timeval) );
    int iv = iniparser_getint(conf, "connection:max_buffdelay_msec", 0);
    if (!iv) iv = 100; // max_buffdelay may not be zero
    conn->max_buffdelay.tv_sec = iv / 1000;
    conn->max_buffdelay.tv_usec = (iv % 1000) *1000;
    conn->default_cmd_path = NULL;
}

void _setup_pth(dictionary * conf, char *section, path_type * path)
{
    char conf_key[255], nullip[128];
    char lipstr[128], gwstr[128], *gwstrpoi;
    char *read_str;
    int32_t af;
    int32_t start;

    sprintf(conf_key, "%s:interface_name", section);
    read_str = iniparser_getstring(conf, conf_key, "unnamed_path");
    strcpy(path->interface, read_str);

    sprintf(conf_key, "%s:ip_ver", section);
    path->ip_version = iniparser_getint(conf, conf_key, 0);

    sprintf(conf_key, "%s:cmd_default", section);
    path->cmd_default = iniparser_getint(conf, conf_key, 0);
//    if (path->cmd_default) conn->default_cmd_path = path;

//    sprintf(conf_key, "%s:local_macaddr", section);
//    read_str = iniparser_getstring(conf, conf_key, "00:00:00:00:00:00");
//    mac_pton(read_str, (char *) path->mac_local);

    if (path->ip_version == 6) {
        af = AF_INET6;
        start = 0;
        strcpy(nullip, "::");
    } else {
        af = AF_INET;
        path->ip_version = 4;
        start = 3;
        strcpy(nullip,"0.0.0.0");
        path->ip_private[2] = 0xFFFF0000;
        path->ip_public[2] = 0xFFFF0000;
        path->ip_remote[2] = 0xFFFF0000;
        path->ip_gw[2] = 0xFFFF0000;
    }

    sprintf(conf_key, "%s:gw_ipaddr", section);
    read_str = iniparser_getstring(conf, conf_key, nullip);
    strcpy(gwstr, read_str);
    gwstrpoi = NULL;
    if (!strcmp(gwstr, "0.0.0.0")) {
        if (get_gatewayip4(gwstr, path->interface) ) gwstrpoi = gwstr;
    }
    inet_pton(af, gwstr, &(path->ip_gw[start]));

    sprintf(conf_key, "%s:private_ipaddr", section);
    read_str = iniparser_getstring(conf, conf_key, nullip);
    strcpy(lipstr, read_str);
    if ((path->ip_version == 4) && (!strcmp(lipstr, "0.0.0.0"))) get_privateip4(lipstr, path->interface);
    inet_pton(af, lipstr, &(path->ip_private[start]));
// DEBUG("Private IP: %s \n", lipstr);

    sprintf(conf_key, "%s:public_ipaddr", section);
    read_str = iniparser_getstring(conf, conf_key, nullip);
    if (strcmp(read_str, "private_ipaddr")) {
        strcpy(lipstr, read_str);
        if ((path->ip_version == 4) && (!strcmp(lipstr, "0.0.0.0"))) get_globalip4(lipstr, path->interface, gwstrpoi);
    }
    inet_pton(af, lipstr, &(path->ip_public[start]));

//    sprintf(conf_key, "%s:gw_macaddr", section);
//    read_str = iniparser_getstring(conf, conf_key, "00:00:00:00:00:00");
//    mac_pton(read_str, (char *) path->mac_gw);

    sprintf(conf_key, "%s:remote_ipaddr", section);
    read_str = iniparser_getstring(conf, conf_key, nullip);
    inet_pton(af, read_str, &(path->ip_remote[start]));

    sprintf(conf_key, "%s:keepalive_time", section);
    path->keepalive = iniparser_getint(conf, conf_key, 0);

    sprintf(conf_key, "%s:dead_time", section);
    path->deadtimer = iniparser_getint(conf, conf_key, 3*path->keepalive);


    sprintf(conf_key, "%s:weight_in", section);
    path->weight_in = iniparser_getint(conf, conf_key, 1);

    sprintf(conf_key, "%s:weight_out", section);
    path->weight_out = iniparser_getint(conf, conf_key, 1);

    sprintf(conf_key, "%s:window_size", section);
    path->packet_max = iniparser_getint(conf, conf_key, 1);

    sprintf(conf_key, "%s:status", section);
    path->status = iniparser_getint(conf, conf_key, 0);

    connection_type *conn = (connection_type *)path->connection;
    sprintf(conf_key, "%s:tcp_src", section);
    read_str = iniparser_getstring(conf, conf_key, "");
    populate_lut(conn->tcp_src_lut, read_str, path);

    sprintf(conf_key, "%s:tcp_dst", section);
    read_str = iniparser_getstring(conf, conf_key, "");
    populate_lut(conn->tcp_dst_lut, read_str, path);

    sprintf(conf_key, "%s:udp_src", section);
    read_str = iniparser_getstring(conf, conf_key, "");
    populate_lut(conn->udp_src_lut, read_str, path);

    sprintf(conf_key, "%s:udp_dst", section);
    read_str = iniparser_getstring(conf, conf_key, "");
    populate_lut(conn->udp_dst_lut, read_str, path);
}

void _setup_net(dictionary * conf, char *section, network_type * net)
{
    char conf_key[255], lipstr[255];
    char *read_str;
    int32_t version;
    int32_t offset;

    sprintf(conf_key, "%s:ip_ver", section);
    net->version = iniparser_getint(conf, conf_key, 4);
    version = (net->version == 4) ? AF_INET : AF_INET6;
    offset = 0;
    if (net->version == 4) {
	offset = 3;
	net->source[2] = 0xFFFF0000;
	net->destination[2] = 0xFFFF0000;
    }

    sprintf(conf_key, "%s:src_addr", section);
    read_str = iniparser_getstring(conf, conf_key, "0.0.0.0");
    strcpy(lipstr, read_str);
    read_str = strchr(lipstr, '/');
    if (read_str) {
        read_str[0] = '\0';
        read_str++;
        sscanf(read_str,"%d", &net->source_prefix_length);
    }
    inet_pton(version, lipstr, &net->source[offset]);

    sprintf(conf_key, "%s:dst_addr", section);
    read_str = iniparser_getstring(conf, conf_key, "0.0.0.0");
    strcpy(lipstr, read_str);
    read_str = strchr(lipstr, '/');
    net->destination_prefix_length = 0;
    if (read_str) {
        read_str[0] = '\0';
        read_str++;
        sscanf(read_str,"%d", &net->destination_prefix_length);
    }
    inet_pton(version, lipstr, &net->destination[offset]);

}

// set the IP string from binary value, according to the version
// The result output variable must have minimum 64 bytes place to store the output string
void set_ipstr(char *result, bit_32 * ip, bit_8 version)
{
    int result_size = 64;
    if (version == 6) {
	inet_ntop(AF_INET6, ip, result, result_size);
    } else if (version == 4) {
	inet_ntop(AF_INET, &ip[3], result, result_size);
    }
}

void populate_lut(path_type *p[], char *port_nums, path_type *current_path)
{
  if(strcmp(port_nums, "") == 0 || port_nums == NULL)
    return;
  char *tok;
  tok = strtok(port_nums, " ,:;");
  while(tok != NULL){
    p[htons(atoi(tok))] = current_path;
    tok = strtok(NULL, " ,:;");
  }
} 