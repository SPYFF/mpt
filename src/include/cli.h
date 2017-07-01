/**
 * @file
 * @author Kelemen Tamas <kiskele@krc.hu>
 * @brief Macro definitions used in cli.c
 */

#ifndef CLI_H
#define CLI_H

#include "multipath.h"

#define strstart(src, pattern) strncmp(src, pattern, strlen(pattern))   ///< Src string begins with pattern

#define REGEX_IPV4 "([[:digit:]]{1,3})\\.([[:digit:]]{1,3})\\.([[:digit:]]{1,3})\\.([[:digit:]]{1,3})"
                                                                        ///< Regexp pattern to match a part of IPv4 address
#define REGEX_IPV6 "[0-9a-fA-F:\\.]{1,4}"                               ///< Regexp pattern to match a part of IPv6 address

int parse_cmd(char * cmd);
int exec_cmd(char * cmd, int sock, struct sockaddr * client, unsigned int csize);

/**
 * @defgroup ReturnParser Parser Errors
 * @brief Return values of parser()
 * @{
 */
#define PARSE_OK            0   ///< Successfully parsed and parameters seems good
#define PARSE_ERR_NOCMD     1   ///< No such command
#define PARSE_ERR_REGEX     2   ///< Invalid regexp format specified in c source
#define PARSE_ERR_IP        3   ///< Wrong IP address entered
#define PARSE_ERR_MASK      4   ///< Wrong network mask entered
#define PARSE_ERR_PARAMS    5   ///< Wrong parameter list

/** @} */


/**
 * @brief The parser() function sets in its 2nd parameter that witch command were recognised
 * These values are the corresponding indexes of the successfully matched regular expressions or MPT_NOCMD if nothing was matched.
 */
typedef enum {
#define CLI_COMMAND(name, regex) name,
#include "../include/cmd.def"
#undef CLI_COMMAND
    MPT_NOCMD                   ///< None of the commands above was matched
} CMDTYPE;

/**
 * @defgroup MptHelp Predefined help outputs
 * @{
 */

/**
 * Help page of command mpt address
 */
#define HELP_ADDR "mpt addr[ess] {add | del} IP_ADDRESS[/PREF_LEN] dev INTERFACE\n\
     Adds or deletes the IP_ADDRESS to the INTERFACE.\n\
       IP_ADDRESS: The IP address (can be v4 or v6) to manipulate\n\
       PREF_LEN:   The prefix length of the manipulated address\n\
                   Default prefix length: 24 for IPv4, 64 for IPv6.\n\
       INTERFACE:  The name of the interface related to the manipulated address\n\n"

/**
 * Help page of command mpt interface
 */
#define HELP_INT "mpt int[erface] INTEFACE {up | down}\n\
     Turnes up or down the INTEFACE (Also the paths on the INTERFACE),\n\
       INTEFACE: The name of the interface e.g. eth0\n\n"

/**
 * Help page of command mpt reload
 */
#define HELP_RELOAD "mpt reload [FILENAME]\n\
     Stops and deletes the specified (or all) connection(s).\n\
     Then reloads and starts the given (or all) configuration(s)\n\
       FILENAME: The config file of the connection to reload \n\
                 (Optional. Default: all configs)\n\n"

/**
 * Help page of command mpt restart
 */
#define HELP_RESTART "mpt restart [FILENAME]\n\
     Stops and starts the connection(s), without reloading the config file(s).\n\
     WARM Restart, without changing the connections configuration data.\n\
       FILENAME: The config file of the connection to restart. \n\
                 (Optional. Default: all connections\n\n"

/**
 * Help page of command mpt delete
 */
#define HELP_DELETE "mpt delete FILENAME\n\
     Stops and deletes the specified connection. \n\
       FILENAME: The config file of the connection to delete. \n\n"

/**
 * Help page of command mpt save
 */
#define HELP_SAVE "mpt save [FILENAME]\n\
     Saves the connection(s) configuration. \n\
       FILENAME: The config file from which the configuration was loaded in.\n\
                 (Optional. Defaukt: all connection configs are saved) \n\n"

/**
* Help page of command mpt create
*/
#define HELP_CREATE "mpt create FILENAME\n\
     Communicates the creation of the connection to the peer.\n\
       FILENAME: The name of the config file of the connection to create\n\n"

/**
 * Help page of command mpt path
 */
#define HELP_PATH "mpt path {up | down} SRC_IP DST_IP\n\
     Turnes up or down the specified path (if it is present in a connection)\n\
       up|down:    Change path status to up or down\n\
       SRC_IP:     IP address of the path local endpoint\n\
       DST_IP:     IP address of the path on the remote peer\n\n"

/**
 * Help page of command mpt path
 */
#define HELP_STATUS "mpt show status\n\
     Prints the status of the tunnel and connections\n\n"

/**
 * Detailed help of all usable mpt commands
 */
#define HELP HELP_ADDR  HELP_CREATE HELP_DELETE HELP_INT HELP_PATH HELP_RELOAD HELP_RESTART HELP_SAVE HELP_STATUS

/** @} */

/// Structure to save arguments of command mpt interface up|down
struct mpt_int {
    char * interface;           ///< Interface name
    char * mark;                ///< Mark interface up or down
};

/// Structure to save arguments of command mpt address add|del
struct mpt_addr {
    char * op;                  ///< Operator can be add or del
    char * ip;                  ///< IP Address
    char * mask;                ///< Network mask
    char * dev;                 ///< Interface name
};

/// Structure to save arguments of command mpt path up|down
struct mpt_change_path {
    char * op;                  ///< Operator can be up or down
    bit_32 local_ip[4];         ///< Local IP address
    bit_32 remote_ip[4];        ///< IP address of remote peer
};

#endif
