/**
 * @file
 * @brief Thread handle functions
 * @author Almasi, Bela; Debrecen, Hungary
 * @date 2012.08.15.
 * @copyright Project maintained by Almasi, Bela; Debrecen, Hungary
 */

#define MPT_DEBUG

#include <sys/time.h>
#include "multipath.h"
#include "mp_local.h"

#include "cli.h"
#include "auth.h"

/**
 * The thread to read data from the tunnel interface
 */
void * tunnel_read_thread (void * arg)
{
    bit_8   ver, grelen;
    char    grebuff[2064];
    char *buff;
    int blen = 0, start, sh, sock, pind, c;
    tunnel_type *tun;
    connection_type *con;
    path_type *path;
    struct sockaddr *saddr;

    tun = (tunnel_type *)arg;
// fprintf(stderr, "Tunnel read thread starts \n");

    while(1) {
// GRE in UDP needs maximum 16 bytes for the GRE header
        buff = grebuff + 16;
        blen = read(tun->fd, buff, 2048);
//DEBUG("Read %d bytes from tunnel \n", blen);
        if (blen < 8) goto NEXT_READ ;
        ver = (buff[0] & 0xF0) >> 4;
        if (ver == 6) {
            start = 8;
            sh = SIZE_IN6ADDR;
        }
        else {
            start = 12;
            sh = SIZE_INADDR;
        }
        mpt_lock;
        con = conn_search_ip(ver, (bit_32 *)&buff[start], (bit_32 *)&buff[start+sh],
               mp_global_conn);
        if (con) {
//DEBUG("  Connection found\n");
            pind = con->path_index;
            path = con->pathselectionlist[pind];
            c=0;
            while ((path->status != STAT_OK) && (c<2)) {
                pind++;
                if (pind >= con->pathselectionlength) { pind = pind % con->pathselectionlength; c++; }
                path = con->pathselectionlist[pind];
//DEBUG("Search for STAT_OK path c=%d  pind=%d\n", c, pind);
            }
            if (c >=2) {
                printf("Packet can not be sent out from tunnel interface - no path has STAT_OK\n");
                usleep(10000);
                goto NEXT_READ ;
            }
//DEBUG("Check interface\n");
// Here pind stores the index of pathselectionlist to send the packet out; path is a pointer to the path out
            if (check_interface( path->interface, path->socket )) { // int. up
//                if (p->mpath[pind].status == STAT_IF_DOWN) {
//                     path_change_status(p, pind, STAT_OK);
//    fprintf(stderr, "Interface %s could be changed state to UP.\n", p->mpath[pind].interface);
//                }
            }
            else { // interface down
                if (path->status == STAT_OK) {
                    path_change_status(con, pind, STAT_IF_DOWN);
                    fprintf(stderr, "Interface %s changed state to DOWN.\n", con->mpath[pind].interface);
                }
            }

//DEBUG("  Beginning to fill GRE header\n");
            sock = path->socket;
            saddr = (struct sockaddr *)&(path->peer);
            grelen = con->gre_length;
            bit_32 greseq;
            bit_32 *greseq_ptr;
            greseq = con->conn_packet;
	        con->conn_packet++;
            pind = (pind + 1) % con->pathselectionlength;
            con->path_index = pind;

//printf("Send seq: %d  grelen: %d\n", greseq, grelen);
            if (con->reorder_window ) {
		        greseq_ptr = (bit_32 *)con->gre_header;
                greseq_ptr[1] = htonl(greseq);
            }
            memcpy(grebuff+(16-grelen), con->gre_header, grelen);
         } // if (con)
        if (con){
            sendto(sock, grebuff+(16-grelen), blen+grelen, 0, saddr, sizeof(struct sockaddr_in6));
        }
NEXT_READ:
        mpt_unlock;
    } // while 1

    return NULL;
}


/**
 * The thread to read data from the connection's socket
 * Each path has got its own socket_read_thread - 2015.08.20
 */
void * socket_read_thread(void * arg)
{
    char  *buff, ipstr[128], ripstr[128], lipstr[128];
    struct sockaddr_in6 client;
   // int sh;
    int cindex, blen = 0;
    bit_8  grelen;
    bit_32 *greseq;
    grebuff_type *pcircle, *pgre;
    unsigned int client_size = sizeof client;

    path_type *path;
    connection_type *con;
    path = (path_type *)arg;
  //fprintf(stderr, "Starting Data receive thread. \n");
    con = path->connection;
    struct timeval tmptv;
 //   memset(&rcv_timeout, 0, sizeof(struct timeval));
//    rcv_timeout.tv_sec = 2; //set timeout for 2 seconds
//    rcv_timeout.tv_usec = 100;

  //  setsockopt(path->socket, SOL_SOCKET,SO_RCVTIMEO,(char*)&rcv_timeout, sizeof(struct timeval));
 //   grelen = conn->gre_length; // it should consider the checksum, key and seq. flags to calculate the length, but we do not use them now

 //   if (con->reorder_window) setsockopt(path->socket, SOL_SOCKET,SO_RCVTIMEO,(char*)&con->max_buffdelay, sizeof(struct timeval));
    while (1) {
        if (path->grebuffindex >= con->reorder_window) path->grebuffindex -= con->reorder_window;
        pgre = &path->grebuffarray[path->grebuffindex];

        blen = recvfrom(path->socket, pgre->buffer, sizeof(pgre->buffer), 0, (struct sockaddr *)&client, &client_size);
        gettimeofday(&pgre->timestamp, NULL);
        if ( (blen < 0) && (con->reorder_window) && (con->circlepackets) ) {
                    pthread_mutex_lock(&mp_buff_lockvar);

                    timeradd(&con->last_receive, &con->max_buffdelay, &tmptv);
                    if ( (con->circlepackets) && (timercmp(&tmptv, &pgre->timestamp, < )) ) {
                          DEBUG("Receive timer exceeded ");
                          send_circulate_packets(con, 1); // The maximum buffering time has elapsed
                    }
                    pthread_mutex_unlock(&mp_buff_lockvar);
                    continue;
        }
        if (blen < 8) continue;
        if(memcmp(&client.sin6_addr, path->ip_remote, SIZE_IN6ADDR) != 0) {
                      inet_ntop(AF_INET6, &client.sin6_addr, ipstr, 100);
                      inet_ntop(AF_INET6, path->ip_remote, lipstr, 100);
                      printf("Got UDP %d bytes from bad client. IP6: %s (Required: %s)  Errno: %d Socket: %d\n", blen, ipstr, lipstr, errno, path->socket);
                      continue; // We do not consider the packet, if the sender is not the peer
        }
        grelen = 4;
        greseq = NULL;
        if ( pgre->buffer[0] & 0x10 ) {
              grelen += 4;
              greseq = (bit_32 *)(&pgre->buffer[4]) ;
        }
        buff = pgre->buffer + grelen;
        if (greseq) pgre->seq = ntohl(greseq[0]); else pgre->seq = 0;
        pgre->grebufflen = blen;
        pgre->grelen = grelen;
        blen -= grelen;
// if ( (grelen == 8) && (pgre->seq != con->seq_start) )
//      printf("Rcv Data,  Int: %s  GREseq:%d grebufflen:%d  grelen:%d   Waited seq:%d Type:%02X \n", path->interface, pgre->seq, pgre->grebufflen, pgre->grelen, con->seq_start, buff[0]);

        if ((unsigned char)buff[0] < 0xA0 ){
           pthread_mutex_lock(&mp_buff_lockvar);

           memcpy(&con->last_receive, &pgre->timestamp, sizeof(struct timeval));
           if ((!con->reorder_window) || (!greseq)) {
              write(tun.fd, &pgre->buffer[grelen], blen);
           }
           else {
              if ((pgre->seq < con->seq_start) && (pgre->seq > 50)) { // lost packet arrived too late
                  DEBUG("Dropped packet arrived too late. Seq: %d,  Start seq (waited for): %d \n", pgre->seq, con->seq_start);
                  goto BUFF_UNLOCK_LABEL;
              }

              // if the received seq is too big OR the peer was restarted (seq<10), we accept the seq
              if ( (pgre->seq >= con->seq_start + con->reorder_window) || ((pgre->seq < con->seq_start ) && (pgre->seq <= 50)) ){ // this could be done witk substraction and unsigned comparison, so true even if pgre->seq < con->seq
                      if (pgre->seq >= con->seq_start + con->reorder_window)
                            printf("Too BIG GRE sequence numbering problem:\n");
                      else
                            printf("Too SMALL GRE sequence numbering problem (Peer restarted?)\n");

                      printf("    Circle index: %d,  Start seq(waited for): %d,  Arrived seq: %d \n", con->circlestart, con->seq_start, pgre->seq);
                      printf("    Sending out packets from the buffer; accepting the new GRE seq from the peer \n");
                      send_circulate_packets(con, 1);
                      con->seq_start = pgre->seq;
              }

              cindex = con->circlestart + pgre->seq - con->seq_start;
              if (cindex >= con->circle_window) cindex -= con->circle_window;

              if ((con->circlebuff[cindex] ) && (con->circlebuff[cindex]->grebufflen) ) {
                      printf("Circulate reorder array overloaded with %d packets. Loosing packet (seq: %d ) \n", con->circlepackets + 1, pgre->seq);
                      send_circulate_packets(con, 1); //full buffer clean
                      goto BUFF_UNLOCK_LABEL ; //continue;
              }
              con->circlebuff[cindex] = pgre;
              con->circlepackets++;
// printf("Packet with greseq %d was placed into circlebuffer index:%d \n", pgre->seq, cindex);

              pcircle = con->circlebuff[con->circlestart]; //  The packet we are waiting for arrived, send it out
//  if (pcircle) printf("cindex: %d, pcircle-grebufflen: %d, circlepackets: %d \n", cindex, pcircle->grebufflen, con->circlepackets );
              if (con->circlepackets >= con->circle_window) {
                       printf("Warning! Reorder window filled up: %d packets arrived. Sending out buffered packets. \n", con->circlepackets);
              }

              if ( (con->circlepackets) && (pcircle) && (pcircle->grebufflen - grelen > 0))
                      send_circulate_packets(con, 0); // not full send, only up to the first empty pos.

              path->grebuffindex++;
              if (path->grebuffindex >= con->reorder_window) path->grebuffindex -= con->reorder_window;
              if (path->grebuffarray[path->grebuffindex].grebufflen) {
                      printf("Path GRE buffer overloaded! Packet lost! ");
                      printf("Interface: %s Index: %d ; Clearing buffer.\n", path->interface, path->grebuffindex);
                      send_circulate_packets(con,1);  // full send
                      goto BUFF_UNLOCK_LABEL ; //continue;
              }
           }
BUFF_UNLOCK_LABEL:
           pthread_mutex_unlock(&mp_buff_lockvar);
          // usleep(2000); // give a chance to other paths to receive
        } // if buff[0] < A0 ******************************** CMD Arrived ***********************************************
        else { pgre->grebufflen = 0;   // The CMD packet is in buff[] with length blen; we sign the buffer place as empty
              if ((unsigned char)buff[0] == CMD_KEEPALIVE) {
         //     sh = authSize(buff[2]); // should be used for authentication
//              cp = conn_search_ip(6, (bit_32 *)&(buff[sh+24]), (bit_32 *)&(buff[sh+8]), mp_global_conn);
//            cp = mp_global_conn;
//    printf("Keepalive CMD on DATA received, connection found \n");
                  if(memcmp(&client.sin6_addr, path->ip_remote, SIZE_IN6ADDR) == 0) {  //check if the keepalive came from the peer
                        path->last_keepalive = time(NULL);
                        if(path->status == STAT_PATH_DOWN) {
                            printf("Keepalive received\n");
                            if (path->ip_version == 6) {
                                  inet_ntop(AF_INET6, path->ip_remote, ripstr, 128);
                                  inet_ntop(AF_INET6, path->ip_public, lipstr, 128);
                            } else {
                                  inet_ntop(AF_INET, &path->ip_remote[3], ripstr, 128);
                                  inet_ntop(AF_INET, &path->ip_public[3], lipstr, 128);
                            }
                            path->status = STAT_OK;
                            path->last_keepalive = time(NULL);
                            fprintf(stderr, "Path status changed to 0x%02X (%s -> %s) \n", STAT_OK, lipstr, ripstr);
                        }
                  }
                continue;
              } // IF CMD_KEEPALIVE
              else {
                inet_ntop(AF_INET6, &client.sin6_addr, ipstr, 128);
                fprintf(stderr, "Got command from a data socket. Peer address: %s\n",ipstr);
              }
        }
    } // while(1)
    return NULL;
}




/**
 * The thread to handle the circulate buffer
 * If a packet is buffered more than a given time, the circulate buffer wil be cleared up to this packet
 */
void * circulatebuffer_handler(void * arg)
{
    int cind, findex, np, lostp;
    int sleeptime = 1; // sleep time between sending in milliseconds

    bit_16 tmplen;
    grebuff_type *grebuf, *tmpgre;

    connection_type *con;
    con = (connection_type *)arg;
  //DEBUG("Starting Circulate buffer handling receive thread. \n");
    struct timeval ctime, tmptv;
    memset(&ctime, 0, sizeof(struct timeval));

    sleeptime = 1000*sleeptime; // sleeptime in microseconds
    while (1) {
        if (con->reorder_window) {
              pthread_mutex_lock(&mp_buff_lockvar);

              if (con->circlepackets) {
                   gettimeofday(&ctime, NULL);
                   cind = con->circlestart -1;
                   if (cind<0) cind += con->circle_window;
                   findex = 0;
                   while (cind != con->circlestart) {
                      grebuf = con->circlebuff[cind];
                      if (grebuf) {
                           timeradd(&grebuf->timestamp, &con->max_buffdelay, &tmptv);
                           if (timercmp(&tmptv, &ctime, < ) ) {
                               findex = cind +1;
                               break; // found a packet which is too old - findex shows tha (place +1) in circlebuff
                           }
                      }
                      if (!cind) cind = con->circle_window;
                      cind--;
                   } // cind != con->circlestart

                   if (findex) { // do the work. have to send out all the packets before (findex -1)
                          findex--;
DEBUG("circulatebuffer_handler: Too old packet at index %d (GRE Seq: %d)\n", findex, con->circlebuff[findex]->seq);
                          np = con->circlepackets;
                          tmpgre = con->circlebuff[findex];
                          tmplen = tmpgre->grebufflen;
                          tmpgre->grebufflen = 0; // turn off temporary the packet in findex position
                          cind = con->circlestart;
                          lostp = tmpgre->seq - con->seq_start;
                  //  DEBUG("lostp: %d\n", lostp);
                          while (cind != findex) {
                                grebuf = con->circlebuff[cind];
                                if ((grebuf) && (grebuf->grebufflen)) {
                                     con->circlestart = cind;
                                     send_circulate_packets(con, 0);
                                }
                                cind++;
                                if (cind >= con->circle_window) cind -= con->circle_window;
                          }
                          tmpgre->grebufflen = tmplen; // the findex packet is active again
                          con->circlestart = findex;
                          lostp -= (np - con->circlepackets);
                          send_circulate_packets(con, 0);
DEBUG("circulatebuffer_handler:       sent out %d packets; lost %d packets.\n", np-con->circlepackets, lostp);
                   }  // if findex
              }  // ic con->circle_packets
              pthread_mutex_unlock(&mp_buff_lockvar);
        } // if con->reorder_window
        usleep(sleeptime);
    } // while 1

    return NULL;
}




/**
 * The thread to read commands from the cmd socket
 */
void * cmd_read_thread(void *arg)
{
    char buff[SIZE_DGRAM], lbuff[SIZE_DGRAM], ripstr[128], gwipstr[128], cmdbuf[200];
    char *dbuff = buff+4;
    struct sockaddr_in6 client;
    unsigned int csize = sizeof client;
    int sock, i,  blen = 0, timeout_msec;
    bit_8 sh, cmd;
    char c, ret = 0;
    struct timeval timer, tv, ival;

    connection_type *cp;
    //path_type *path;

    sock = tun.cmd_socket_rcv;

    // set socket timeout
    timeout_msec = 500; // 0.5s

    // set keepalive timer
    memset(&ival, 0, sizeof(ival));
    ival.tv_sec = CHECK_KEEPALIVE;
    gettimeofday(&tv, NULL);
    timeradd(&tv, &ival, &timer);

    while (1) {
            ret = 0;
            blen = getcmd(sock, buff, sizeof(buff), 0, (struct sockaddr *)&client, &csize, timeout_msec);

            // check keepalives if timer expired
            gettimeofday(&tv, NULL);
            if(timercmp(&timer, &tv, <)) {
                cp = mp_global_conn;
                while(cp) {
                    for(i=0; i<cp->path_count; i++) {
                        if(cp->mpath[i].last_keepalive == 0) cp->mpath[i].last_keepalive = time(NULL); // allow some converge time after startup
                        else if(cp->mpath[i].deadtimer > 0 && cp->mpath[i].last_keepalive + cp->mpath[i].deadtimer < time(NULL) &&
                           cp->mpath[i].status == STAT_OK) {
                            printf("no keepalive received\n");
                            path_change_status(cp, i, STAT_PATH_DOWN);
                        }
                    }
                    cp = cp->next;
                }
                timeradd(&tv, &ival, &timer);
            }

            // no data received
            if (blen < 0) {
                continue;
            }

            buff[blen] = 0;
            set_ipstr(ripstr, (bit_32 *) &client.sin6_addr, 6);
  printf("Cmd from peer %s, length:%d\n", ripstr, blen);
            if ( check_loopback((char *)&client.sin6_addr)) {
               printf("   Local cmd: %s\n", dbuff);
                //if (buff[blen] != 0x00) buff[blen] = 0x00;
                //ret =  do_local_cmd(buff);
                ret = exec_cmd(dbuff, sock, (struct sockaddr *) &client, csize);
                memset(lbuff, 0, sizeof(lbuff));
                lbuff[0]=CMD_LOCAL_REPLY;
                if (ret) sprintf(lbuff+4, "CMD_OK:%s", dbuff);
                else sprintf(lbuff+4, "CMD-ERROR:%s", dbuff);
                sendto(sock, lbuff, strlen(lbuff+4)+4, 0, (struct sockaddr *)&client, csize);
                lbuff[0] = CMD_LOCAL_REPLY_END;
                usleep(5000);
                sendto(sock, lbuff, 4, 0, (struct sockaddr *)&client, csize);   // mark end of data stream
                continue;
            }

// as getcmd checks the command, here we can omit this checking
/********            if ((unsigned char)buff[0] < 0xA0 ) {
                inet_ntop(AF_INET6, &client.sin6_addr, ripstr, 128);
                fprintf(stderr, "Got data from the cmd socket. Peer address: %s\n",ripstr);
                continue;
            }
**********/
//        else
//        {
//      inet_ntop(AF_INET6, &client.sin6_addr, ripstr, 128);
//          fprintf(stderr, "Got CMD: %02X  Peer address: %s\n",buff[0], ripstr);
//        }

            cmd = buff[0];
            if (cmd == CMD_CONNECTION_CREATE) buff[2] = AUTH_SHA256;
            sh = authSize(buff[2]);
            cp = conn_search_ip(6, (bit_32 *)&(buff[sh+24]), (bit_32 *)&(buff[sh+8]), mp_global_conn);
            if (buff[sh+4]>=4) continue; // do not work in the 4th round (or later)
//      cp = mp_global_conn;  // for testing only

            // save received keepalive packet
            if (cmd == CMD_KEEPALIVE) {
                if(cp) {
                    for(i=0; i<cp->path_count; i++) {
                        if(memcmp(&client.sin6_addr, cp->mpath[i].ip_remote, SIZE_IN6ADDR) == 0) {
                            cp->mpath[i].last_keepalive = time(NULL);
                            if(cp->mpath[i].status == STAT_PATH_DOWN) {
                                printf("keepalive received\n");
                                path_change_status(cp, i, STAT_OK);
                            }
                        }
                    }
                }
                continue;
            }

 //  char lipstr[128]; inet_ntop(AF_INET6, cp->ip_remote, lipstr, 128);
 //  char ripstr[128]; inet_ntop(AF_INET6, &(buff[sh+8]), ripstr, 128);
 //  printf("c1 end %s %s\n", ripstr, lipstr);

            if (!mp_global_server && !cp ) {
                fprintf(stderr, "CMD arrived - no global server enabled and no connection exist. \n");
                continue; // peer is not allowed to send cmd
            }
            if (!cp) {
                if ((buff[sh+4] != 1) && (cmd != CMD_CONNECTION_CREATE)) {
                    fprintf(stderr, "Must have connection after round 1\n");
                    continue;
                }

                // create connection
/******                if (cmd == CMD_CONNECTION_CREATE) // we do not add new connection here after the first round
                {
                    cp = conn_new();
                    cp->ip_version = buff[sh+5];
                    memcpy(cp->ip_local,  &buff[sh+24], SIZE_IN6ADDR);
                    memcpy(cp->ip_remote, &buff[sh+ 8], SIZE_IN6ADDR);
        DEBUG("New connection is set from data received ver %d \n", cp->ip_version);
                } else
**********/
                if (cmd != CMD_CONNECTION_CREATE){
                    fprintf(stderr, "ERROR: Got cmd (%X) (not conn. create) and connection not found. \n", cmd);
                    continue;
                }
            } else { // cp found the connection
                if (cp->waitround == 0 && buff[sh+4]>1) {
                        fprintf(stderr, "Cannot receive round %d, send round 1 first.\n", buff[sh+4]);
                        continue; // Session not initiated
                }
                if(cp->waitround > 0 && buff[sh+4] > cp->waitround) {
                        fprintf(stderr, "Cannot receive round %d more than expected (%d)\n", buff[sh+4], cp->waitround);
                        continue; // Not reached this state
                }
            }

            if ( !authTest(cp, buff, blen) ) {
                fprintf(stderr, "Packet authentication data is bad\n");
                continue;
            }

            if ((cmd == CMD_CONNECTION_CREATE) && (!mp_global_server)) {
                    fprintf(stderr, "Error: Got connection create command (%X) with global server disabled.\n", cmd);
                    continue;
            }

            // don't need do_command() if round=1
            c = 0;
            if(buff[sh+4] != 1 ) {
                c = do_command(cp, buff, &blen);
                // Send reply
            }
            buff[1] = c;
       // DEBUG("CMD thread: sending reply after round %d do_command returned %d\n", buff[sh+4], c);
            buff[sh+4] += 1; i = 0; ret = -1;
            if (cp) {
                    cp->waitround = buff[sh+4] + 1;
            }
            authSet(cp, buff, blen);

            path_type *path = template_conn.default_cmd_path;
            int need_route = (cmd == CMD_CONNECTION_CREATE) && path ;//(!search_path_remote_ip(client.sin6_addr)) ;
            if (need_route && !path_search_remote_ip((bit_32 *)&client.sin6_addr)) { // have to set route to the new peer
                 set_ipstr(ripstr, (bit_32 *)&client.sin6_addr, path->ip_version);
                 set_ipstr(gwipstr, path->ip_gw, path->ip_version);
                 sprintf(cmdbuf, "bin/mpt_peer_routes.sh %s %d %s %s %s", "add", path->ip_version, ripstr, gwipstr, path->interface);
                 system(cmdbuf);
            }

            while ((i<2) && (ret<0)) {
                    i++;
                    ret = sendto(sock, buff, blen, 0, (struct sockaddr *)&client, csize);
                    if(ret<0) fprintf(stderr, "Sending response failed with result %d (errno %d)\n", ret, errno);
                    usleep(5000);
            }
            if (need_route && !path_search_remote_ip((bit_32 *)&client.sin6_addr)) { // have to remove route to the new peer
                 set_ipstr(ripstr, (bit_32 *)&client.sin6_addr, path->ip_version);
                 set_ipstr(gwipstr, path->ip_gw, path->ip_version);
                 sprintf(cmdbuf, "bin/mpt_peer_routes.sh %s %d %s %s %s", "del", path->ip_version, ripstr, gwipstr, path->interface);
                 system(cmdbuf);
            }

    } // while 1
    return NULL;
}

/**
 * The thread to send keepalive messages
 * Each path has its own keepalive_send_thread - 2015.08.31.
 */
void * keepalive_send_thread(void * arg)
{
    path_type * path;
    connection_type *conn;
    int sock, blen, grelen, slen;
    unsigned int sleeptime, socksize = sizeof(struct sockaddr_in6);
    struct sockaddr_in6 saddr;
    char grecmdbuf[56], ipstr[128];
    char *cmdbuf;

    path = (path_type *) arg;
    conn = (connection_type *)path->connection;
    // construct keepalive packet
    memset(grecmdbuf, 0, sizeof(grecmdbuf));
    grelen = conn->gre_length;
    sleeptime = path->keepalive;
    if (!sleeptime) return NULL;

    grelen = 4;
    cmdbuf = grecmdbuf+grelen;
    cmdbuf[0] = CMD_KEEPALIVE;
    cmdbuf[5] = conn->ip_version;
    memcpy(&cmdbuf[8], conn->ip_local, SIZE_IN6ADDR);
    memcpy(&cmdbuf[24], conn->ip_remote, SIZE_IN6ADDR);
    memcpy(grecmdbuf, conn->gre_header, grelen);
    grecmdbuf[0] = 0x00;
    blen = 40+grelen;
    memcpy(&saddr, &path->peer, socksize);
    sock = path->socket;
    set_ipstr(ipstr, path->ip_remote, path->ip_version);

    while(1) {
                if ( (path->status == STAT_PATH_DOWN) || (path->status == STAT_OK) ) {
                        slen = sendto(sock, grecmdbuf, blen, 0, (struct sockaddr *)&path->peer, socksize);
                        if (slen<0) fprintf(stderr,"Keepalive send failed for connection %s\n", conn->name);
                       // DEBUG("Sent keepalive for conn %s; to peer: %s (slen: %d, errno: %d)\n", conn->name, ipstr, slen, errno);
                }
                sleep(sleeptime);
    }
}
