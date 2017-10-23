/**
 * @file
 * @brief MPT Server global variables, macros and function definitions
 * @author Almasi, Bela; Debrecen, Hungary
 * @copyright Project maintained by Almasi, Bela; Debrecen, Hungary
 * @date 2012.08.15.
*/

#ifndef MP_LOCAL_H
#define MP_LOCAL_H
#include "iniparser.h"

// ****************** thread settings ***************************
pthread_mutex_t  mp_mpt_lockvar;
pthread_mutex_t  mp_buff_lockvar;

#define mpt_start   pthread_mutex_init(&mp_mpt_lockvar , NULL)
#define mpt_lock    pthread_mutex_lock(&mp_mpt_lockvar)
#define mpt_unlock  pthread_mutex_unlock(&mp_mpt_lockvar)
#define mpt_end     pthread_mutex_destroy(&mp_mpt_lockvar)

#define buff_start  pthread_mutex_init(&mp_buff_lockvar , NULL)
#define buff_lock   pthread_mutex_lock(&mp_buff_lockvar)
#define buff_unlock pthread_mutex_unlock(&mp_buff_lockvar)
#define buff_end    pthread_mutex_destroy(&mp_buff_lockvar)

// *************************************************************

// ******************* uncomment the following line to enable debug messages printing with DEBUG command
//#define  MPT_DEBUG

#ifdef MPT_DEBUG
#define DEBUG printf
#else
#define DEBUG(...)
#endif


/// Print error message to stderr and exit with error code
#define exerror(c,z)  {fprintf(stderr, "%s: %s Errno: %d\n", __FUNCTION__, c, z); exit(z);}
/// Print error message to stderr and return with error code
#define reterror(c,z)  {fprintf(stderr, "%s(): %s Errno: %d\n", __FUNCTION__, c, z); return(z);}
/// Read one line text
#define getline(o,z)  fgets(z, 128, o)

#define SIZE_INADDR  sizeof(struct in_addr)
#define SIZE_IN6ADDR sizeof(struct in6_addr)
#define SIZE_DGRAM   65536

#define CHECK_KEEPALIVE 1              ///< Interval for scan pathes reached deadtimer
#define DEFAULT_KEY "A0A1A2A3A4A5A6A7A8A9AAABACADAEAFB0B1B2B3B4B5B6B7B8B9BABBBCBDBEBF"


/**
 * @defgroup StatusValues Status values
 * @brief Status values of connections and pathes
 * @{
 */
#define STAT_OK                 0x00    ///< Status OK
#define STAT_PATH_DOWN          0x80    ///< Path is down
#define STAT_IF_DOWN            0x81    ///< Interface is down
#define STAT_ADDRESS_DOWN       0x82    ///< Address is down
#define STAT_PATH_DOWN_MANUAL   0x83    ///< Path has been manually set to down
#define STAT_DISABLED           0xFF    ///< Disabled

/// @}

/**
 * @defgroup ConnectionStatusValues Connection Status values
 * @brief Status values of connections
 * @{
 */
#define CONN_STAT_OK            STAT_OK ///< Status OK
#define CONN_STAT_DOWN          0x80    ///< Connection is down
#define CONN_STAT_EMPTY         0x81    ///< Connection is allocated but empty

/// @}

/**
 * @defgroup CMDValues CMD values
 * @brief CMD values starts from 0xA0
 * @{
 */
#define CMD_KEEPALIVE           0xA1    ///< Keepalive packet
#define CMD_P_STATUS_CHANGE     0xA2    ///< Path status change request
#define CMD_CONNECTION_CREATE	0xA3	///< Connection create request
#define CMD_LOCAL_REPLY         0xA4    ///< Reply from the server to the client on localhost
#define CMD_LOCAL_REPLY_END     0xA5    ///< The reply has been finished

/// @}

/**
 * @defgroup PermValues Permission Values
 * @brief Permissions used to control of sending connection updates
 * @{
 */
#define PERM_NONE       0       ///< Deny sending and receiving connection updates
#define PERM_SEND       1       ///< Send connection update but don't receive from the remote peer
#define PERM_RECV       2       ///< Allow to receive connection updates but don't send any
#define PERM_BOTH       3       ///< Allow to send and receive connection updates

/// @}

/**
 * @defgroup AuthTypes Authentication types
 * @brief These types can be used to detect the packets of attackers on the CMD communication
 * @{
 */
#define AUTH_NONE       0       ///< Don't use any authentication
#define AUTH_RAND       1       ///< Use random number to detect malicious packets
#define AUTH_SHA256     2       ///< Use random number and crypt the content of the whole packet with the secret key to an SHA hash

/// @}


// *********************************** Global variables *****************
connection_type *mp_global_conn;       ///< The connection list pointer
connection_type  template_conn;        ///< The template connection data (for connection_create)
unsigned char    mp_global_server;     ///< If 0, then only the live connections are allowed to send cmd
tunnel_type      tun;                  ///< Global tunnel interface
char             globalkey[128];       ///< The sha256 authentication kay used globally
ipv4_masks_t     ipv4_masks;		   ///< Masks for ipv4 addresses. Use: ipv4_masks[PREFIX_LENGTH]
ipv6_masks_t	 ipv6_masks;		   ///< Masks for ipv6 addresses. Use: ipv6_masks[PREFIX_LENGTH][32bit slice]
// **********************************************************************

// **************************** Function headers ***************************
char *basename(char *);
char *trim(char *);
void mac_pton(char *, char *);
void set_ipstr(char* , bit_32 * , bit_8 );
void mac_ntop(char *, char *);
FILE *android_fmemopen(unsigned char *, size_t, const char *);

void    conn_print_item(FILE *stream, connection_type *);
int     path_open_socket(path_type *);
connection_type *conn_search_ip(bit_8, bit_32 *, bit_32 *, connection_type *);
connection_type *conn_search_name( char *conn_name);
connection_type *conn_empty_search(connection_type *conn);
path_type *path_search_remote_ip(bit_32 *);
int comp_ipv6(connection_type *conn, bit_32 *local, bit_32 *remote);
int comp_ipv4(connection_type *conn, bit_32 *local, bit_32 *remote);
void set_ipv6_network(bit_32* ipv6_address, bit_32* mask, bit_32* network);
void setglobalkey(void);

void *tunnel_read_thread (void *);
void *socket_read_thread (void *);
void *cmd_read_thread(void *);
void *keepalive_send_thread(void *);
void *circulatebuffer_handler(void *);

int check_loopback (char *);
int check_interface (char *, int);
void interface_change_status(char *, bit_8);
void address_change_status(bit_32 *, bit_8);
int path_change_status(connection_type *, int, bit_8 );
char do_command(connection_type *, char *, int *);
int do_local_cmd(char *);
int cmd_open_socket(tunnel_type *);
connection_type * conn_new(void);
int conn_activate(connection_type *conn, int replicate);
int conn_diff(connection_type *old, connection_type *new);
void conn_mirror(connection_type * dst, connection_type *src);
void conn_merge(connection_type * , connection_type *, connection_type *);
int conn_create(connection_type *);
void send_circulate_packets(connection_type *, int );
void set_ipstr(char *, bit_32 *, bit_8 );
void conn_setup_fromdict(connection_type *, dictionary *);
dictionary * iniparser_load_descriptor(FILE *, const char *);

void send_circulate_packets(connection_type *conn, int full);

void interface_load(char *, tunnel_type *);

int   tunnel_start(tunnel_type *);
void  tunnel_print(FILE *);
int   tunnel_stop(tunnel_type *);

void  connection_start(connection_type *);
void  connection_print(FILE *);
void  connection_stop (connection_type *);
void  connection_restart (connection_type *);
void  connection_reload (connection_type *);
void  connection_load_dir(connection_type *);
void  connection_save(FILE *, connection_type *);
void connection_delete(connection_type *);
int   connection_read_memory(connection_type *, char *, bit_32);
int   connection_write_memory(char *, connection_type *, bit_32);

connection_type *conn_search_filename(char *);
void  conn_save_filename(char *);
void  conn_load(char *, connection_type *);
void  conn_swap(connection_type *);
void  conn_parser(connection_type *);
int   replicate_connection(connection_type *);

int get_globalip4(char *, char *, char *);
int get_privateip4(char *, const char *);
int get_gatewayip4( char *, char* );

void peer_route(char *, path_type *);
int getcmd(int, char *, int, int, struct sockaddr *, unsigned int *, long);

void add_routes(connection_type *);
void del_routes(connection_type *);
void initialize();

#endif
