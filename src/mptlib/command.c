/**
 * @file
 * @brief Handling of network commands
 * @date 2012.08.15
 * @author Almasi, Bela; Debrecen, Hungary
 * @copyright Project maintained by Almasi, Bela; Debrecen, Hungary
 */

//#define MPT_DEBUG

#include "multipath.h"
#include "mp_local.h"
#include "auth.h"




/**
 * Do the command received from the network peer. This is used in cmd_read_thread()
 *
 * @param[in]      conn  The connection of the network peer
 * @param[in,out]  buff  Contains the message received. After execution, it will contain the response message
 * @param[in,out]  blen  The size of the buff param
 * @return Returns with 0 means OK, other values are error codes, which is set in buff[1] too
 */
char do_command(connection_type *conn, char *buff, int * blen)
{
    connection_type * recvd_conn, * ctemplate;
    connection_type tmp_conn;
    char lipstr[128], ripstr[128];
    int i;
    char ret=0;
    bit_8 cmd, value, sh;

    cmd = buff[0];
    value = buff[1];
    sh = authSize(buff[2]);

    if (!conn && cmd != CMD_CONNECTION_CREATE) {
        buff[1] = 4;
        return 4;
    }

    if (cmd == CMD_CONNECTION_CREATE) {
DEBUG("%s\nConnection create starts with buffer above \n", buff+44);
        recvd_conn = &tmp_conn;
        memset(recvd_conn, 0, sizeof(connection_type));
        if (!connection_read_memory(recvd_conn, &buff[sh+44], strlen(buff+sh+44)) ){
            buff[1] = 4;
            return 4;
        }
DEBUG("conn_read_memory finished\n");

/********* Do not need to test the name, teh IP pair is the key to search
        ret = 1;
        template = conn_search_name( recvd_conn->name);
        if ((template) && ( template != conn )) {
            *blen = 41 + sh;
            char flag = 1;
        DEBUG("Connection create failed - connection name already exists (%s) \n", recvd_conn->name);
            connection_delete(conn);
            memcpy(&buff[sh+40], &flag, sizeof(char));
            ret = 0;
        }
**********/
        i=0; if(recvd_conn->ip_version == 4) i=3;
        ctemplate = conn_search_ip(recvd_conn->ip_version, &recvd_conn->ip_remote[i], &recvd_conn->ip_local[i], mp_global_conn);
        if (ctemplate) {
            *blen = 41 + sh;
            char flag = 1;
        DEBUG("Connection create failed - connection tunnel IP pair already exists (%s) \n", recvd_conn->name);
            //connection_delete(conn);
            memcpy(&buff[sh+40], &flag, sizeof(char));
            buff[1] = 5;
            ret = 5;
            return(ret);
        }

        { // New connection, load data
        DEBUG("Starting the creation & merge of the new connection \n");
            ctemplate = conn_new();
            conn = ctemplate;

            conn_merge(ctemplate, &template_conn, recvd_conn);
            template_conn.port_local++;
//            ctemplate->port_local = 0; //bind to random port
//            ctemplate->status = CONN_STAT_OK;

            connection_start(ctemplate);
            conn_print_item(stdout, ctemplate);
            connection_write_memory(&buff[sh+40], ctemplate, SIZE_DGRAM);
            *blen = 40+strlen(&buff[sh+40]);
//            memcpy(&buff[sh+40], ctemplate, sizeof(connection_type));

            ret = 0;
        }
        buff[1] = ret;
        return(ret);
    }
    else {
        if (!conn) {
            buff[1] = 4;
            return 4;
        }
    }

    if (cmd == CMD_P_STATUS_CHANGE) {
        for (i=0; i < conn->path_count; i++ ) {
            if ((memcmp(&buff[sh+40], conn->mpath[i].ip_remote, SIZE_IN6ADDR) == 0) &&
                (memcmp(&buff[sh+56], conn->mpath[i].ip_public, SIZE_IN6ADDR) == 0) &&
                (conn->mpath[i].status != value) ) {
            if (value == STAT_OK ) { // avoid dead timer timeout
            conn->mpath[i].last_keepalive = time(NULL);
            }
                    conn->mpath[i].status = value;
                    if (conn->mpath[i].ip_version == 6) {
                        inet_ntop(AF_INET6, &buff[sh+56], lipstr, 128);
                        inet_ntop(AF_INET6, &buff[sh+40],  ripstr, 128);
                    }
                    else {
                        inet_ntop(AF_INET, &buff[sh+68], lipstr, 128);
                        inet_ntop(AF_INET, &buff[sh+52],  ripstr, 128);
                    }
                    fprintf(stderr, "Path status changed to 0x%02X (%s -> %s) \n", value, lipstr, ripstr);
            }
        } // for

        // confirm as OK
        *blen = 40+sh;
        ret = 0;
    } // CMD_P_STATUS_CHANGE

/****************** connection crate is used instead
    if (cmd == CMD_CONNECTION_SEND) {
        connection_type new;
        memset(&new, 0, sizeof(connection_type));

        conn_mirror(&new, (connection_type *)&buff[sh+40]);

        // TODO: Ha már létezik a connection adott névvel, akkor onnan átvenni gatewayeket

        conn_activate(&new, 0);

        // Send back port number
        *blen = 42+sh;
        memcpy(&buff[sh+40], &conn->port_local, 2);
        ret = 1;
    } // CMD_CONNECTION_SEND
********************************/
    buff[1] = ret;
    return ret;
}


/**
 * Common communication function implementing four-way handshake
 *
 * @param[in]           conn    The connection to use
 * @param[in]           pind    Index of the path to use
 * @param[in]           cmd     Which command to send. See @ref CMDValues
 * @param[in]           status  Which subcommand
 * @param[in,out]       size    Size of the data
 * @param[in,out]       buff    The data to send out. This function will add the mpt headers
 *
 * @return This function will return with an error code differs from 0 in case of communication failure
 *
 * @msc
 *      arcgradient = 8;
 *      a [label="handshake()"], b [label="cmd_read_thread()"];
 * # a rbox a [label="authSet()"];
 *      a=>b [label="round=1, data size"];
 * # b rbox b [label="authTest()"];
 * # b rbox b [label="malloc(size)"];
 * # b rbox b [label="authSet()"];
 *      a<=b [label="round=2, data size"];
 * # a rbox a [label="authTest()"];
 * # a rbox a [label="memcpy(data)"];
 * # a rbox a [label="authSet()"];
 *      a=>b [label="round=3, data"];
 * # b rbox b [label="authTest()"];
 * b rbox b [label="do_command()"];
 * # b rbox b [label="authSet()"];
 *      a<=b [label="round=4, response"];
 * # a rbox a [label="authTest()"];
 *      --- [label="Can be more rounds (never used)"];
 *      a=>b [label="round=5"];
 *      b=>a [label="round=6"];
 *      ...;
 * @endmsc
}
 */
int handshake(connection_type *conn, bit_32 *peer_addr, bit_8 cmd, char status, int * size, char * buff)
{
//    path_type *path;
    char auth_len = 0;
    char cmdbuf[SIZE_DGRAM], rbuf[SIZE_DGRAM];
    int i, ret, sh, sock, blen;
    unsigned int csize, socksize = sizeof(struct sockaddr_in6);
    struct sockaddr_in6 saddr, caddr;
    bit_32 size_buff;

    if(!conn) return 0;

//    path = &(conn->mpath[0]);

    memset(cmdbuf, 0, 40+auth_len);
    cmdbuf[0]   = cmd;
    cmdbuf[1]   = status;
    cmdbuf[2]   = conn->auth_type; // The authentication type
    if (cmd == CMD_CONNECTION_CREATE) cmdbuf[2] = AUTH_SHA256;
    auth_len = authSize(cmdbuf[2]);
    cmdbuf[3]   = sh = auth_len;   // The length of authentication data

    cmdbuf[sh+4]   = 1;            // round No.
    cmdbuf[sh+5]   = conn->ip_version;

    memcpy(&cmdbuf[sh+ 8], conn->ip_local, SIZE_IN6ADDR);
    memcpy(&cmdbuf[sh+24], conn->ip_remote,SIZE_IN6ADDR);

    // send size of data
    size_buff = *size;
    memcpy(&cmdbuf[sh+40], &size_buff, 4);

    blen = auth_len+44;

    sock = tun.cmd_socket_rcv; // cmd  socket
    saddr.sin6_family = AF_INET6;
    memcpy(&saddr.sin6_addr,  peer_addr, SIZE_IN6ADDR);
    saddr.sin6_port = htons(conn->cmd_port_remote);
    csize = socksize;

    authSet(conn, cmdbuf, blen);

    i = 0; ret = -1;
    while ((i<=5) && (ret<0)) {
        if (i) usleep(2000);
        ret = sendto(sock, cmdbuf, blen, 0, (struct sockaddr *)&saddr, socksize);
    DEBUG("handshake send round1 ret:%d errno%d\n", ret, errno);
        ret = getcmd(sock, rbuf, blen, 0, (struct sockaddr *)&caddr, &csize, 500 );
        i++;
    }

    if (ret < 0) {
        reterror("error: No answer from peer (Round 2).", 1);
    }

    if (rbuf[sh+4] != 2 ) {
        reterror("Round number error in round 2.", 9);
    }
    if (!authTest(conn, rbuf, ret)) {
        reterror("Authentication check failed.", 3);
    }


    cmdbuf[sh+4] = rbuf[sh+4] + 1;
    memcpy(&cmdbuf[sh+40], buff, *size);
    blen = *size+sh+40;

    authSet(conn, cmdbuf, blen);

    i = 0; ret = -1;
    //while ((i<2) && (ret>0)) { // the last send will be repeated, if the peer still answers
    while ((i<=5) && (ret<0)) {
        if (i) usleep(2000);
        ret = sendto(sock, cmdbuf, blen, 0, (struct sockaddr *)&saddr, socksize);
    DEBUG("handshake send round3 ret:%d errno%d\n", ret, errno);
        ret = getcmd(sock, rbuf, blen, 0, (struct sockaddr *)&caddr, &csize, 500);
        i++;
    }
    if (ret < 0) {
        reterror("error: No answer from peer (Round 4).", 1);
    }

    if (rbuf[sh+4] != 4 ) {
        reterror("Round number error in round 4.", 9);
    }
    if (!authTest(conn, rbuf, ret)) {
        reterror("Authentication check failed.", 3);
    }


    if(ret < *size) *size = ret;          // size = max(ret,size)
    memcpy(buff, &rbuf[sh+40], *size);   // return with received data
  DEBUG("handshake return with %d\n", rbuf[1]);
    return rbuf[1];
}

/**
 * Initiate the change of the path's status
 *
 * @param       conn    The connection to use
 * @param       pind    Index of path
 * @param       lstatus The new status to use
 *
 * @return On successful operation returns with 0, otherwise with error code.
 */
int path_change_status(connection_type *conn, int pind, bit_8 lstatus)
{
    path_type *path;
    int gind, size;
    char  lipstr[128], ripstr[128];
    char c, buff[32];
    bit_8 oldstatus, rstatus;

    if(!conn) return 0;

    path = &(conn->mpath[0]);
    oldstatus = path[pind].status;

    if (oldstatus == lstatus) return 0;

    rstatus = lstatus & STAT_PATH_DOWN;
    if (lstatus & STAT_PATH_DOWN) path[pind].status = lstatus;
    gind=0;
    while ((gind<MAX_PATH) && (path[gind].status != STAT_OK))  gind++;

    if (gind>= MAX_PATH) {
        if (lstatus == STAT_OK) gind = pind;
        else {
            sprintf(lipstr, "All path down for connection %s ", conn->name);
            buff[1] = 6;
            reterror(lipstr, 6);
        }
    }
    path[pind].status = oldstatus;

    memcpy(&buff[0], path[pind].ip_public, SIZE_IN6ADDR);
    memcpy(&buff[16], path[pind].ip_remote, SIZE_IN6ADDR);
    size = 2*SIZE_IN6ADDR;

    if (path[pind].ip_version == 6) {
            inet_ntop(AF_INET6, &buff[16], ripstr, 128);
            inet_ntop(AF_INET6, &buff[0], lipstr, 128);
    } else {
            inet_ntop(AF_INET, &buff[28], ripstr, 128);
            inet_ntop(AF_INET, &buff[12], lipstr, 128);
    }

/* I think we don't need condition
    if(handshake(conn, gind, CMD_P_STATUS_CHANGE, rstatus, &size, buff) == 0) {
        fprintf(stderr, "Path status changed to 0x%02X (%s -> %s) \n",
            lstatus, lipstr, ripstr);
        path[pind].status = lstatus;
    }
*/
    c = handshake(conn, conn->mpath[gind].ip_remote, CMD_P_STATUS_CHANGE, rstatus, &size, buff);
    if (c) {
        fprintf(stderr, "Path change status handshake returned with error code %d \n", c);
    }
    buff[1] = c;
    fprintf(stderr, "Path status changed to 0x%02X (%s -> %s) \n", lstatus, lipstr, ripstr);
    path[pind].status = lstatus;
    if ( lstatus == STAT_OK ) { // reset the keepalive timer, if came up
    path[pind].last_keepalive = time(NULL);
    }

    return c;
}



// add or delete a route to the peer
void peer_route(char *op, path_type *peer)
{
    char command[255], destination_host[128], gateway[128];
    int af, start;

    strcpy(destination_host, "");
    strcpy(gateway, "");
    if (peer->ip_version == 4) {
    af=AF_INET;
    start = 3;
    } else {
    af = AF_INET6;
    start = 0;
    }
    inet_ntop(af, &peer->ip_remote[start], destination_host, 128);
    inet_ntop(af, &peer->ip_gw[start], gateway, 128);
    sprintf(command, "bin/mpt_peer_routes.sh %s %d %s %s %s", op, peer->ip_version, destination_host, gateway, peer->interface);
    system(command);

}


// add routes to the peers and networks
void add_routes(connection_type* conn)
{
  char command[255];
  path_type *peer;
  char destination_network[255], gateway[255];
  network_type *network;
  int af, start;
  int j;

// ********** Routes for the PEERS *************
  for (j=0; j< conn->path_count; ++j) {
    peer = &conn->mpath[j];
DEBUG("add_routes peer_route %d\n", j);
    peer_route("add", peer);
  }

DEBUG("add route to the remote tunnel ip");
    path_type tunnelpeer;
    tunnelpeer.ip_version = conn->ip_version;
    memcpy(&tunnelpeer.ip_remote, &conn->ip_remote, SIZE_IN6ADDR);
    memcpy(&tunnelpeer.ip_gw, &conn->ip_local, SIZE_IN6ADDR);
    sprintf((char *)&tunnelpeer.interface, "%s", (char *)&tun.interface);
    peer_route("add", &tunnelpeer);

DEBUG("add_routes after peer_route\n");
// Routes for the NETWORKS **********************
  for(j=0; j < conn->network_count; ++j){

    network = &conn->networks[j];
    strcpy(destination_network, "");
    strcpy(gateway, "");

    if (network->version == 6) {
    af = AF_INET6;
    start = 0;
    }
    else {
    af = AF_INET;
    start = 3;
    }

DEBUG("add_routes network_route %d\n", j);
    inet_ntop(af, &network->destination[start], destination_network, 128);
    inet_ntop(af, &conn->ip_remote[start], gateway, 128);
    sprintf(command, "bin/mpt_routes.sh add %d %s %d %s", network->version, destination_network, network->destination_prefix_length, gateway);
    system(command);
  }
}


// delete routes to peers and networks
void del_routes(connection_type* conn)
{
  char command[255];
  path_type *peer;
  network_type *network;
  char destination_network[255], gateway[255];
  int af, start;
  int j;

// ********** Routes for the PEERS *************
  for (j= conn->path_count-1; j>=0;  --j) {
    peer = &conn->mpath[j];
    peer_route("del", peer);
  }

// DEBUG("del route to the remote tunnel ip")
    path_type tunnelpeer;
    tunnelpeer.ip_version = conn->ip_version;
    memcpy(&tunnelpeer.ip_remote, &conn->ip_remote, SIZE_IN6ADDR);
    memcpy(&tunnelpeer.ip_gw, &conn->ip_local, SIZE_IN6ADDR);
    sprintf((char *)&tunnelpeer.interface, "%s", (char *)&tun.interface);
    peer_route("del", &tunnelpeer);

// Routes for the NETWORKS **********************

  for(j=0; j < conn->network_count; ++j){

    network = &conn->networks[j];
    strcpy(destination_network, "");
    strcpy(gateway, "");

    if (network->version == 6) {
    af = AF_INET6;
    start = 0;
    }
    else {
    af = AF_INET;
    start = 3;
    }

    inet_ntop(af, &network->destination[start], destination_network, 128);
    inet_ntop(af, &conn->ip_remote[start], gateway, 128);
    sprintf(command, "bin/mpt_routes.sh del %d %s %d %s", network->version, destination_network, network->destination_prefix_length, gateway);
    system(command);
  }
}

/**
 * Send existing local connection to its remote peer to create and modify
 * the local one with the received paramters
 *
 * @param       conn    The connection which should be send to remote peer
 * @return Returns 0 on successful communication, 2 if the remote pair already exists and 1 otherwise
 */
int conn_create(connection_type *conn) {
    if (!conn)
        return 1;

    int size, gind, h;
    bit_32 *peer_addr;
    connection_type *newconn;
    char lipstr[128];
    path_type *path;
    char buff[SIZE_DGRAM];        // This is the receive buffer size too and filled per connections, but was 1528 at testing
    newconn = malloc(sizeof (connection_type));
    memset(newconn, 0, sizeof(connection_type));
    size = connection_write_memory(buff, conn, SIZE_DGRAM);

    path = &(conn->mpath[0]);
    gind=0; peer_addr = NULL;

    if (conn->default_cmd_path) {
        while ( (gind < conn->path_count) && (conn->default_cmd_path != path+gind) ) gind++;
        if ( (path[gind].status == STAT_OK) || (path[gind].status == STAT_PATH_DOWN) ) peer_addr = path[gind].ip_remote;
    }
    if (!peer_addr) {
        while ((gind<conn->path_count) && (path[gind].status != STAT_OK) && (path[gind].status != STAT_PATH_DOWN) )  gind++;
        if (gind<conn->path_count) peer_addr = path[gind].ip_remote;
    }

    if (!peer_addr) {
        sprintf(lipstr, "No path is available for creating connection to remote peer for connection %s ", conn->name);
        reterror(lipstr, 6);
    }

    size = strlen(buff);
    if (!buff[0]) {
        printf("conn_create error: unsuccessful save to memory file\n");
        return(4);
    }

    h = handshake(conn, peer_addr, CMD_CONNECTION_CREATE, 0, &size, buff);
    if (h) {
        printf("Connection create unsuccessful, return code: %d\n", h);
        if (h==5) printf("Connection already exists on the server.\n");
        return(h);
    }
    char *pbuff = buff;
 DEBUG("\nconnection_create after handshake. cmdbuf:\n%s\nStarting connection_read_memory\n", pbuff);
    connection_read_memory(newconn, pbuff, size);
    if (!memcmp(newconn->ip_remote, conn->ip_local, sizeof(conn->ip_local))) {
 DEBUG("mirroring connection\n");
         strcpy(lipstr, conn->filename);
         newconn->network_count = 0; // protect network overriding with conn_merge
         int netcount = conn->network_count;
         conn->network_count = 0;
         conn_merge(conn, conn, newconn);
         strcpy(conn->filename, lipstr);
         conn->network_count = netcount;
    }

        /*
         Connection stop & start to restart the keepalive_thread, otherwise there
         are problems when client has no remote address for a path on mptsrv start-up.
        */
    connection_stop(conn);
    connection_start(conn);
    conn_print_item(stdout, conn);
    return 0;
}
