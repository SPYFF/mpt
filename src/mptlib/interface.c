/**
 * @file
 * @brief Basic operations on an interface
 * @author Almasi, Bela; Debrecen, Hungary
 * @date 2012.08.15.
 * @copyright Project maintained by Almasi, Bela; Debrecen, Hungary
 */

#define MPT_DEBUG

#include "multipath.h"
#include "mp_local.h"
#include "iniparser.h"

/**
 * Convert the MAC address string (':' or '-' byte delimiter) to binary form
 */
void mac_pton(char *src, char *dst)
{
    int slen;
    unsigned char *p, *d, c;
    memset(dst, 0, 6);

    if ((slen = strlen(src)) != 17)
	return;			// Bad format input MAC string
    p = (unsigned char *) src;
    d = (unsigned char *) dst;
    while (p < p + slen) {
	c = toupper(*p);
	if (c >= 'A' && c <= 'F') {
	    *d = (c - 'A' + 10) << 4;
	} else
	    *d = (c - '0') << 4;
	p++;
	c = toupper(*p);
	if (c >= 'A' && c <= 'F') {
	    *d |= (c - 'A' + 10);
	} else
	    *d |= (c - '0');

	p++;
	if ((*p == ':') || (*p == '-')) {
	    p++;
	} else
	    return;
	d++;
    }
    return;
}

void mac_ntop(char *src, char *dst)
{
    sprintf(dst, "%02X:%02X:%02X:%02X:%02X:%02X", src[0], src[1], src[2],
	    src[3], src[4], src[5]);
}

/**
 * Check if the address is loopback (Ret. 1=yes)
 */
int check_loopback(char *address)
{
    bit_8 lo4[SIZE_IN6ADDR], lo6[SIZE_IN6ADDR];
    inet_pton(AF_INET6, "::FFFF:127.0.0.1", lo4);
    inet_pton(AF_INET6, "::1", lo6);
    if (!memcmp(lo4, address, SIZE_IN6ADDR))
	return (4);
    if (!memcmp(lo6, address, SIZE_IN6ADDR))
	return (6);
    return (0);
}


/**
 * Change the path status for all path of the intercace
 */
void interface_change_status(char *interface, bit_8 status)
{
    connection_type *conn;
    int i;

    conn = mp_global_conn;
    while (conn) {
	for (i = 0; i < conn->path_count; i++) {
	    if (strcmp(interface, conn->mpath[i].interface) == 0) {
		path_change_status(conn, i, status);
	    }
	}			// for
	conn = conn->next;
    }				// while
}

/**
 * Change the path status for all path of the address
 */
void address_change_status(bit_32 * addr, bit_8 status)
{
    connection_type *conn;
    int i;

    conn = mp_global_conn;
    while (conn) {
	for (i = 0; i < conn->path_count; i++) {
//   int j; printf("\n"); for(j=0; j<4; j++) printf("%08X - %08X \n", conn->mpath[i].ip_local[j], addr[j]);
	    if (memcmp(addr, conn->mpath[i].ip_public, SIZE_IN6ADDR) == 0) {
		path_change_status(conn, i, status);
	    }
	}			// for
	conn = conn->next;
    }				// while
}


/**
 * Check the status of an interface, value 1 means UP
 */
int check_interface(char *interface, int sock)
{
    struct ifreq ifr;

    strncpy(ifr.ifr_name, interface, 16);
    ioctl(sock, SIOCGIFFLAGS, &ifr);
    if (((ifr.ifr_flags) & IFF_UP) && ((ifr.ifr_flags) & IFF_RUNNING)) {
	return (1);
    }
    return (0);
}

/**
 * Load the data of the interfaces from a file
 */
void interface_load(char *filename, tunnel_type * tunp)
//		    interface_type * interface)
{
    dictionary *conf;
    FILE *fd;
    int if_num;
    int tun_num, local_cmdport;
    char section[255];
    char line[256];
    int index;

    if (!tunp) tunp = &tun;
    fd = fopen(filename, "r");
    if (fd == NULL) {
	sprintf(line, "\nFile read open error: %s\n", filename);
	exerror(line, 3);
    }
    fclose(fd);

    conf = iniparser_load(filename);
    if (conf == NULL) {
	sprintf(line, "cannot parse file: %s\n", filename);
	exerror(line, 3);
    }

    strcpy(tunp->device, "/dev/net/tun");
    mp_global_server = iniparser_getint(conf, "general:accept_remote", 0);
    if_num = iniparser_getint(conf, "general:interface_num", 0);
    tun_num = iniparser_getint(conf, "general:tunnel_num", 0);
    local_cmdport = iniparser_getint(conf, "general:cmdport_local", 65050);
    tunp->cmd_port_rcv = local_cmdport;

    if (if_num < 0) {
	sprintf(line, "Interface number is too small.\n");
	exerror(line, 4);
    }

    if (tun_num < 1) {
	sprintf(line, "Tunnel number is too small.\n");
	exerror(line, 4);
    }

    for (index = 0; index < tun_num; ++index) {
	sprintf(section, "tun_%d", index);

    char conf_key[255];
    char *read_str;
    char *lstr;

    sprintf(conf_key, "%s:name", section);
    read_str = iniparser_getstring(conf, conf_key, "tun0");
    strcpy(tunp->interface, read_str);

// cmdport_local was moved to the general section - 2015.09.04
//    sprintf(conf_key, "%s:cmdport_local", section);
//    tun->cmd_port_rcv = iniparser_getint(conf, conf_key, 65050);

    sprintf(conf_key, "%s:mtu", section);
    tunp->mtu = iniparser_getint(conf, conf_key, 1440);
    sprintf(conf_key, "%s:ipv4_addr", section);
    read_str = iniparser_getstring(conf, conf_key, "0.0.0.0/0");
    lstr = trim((char *) strtok(read_str, "/#\n"));
    inet_pton(AF_INET, lstr, &tunp->ip4);
    lstr = trim((char *) strtok(NULL, "/#\n"));
    tunp->ip4len = atoi(lstr);

    sprintf(conf_key, "%s:ipv6_addr", section);
    read_str = iniparser_getstring(conf, conf_key, "FE::1/0");
    lstr = trim((char *) strtok(read_str, "/#\n"));
    inet_pton(AF_INET6, lstr, &tunp->ip6);
    lstr = trim((char *) strtok(NULL, "/#\n"));
    tunp->ip6len = atoi(lstr);
    } // for (index...

DEBUG("Tunnels are configured.\n");

}



int is_ppp(char * ifname)
{
	struct ifreq req;
	memset(&req, 0, sizeof(struct ifreq));
	strcpy(req.ifr_name, ifname);
	int sock, err;
	sock = socket(AF_INET, SOCK_DGRAM,0);
	if(sock == -1)
	{
		perror("PPP checking: socket creation error");
		return -1;
	}

	err = ioctl(sock, SIOCGIFFLAGS,&req);
	close(sock);

	if(err == -1)
	{
		fprintf(stderr, "PPP checking: Interface %s ioctl error\n",ifname);
		return -2;
	}

	if(req.ifr_flags & IFF_POINTOPOINT)
		return 1;

	return 0;
}



// get the ipv4 default gateway address for an interface
int get_gatewayip4( char *gw_ip, char* ifname)
{
    FILE *popn_f;
	char popn_s[128], cmd[128], buff[128];
    int sock, ret;
    struct ifreq req;
    bit_32 addr, *netmask;

    ret = 0;
    strcpy(gw_ip, "0.0.0.0");
  DEBUG("Starting gateway IPv4 address determination for interface %s\n", ifname);
    get_privateip4(buff, ifname);
    if (is_ppp(ifname) == 1)
    {
    DEBUG("PPP peer address determination starts\n");
        memset(&req, 0, sizeof(struct ifreq));
        strcpy(req.ifr_name, ifname);
        if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
            perror("socket error ");
            // exit(errno);
        }
        if (ioctl(sock, SIOCGIFDSTADDR, &req) == -1) {
            perror("ioctl");
            // exit(errno);
        }
        close(sock);
        inet_ntop(AF_INET, (void *) &(((struct sockaddr_in *)(&(req.ifr_dstaddr)))->sin_addr), gw_ip, 64);
     DEBUG("PPP peer address found : %s", gw_ip);
        ret = 2;
        return ret;
    }
    if (is_ppp(ifname) <= 0) { // not ppp interface
        sprintf(cmd, "bin/get_gwip %s", ifname);
    DEBUG("Starting %s \n", cmd);
        popn_f = popen(cmd,"r");
        fgets(popn_s,128,popn_f);
        trim(popn_s);
        pclose(popn_f);
        if (isdigit(popn_s[0])) {
            strcpy(gw_ip, popn_s);
            ret = 3;
        } else {
            inet_pton(AF_INET, buff, &addr);
    DEBUG("NMCLI failed. Getting first host address (assumed as gateway) for IP (str %s),  %08X \n", buff, addr);
            if (addr) {
                memset(&req, 0, sizeof(struct ifreq));
                strcpy(req.ifr_name, ifname);
                if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
                    perror("socket creation error");
                }
                if (ioctl(sock, SIOCGIFNETMASK, &req) == -1) {
                    perror("ioctl error at netmask read");
                }
                close(sock);
                netmask = (bit_32 *) &(((struct sockaddr_in *)(&(req.ifr_dstaddr)))->sin_addr);
                addr = addr & netmask[0];
                addr = (unsigned) ntohl(htonl(addr) +1);
                inet_ntop(AF_INET, &addr, gw_ip, 64);
                ret = 1;
            } // addr
        } // isdigit else
    } // not is_ppp
    DEBUG("Gateway IPv4 for int. %s: %s\n", ifname, gw_ip);
    return ret;
}
