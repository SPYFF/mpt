/**
 * @file
 * @brief       Basic operations on a connection
 * @author      Almasi, Bela; Debrecen, Hungary
 * @date        2012.08.15.
 * @copyright   Project maintained by Almasi, Bela; Debrecen, Hungary
 */

//#define MPT_DEBUG

#include "multipath.h"
#include "mp_local.h"

#include "send_circle.h"

// Calculate the pathselection vector according to the weight_out values
void calculate_pathselection(connection_type *con) {
  long calc_lko(long x, long y) {
    if (x == 0) {
      return y;
    }

    while (y != 0) {
      if (x > y) {
        x = x - y;
      }
      else {
        y = y - x;
      }
    }
    return x;
  }

    long long lkt;
    long lko, minincrement, cinc;
    int i,j, minind;
    path_type *p;

    con->pathselectionlength = 0;
    lko = con->mpath[0].weight_out;
    lkt = lko;
    for (i=0; i<PATH_SELECTION_LENGTH; i++) con->pathselectionlist[i] = NULL;
    for (i=0; i<con->path_count; i++) {
         lko = calc_lko(lko, con->mpath[i].weight_out );
         lkt = (lkt * con->mpath[i].weight_out ) / lko;
         con->mpath[i].selection_increment = 0;
    }
printf("PathSelection indices: ");
    for (j=0; j<PATH_SELECTION_LENGTH; j++) {
         minind = 0;
         minincrement = lkt+1;
         for (i=0; i < con->path_count; i++) {
              p = &con->mpath[i];
              cinc = p->selection_increment + (lkt / p->weight_out);
              if ((p->weight_out) && ( cinc < minincrement)) {
                   minind = i;
                   minincrement = cinc;
              }
         }
         con->pathselectionlist[j] = &con->mpath[minind];
         con->mpath[minind].selection_increment = minincrement;
printf("%d, ", minind);
         for (i=0; i<con->path_count; i++) // check if we are ready
                if ( con->mpath[i].selection_increment != minincrement) goto NEXT_SELECTION;
         break; // all the path has got increment == lkt, we have finished
NEXT_SELECTION:
         continue;
    }
    con->path_index = 0;
    con->pathselectionlength = j+1;
printf("\nPathselection list length: %d, Final increment:%d\n", con->pathselectionlength, (int)minincrement);
}



/**
* Search connection with a specified name.
*
* @param conn The head of the connections list
* @param conn_name Name of the searched connection
* @return Pointer to the connection or NULL if not found
*/
path_type *path_search_remote_ip(bit_32 *r_ip)
{
    int i;
    if (r_ip == NULL)
        return NULL;

    connection_type *act = mp_global_conn;
    while(act)
    {
        for(i=0; i<act->path_count; i++)
            if (memcmp(r_ip, act->mpath[i].ip_remote, SIZE_IN6ADDR) == 0) {
                return &act->mpath[i];
            }
        act = act->next;
    }
    return NULL;
}




/**
* Search connection with a specified name.
*
* @param conn The head of the connections list
* @param conn_name Name of the searched connection
* @return Pointer to the connection or NULL if not found
*/
connection_type *conn_search_name(char *conn_name)
{
    if (conn_name == NULL)
        return NULL;

    connection_type *act = mp_global_conn;
    while(act)
    {
        if (strcmp(act->name, conn_name) == 0)
        {
            return act;
        }
        act = act->next;
    }
    return NULL;
}

// The same as conn_search_name but for the filename of the connection
connection_type *conn_search_filename(char *filename)
{
    if (filename == NULL)
        return NULL;

    connection_type *act = mp_global_conn;
    while(act) {
        if (strcmp(act->filename, filename) == 0)
        {
            return act;
        }
        act = act->next;
    }
    return NULL;
}


/**
* Search connection with empty status
*
* @param conn The head of the connections list, NULL means mp_global_conn
* @return Pointer to an empty connection or NULL if not found
*/
connection_type *conn_empty_search(connection_type *conn)
{
    connection_type *act;
    if (!conn)
        act = mp_global_conn;
    else
        act = conn;

    while(act) {
        if (act->status == CONN_STAT_EMPTY)
            return act;
        act = act->next;
    }
    return NULL;
}

/**
 * Start all or the specified connection. Open the connection's socket and start the connection threads
 * @param single_conn - If NULL, then mp_global_conn is used to start all connections, otherwise only the specified connection will be started
 */
void connection_start(connection_type *single_conn)
{
    connection_type *con;
    path_type *path;
    char ipstr[128];
    int i, reorder_window_1;

    setglobalkey();
    if (single_conn) con = single_conn;
    else con = mp_global_conn;
    if (!con) {
         printf("connection_start called with no connection available.\n");
         return;
    }

    while (con) {
//   conn_print_item(stdout, con);
	// p->reorder_window = CIRCLE_SIZE;
        if ( con->cmd_port_local != tun.cmd_port_rcv ) {
            printf("Local cmd port value is taken from the interface.conf file (%d)\n", tun.cmd_port_rcv);
            con->cmd_port_local = tun.cmd_port_rcv;
        }
        if (con->ip_version == 4) {
            if (con->ip_local[3] != tun.ip4) {
                con->ip_local[3] = tun.ip4;
                set_ipstr(ipstr, con->ip_local, 4);
                printf("Local Tunnel IPv4 address is taken from the interface.conf file (%s)\n", ipstr);
            }
        }
        else {
            if (!memcmp(con->ip_local, tun.ip6, SIZE_IN6ADDR)) {
                memcpy(con->ip_local, tun.ip6, SIZE_IN6ADDR ); // The tunnel address must be the same as in the interface file
                set_ipstr(ipstr, con->ip_local, 6);
                printf("Local Tunnel IPv6 address is taken from the interface.conf file (%s)\n", ipstr);
            }
        }
        add_routes(con);
DEBUG("connection_start: add_routes ended\n");
        con->circlestart = 0;
        con->circlepackets = 0;
        con->seq_start = 0;
        // GRE in UDP encapsulation default GRE header settings
        memset(con->gre_header, 0, 16);
        if (con->ip_version == 6) {
            con->gre_header[2]=0x86;
            con->gre_header[3]=0xDD;
        }
        else {
            con->gre_header[2]=0x08;
            con->gre_header[3]=0x00;
        }
        if (con->reorder_window == 0) con->gre_length = 4;
        else { con->gre_length = 8; con->gre_header[0] |= 0x10; }
        reorder_window_1 = con->reorder_window + 1;
        con->circle_window = 2*reorder_window_1;
DEBUG("connection_start - pathselection start\n");
        calculate_pathselection(con);
DEBUG("connection_start - pathselection ended\n");
	    for (i=0; i < con->path_count; i++ ) {
                path = &con->mpath[i];
                if (path->cmd_default) con->default_cmd_path = path;
                path->connection = con;
		        path->grebuffarray = malloc(reorder_window_1 * sizeof(grebuff_type) );
                path_open_socket(path);
                memset(&path->peer, 0, sizeof(path->peer));
                path->peer.sin6_family = AF_INET6;
                path->peer.sin6_port = htons(con->port_remote);
                memcpy(&path->peer.sin6_addr, path->ip_remote, SIZE_IN6ADDR);
                pthread_create(&path->socket_read, NULL, socket_read_thread, path );
                if (path->keepalive) pthread_create(&path->keepalive_send, NULL, keepalive_send_thread, path);
        }
        for (i=0; i< con->circle_window ; i++ ) con->circlebuff[i] = NULL;
        if (con->reorder_window) pthread_create(&con->circlebuffer_handler, NULL, circulatebuffer_handler, con );
        con = con->next;
        if (single_conn) break;
    } // while con
}


/**
 * Stop all or the specified connection. Stop the connection's socket and cancel the connection threads
 * @param single_conn - If NULL, then mp_global_conn is used to stop all connections, otherwise only the specified connection will be stopped
 */
void connection_stop(connection_type *single_conn)
{
    connection_type *p; int i;
    if (single_conn)p = single_conn;
    else p = mp_global_conn;
    while (p) {
  // conn_print(p);
//        pthread_cancel(p->socket_read);
        for (i=0; i < p->path_count; i++ ) {
                if (p->mpath[i].keepalive) pthread_cancel(p->mpath[i].keepalive_send);
                pthread_cancel(p->mpath[i].socket_read);
                close(p->mpath[i].socket);
        }
        if (p->reorder_window) pthread_cancel(p->circlebuffer_handler);
        del_routes(p);
        p = p->next;
        if (single_conn) break;
    }

}

void connection_delete(connection_type *del)
{
    connection_type *conn, *prev, *conn_next, *delconn;
    if (del) conn = del;
    else conn = mp_global_conn;
    int r = 0;
    while(conn) {
        r++;
DEBUG("connection_delete active - round %d \n", r);
        conn_next = conn->next;
        prev = NULL;
        delconn = mp_global_conn;
        while((delconn) && (delconn != conn ))
        {
            prev = delconn;
            delconn = delconn->next;
        }
        if (delconn)
        {
DEBUG("connection_delete found connection to delete\n");
            if (prev) prev->next = delconn->next;
            if (delconn == mp_global_conn)  mp_global_conn = mp_global_conn->next;
            free(delconn);
        }
        else printf("connection_delete problem: Did not found the collection to delete\n");
        if (del) break;
        conn = conn_next;
    } // while conn
DEBUG("connection_delete finished\n");
}

/**
 * Restart (stop and then start) all or the specified connection.
 * @param single_conn - If NULL, then mp_global_conn is used to restart all connections, otherwise only the specified connection will be restarted
 */
void connection_restart(connection_type *single_conn)
{
    connection_type *conn;
    int i;
    conn = mp_global_conn;
    if (single_conn) conn = single_conn;
    while (conn)
    {
DEBUG("connection_stop started...\n");
       connection_stop(conn);
       conn->status = STAT_OK;
       for  (i=0; i<conn->path_count; i++ )
            conn->mpath[i].status = STAT_OK;
DEBUG("starting connection_start...\n");
       connection_start(conn);
       if (single_conn) break;
       conn = conn->next;
    }
}

/**
 * Reload (stop, reload configuration and then start) all or the specified connection.
 * @param single_conn - If NULL, then mp_global_conn is used to reload all connections, otherwise only the specified connection will be reloaded
 */
void connection_reload(connection_type *single_conn)
{
    char filename[255];
    connection_type *conn;
    strcpy(filename, "");
DEBUG("starting connection_stop\n");
    connection_stop(single_conn);
DEBUG("starting connection_delete\n");
    if (single_conn) strcpy(filename, single_conn->filename);
    connection_delete(single_conn);
    if ((!single_conn) && (mp_global_conn)) {
        printf("connection_reload: unsuccessful delete.\n");
        mp_global_conn = NULL;
    }
    usleep(1000); // wait 1 ms before restarting
DEBUG("starting connection_load %s \n", filename);
    if (single_conn) {
         conn_load(filename, NULL);
         //connection_print();
         conn = conn_search_filename(filename);
    }
    else {
         connection_load_dir(NULL);
         conn = NULL;
    }
DEBUG("Starting connections - reload\n");
    connection_start(conn);
}



/**
 * Open the socket of the specified connecton (after 'Load')
 * @param conn The connection which needed to be
 */
int path_open_socket(path_type *path)
{
    int sock6;
    char msg[255], ipstr[128];
    connection_type *con;
    unsigned int socklen;
    struct sockaddr_in6 sockaddr6;
//    char ipstr[128];
    int off = 0;
    int on = 1;

    socklen = sizeof(sockaddr6); // **************** IPv6 socket ************
    con = (connection_type *)path->connection;
    sockaddr6.sin6_port   = htons(con->port_local);
    sockaddr6.sin6_family = AF_INET6;
    memcpy(&(sockaddr6.sin6_addr), &path->ip_private, SIZE_IN6ADDR);
    if ((sock6 = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
            exerror("Socket creation error.", errno);

    setsockopt(sock6, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&off, sizeof off);
    // IPV6_only is turned off, so this socket will be used for IPv4 too.
    setsockopt(sock6, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof on);

    if (bind(sock6, (struct sockaddr *) &sockaddr6, socklen) <0) {
            close(sock6);
            set_ipstr(ipstr, path->ip_private, path->ip_version);
            sprintf(msg, "Socket bind error - cannot bind local data %s:%d  interface:%s", ipstr, con->port_local, path->interface );
            exerror(msg, errno);
    }
    if (con->port_local == 0) { // This is the first path: store the port num.
            getsockname(sock6, (struct sockaddr *) &sockaddr6, &socklen);
            con->port_local = ntohs(sockaddr6.sin6_port);
    }
    path->socket = sock6;
//  printf("Path socket created %d \n", path->socket);
    return(0);
}


/**
 * Search the connection specified by version, local ip and remote ip keys
 * @param version       The IP Version number (4 or 6)
 * @param local         The local IP address
 * @param remote        The remote IP address
 * @param conn          The head of the connections list
 * @return Pointer to the connection or NULL if not found
 */
connection_type *conn_search_ip (bit_8 version, bit_32 *local, bit_32 *remote,
    connection_type *conn)
{
/* ************* NEW PART START */

  DEBUG("version: %d", version);
  int (*comp)(connection_type *conn, bit_32 *local, bit_32 *remote) = (version == 4) ? comp_ipv4 : comp_ipv6;
  connection_type *act = conn;
  int found;
  for(found = 0; act != NULL; act = act->next){
    found = comp(act, local, remote);
    if(found){
      break;
    }
  }
  //DEBUG("found: %d | %d", found, act);
  return act;
/* ************* NEW PART END */

    connection_type *p;
    p = conn;
    if (version == 4) {
        while (p != NULL) {
            if ((p->ip_version == 4)  && (p->ip_local[3] == *local)
             && (p->ip_remote[3] == *remote))
                return(p);
 //printf("%0lX - %0lX \n", p->ip_local[3], *local);
            p = p->next;
        }
    } // version == 4
    if (version == 6) {
        while (p != NULL) {
            if ( (!memcmp(p->ip_local, local, SIZE_IN6ADDR))
               && (!memcmp(p->ip_remote, remote, SIZE_IN6ADDR)))
                    return(p);
            p = p->next;
        }
    }

    return(NULL);

}

int comp_ipv6(connection_type *conn, bit_32 *local, bit_32 *remote)
{


  int found = !memcmp(conn->ip_local, local, SIZE_IN6ADDR);

  DEBUG("conn->ip_local:%X:%X:%X:%X, local:%X:%X:%X:%X", conn->ip_local[0], conn->ip_local[1], conn->ip_local[2], conn->ip_local[3], local[0], local[1], local[2], local[3]);

  bit_32 mask[4], network[4];
  int i,prefix_length;
  for(i=0; !found && i < conn->network_count; i++){
	prefix_length = conn->networks[i].source_prefix_length;
	memcpy(mask, ipv6_masks[prefix_length], 16);
    set_ipv6_network(conn->networks[i].source, mask, network);

    DEBUG("source:%X:%X:%X:%X, network:%X:%X:%X:%X, mask:%X:%X:%X:%X, local:%X:%X:%X:%X",
	conn->networks[i].source[0], conn->networks[i].source[1], conn->networks[i].source[2], conn->networks[i].source[3],
	network[0], network[1], network[2], network[3],
	mask[0], mask[1], mask[2], mask[3],
        local[0], local[1], local[2], local[3]);

    found = !memcmp(conn->networks[i].source, network, SIZE_IN6ADDR);
  }

  DEBUG("BEFORE DESTINATION SEARCH");
  if(found == 0){
    return 0;
  }
  DEBUG("DESTINATION SEARCH");
//  found = conn->ip_remote[3] == *remote;
  found = !memcmp(conn->ip_remote, remote, SIZE_IN6ADDR);
  for(i=0; !found && i < conn->network_count; i++){
	prefix_length = conn->networks[i].destination_prefix_length;
	memcpy(mask, ipv6_masks[prefix_length], 16);
    set_ipv6_network(conn->networks[i].destination, mask, network);
    found = !memcmp(conn->networks[i].destination, network, SIZE_IN6ADDR);
  }
  return found;
}

int comp_ipv4(connection_type *conn, bit_32 *local, bit_32 *remote)
{
  if(conn->ip_version != 4){
    return 0;
  }
  int found = conn->ip_local[3] == *local;
  DEBUG("conn->local:%X, local:%X, conn->remote:%X, remote:%X, found: %d",conn->ip_local[3], *local, conn->ip_remote[3], *remote, found);

//  const bit_32 base = 0xFFFFFFFF;
  bit_32 mask, network;
  int i,prefix_length;
  for(i=0; !found && i < conn->network_count; i++){
	prefix_length = conn->networks[i].source_prefix_length;
	mask = ipv4_masks[prefix_length];
    network = (*local) & mask;
    DEBUG("conn->networks[i].source:%X:%X:%X:%X, mask: %X network:%X\n",conn->networks[i].source[0], conn->networks[i].source[1], conn->networks[i].source[2], conn->networks[i].source[3], mask, network);
    found = conn->networks[i].source[3] == network;
  }

  if(found == 0){
    return 0;
  }

  found = conn->ip_remote[3] == *remote;
  //DEBUG("conn->local:%X, local:%X, conn->remote:%X, remote:%X, found: %d",conn->ip_local[3], *local, conn->ip_remote[3], *remote, found);
  for(i=0; !found && i < conn->network_count; i++){
	prefix_length = conn->networks[i].destination_prefix_length;
	mask = ipv4_masks[prefix_length];
    network = (*remote) & mask;
    //DEBUG("conn->networks[i].destination:%X, network:%X\n",conn->networks[i].source[3], network);
    found = conn->networks[i].destination[3] == network;
  }
  return found;
}

void set_ipv6_network(bit_32* ipv6_address, bit_32* mask, bit_32* network)
{
  network[0] = ipv6_address[0] & mask[0];
  network[1] = ipv6_address[1] & mask[1];
  network[2] = ipv6_address[2] & mask[2];
  network[3] = ipv6_address[3] & mask[3];
}
