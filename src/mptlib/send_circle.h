
/**
 *  The function to send the buffered packets from the circulatebuffer
 *  conn : the connection, which circulate buffer must be cleared
 *  full : if not zero, then all the buffered packets will be sent out (even if empty places are between them)
 */
void send_circulate_packets(connection_type *conn, int full)
{
  bit_32 s_i, s_pktnum, s_sent, s_cindex, s_lastindex;
  grebuff_type *s_pcircle, *s_last;

  s_i = 0; s_sent = 0;
  s_pktnum = conn->circlepackets;
  s_lastindex = 0;
  if ((conn) && (!s_pktnum)) {
      conn->circlestart = 0;
//      conn->seq_start = 0;
  } else {
// printf("send_circulate_packets called, full = %d\n", full);
      s_last = NULL; s_i = 0; s_sent = 0;
      while ((s_sent < s_pktnum) && (s_i < conn->circle_window))
      {
           s_cindex = conn->circlestart + s_i;
           if (s_cindex >= conn->circle_window) s_cindex -= conn->circle_window;
           s_pcircle = conn->circlebuff[s_cindex];
           if ((s_pcircle) && (s_pcircle->grebufflen > 0 )) {
                   write(tun.fd, &s_pcircle->buffer[s_pcircle->grelen], s_pcircle->grebufflen - s_pcircle->grelen );
                   s_last = s_pcircle;
                   s_lastindex = s_cindex;
                   s_sent++;
                   s_pcircle->grebufflen = 0;
                   conn->circlebuff[s_cindex] = NULL;
           }
           else { if (!full) break;; }
           s_i++;
      }
      if (full) {
 printf("send_circle_packets full call sent out  %d packets \n", s_sent);
           if (s_sent < s_pktnum) printf("Circle buffer full clear problem: sent %d packets instead of %d \n", s_sent, s_pktnum);
                 conn->circlepackets = 0;
                 conn->circlestart = 0;
                 conn->seq_start = s_last->seq + 1;
      } else { // not full sending, so we send only up to the first empy buffer
// printf("send_circle_packets NOT full call, sent out %d packets \n", sent);
           if (s_sent) {
                 conn->circlepackets -= s_sent;
                 conn->circlestart = s_lastindex + 1;
                 if (conn->circlestart >= conn->circle_window ) conn->circlestart -= conn->circle_window;
                 conn->seq_start = s_last->seq + 1;
           }
      } // if (full) elses
  } // if conn && !pktnum
  return;
} // ********************************* send_circulate_packets end *******************************************
