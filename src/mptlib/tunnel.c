/**
 * @file
 * @brief Tunnel handling functions
 * @author Almasi, Bela; Debrecen, Hungary
 * @date 2012.08.15.
 * @copyright Project maintained by Almasi, Bela; Debrecen, Hungary
 */

#include "multipath.h"
#include "mp_local.h"

#define OTUNSETIFF     (('T'<< 8) | 202)


/**
 * Open and config the tunnel interface
 */
int tunnel_start (tunnel_type *tunp)
{
    char cmdbuf[256], ipstr[256];
    //int mtu = 1420; // 1500-sizeof(IPv6+UDP+GRE header)
    int qlen = 1500, sockfd;
    struct ifreq ifr;
    if (!tunp) tunp = &tun; // we use the global tun if called with NULL argument

    strcpy(tunp->device, "/dev/tun");

    if ((tunp->fd = open(tunp->device, O_RDWR)) < 0)
        exerror("Tunnel inteface opening error. ", errno);
    //mp_global_tun = tun;

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;  // for tun (point-to-point) interface
    strncpy(ifr.ifr_name, tunp->interface, IFNAMSIZ);
    if (ioctl(tunp->fd, TUNSETIFF, (void *) &ifr) < 0) {
        /* Try old ioctl */
        if (ioctl(tunp->fd, OTUNSETIFF, (void *) &ifr) < 0) {
            close(tunp->fd);
            exerror("Tunnel interface IOCTL error.", errno);
        }
    }
    sockfd = socket(PF_INET, SOCK_DGRAM, 0); // set the outgoing que length
    if (sockfd >0) {
        	strcpy(ifr.ifr_name, tunp->interface);
            ifr.ifr_qlen = qlen;
            if (ioctl(sockfd, SIOCSIFTXQLEN, &ifr) < 0) {
                fprintf(stderr, "Tunnel interface qlen IOCTL error - %d.", errno);
            }
    close(sockfd);
    }

    if (tunp->ip4len) {
        inet_ntop(AF_INET, &tunp->ip4, ipstr, 255);
        sprintf(cmdbuf,"sh bin/mpt_tunnel_start.sh ipv4 %s %s/%d %d",
	        tunp->interface, ipstr, tunp->ip4len, tunp->mtu);
printf("Cmd: %s\n", cmdbuf);
DEBUG("Initializing tunnel interfaces tunnel_start.sh\n");
        system(cmdbuf);
    }

    if (tunp->ip6len ) {
        inet_ntop(AF_INET6, &tunp->ip6, ipstr, 255);
        sprintf(cmdbuf,"sh bin/mpt_tunnel_start.sh ipv6 %s %s/%d %d",
	        tunp->interface, ipstr, tunp->ip6len, tunp->mtu);
printf("Cmd: %s\n", cmdbuf);
        system(cmdbuf);
    }

    cmd_open_socket(tunp);
    mpt_start;
    buff_start;
DEBUG("Starting tunnel threads\n");
    pthread_create(&tunp->tunnel_read, NULL,   tunnel_read_thread , tunp);
    pthread_create(&tunp->cmd_read, NULL,   cmd_read_thread , NULL);
    return(tunp->fd);
}


/**
 * Open and bind the cmd socket
 */
int cmd_open_socket( tunnel_type *tunp )
{
    int sock6;
    unsigned int socklen;
    struct sockaddr_in6 sockaddr6;
    int off = 0;

    socklen = sizeof(sockaddr6);
    sockaddr6.sin6_family = AF_INET6;
    memcpy(&(sockaddr6.sin6_addr), &in6addr_any, SIZE_IN6ADDR);

    sockaddr6.sin6_port   = htons(tunp->cmd_port_rcv); // ************* CMD server socket
    if ((sock6 = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
            reterror("Socket creation error.", errno);

    setsockopt(sock6, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&off, sizeof off);
    // IPV6_only is turned off, so this socket will be used for IPv4 too.

    if (bind(sock6, (struct sockaddr *) &sockaddr6, socklen) <0) {
            close(sock6);
            exerror("CMD server socket bind error.", errno);
    }
    if (tunp->cmd_port_rcv == 0) { // The port number was not known.
            getsockname(sock6, (struct sockaddr *) &sockaddr6, &socklen);
            tunp->cmd_port_rcv = ntohs(sockaddr6.sin6_port);
    }
    tunp->cmd_socket_rcv = sock6;

//    sockaddr6.sin6_port   = htons(tun->cmd_port_snd); // ************* CMD client socket
//    if ((sock6 = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
//            reterror("Socket creation error.", errno);
//
//    setsockopt(sock6, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&off, sizeof off);
//    // IPV6_only is turned off, so this socket will be used for IPv4 too.
//
//    if (bind(sock6, (struct sockaddr *) &sockaddr6, socklen) <0) {
//            close(sock6);
//            exerror("CMD client socket bind error.", errno);
//    }
//    if (tun->cmd_port_snd == 0) { // The port number was not known.
//            getsockname(sock6, (struct sockaddr *) &sockaddr6, &socklen);
//            tun->cmd_port_snd = ntohs(sockaddr6.sin6_port);
//    }
//    tun->cmd_socket_snd = sock6;

    return(0);
}


/**
 * Stop and close the tunnel device
 */
int tunnel_stop(tunnel_type *tunp)
{
    if (!tunp) tunp = &tun; // if called with NULL we use the global tun variable
    mpt_end;
    buff_end;
    pthread_kill(tunp->tunnel_read,SIGUSR1);
    pthread_kill(tunp->cmd_read,SIGUSR1);
    close(tunp->cmd_socket_rcv);
//    close(tun->cmd_socket_snd);
    return close(tunp->fd);
}

