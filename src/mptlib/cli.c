/**
 * @file
 * @author      Kelemen Tamas <kiskele@krc.hu>
 * @brief This file is used to check CLI commands and their parameters.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <arpa/inet.h>

#include "cli.h"

#ifdef MPT_SERVER
#include "multipath.h"
#include "mp_local.h"
#endif

/**
 * Constructor that allocates memory and initializes the structure of "interface up|down" command
 *
 * @note This memory needs to deallocate after use. Use mpt_int_destructor()
 *
 * @param this   Pointer to an "mpt_int *" variable
 */
void mpt_int_constructor(struct mpt_int ** this) {
    if((*this = (struct mpt_int *)malloc(sizeof(struct mpt_int))) == NULL) return;
    (*this)->interface = NULL;
    (*this)->mark = NULL;
}

/**
 * Destructor that deallocates memory after struct mpt_int * is no longer needed
 *
 * @param this   Pointer to an "mpt_int *" variable
 */
void mpt_int_destructor(struct mpt_int ** this) {
    if(*this == NULL) return;

    if((*this)->interface) free((*this)->interface);
    if((*this)->mark) free((*this)->mark);
    free(*this);
}

/**
 * Constructor that allocates memory and initializes the structure of "address add|del" command
 *
 * @note This memory needs to deallocate after use. Use mpt_addr_destructor()
 *
 * @param this   Pointer to an "mpt_addr *" variable
 */
void mpt_addr_constructor(struct mpt_addr ** this) {
    if((*this = (struct mpt_addr *)malloc(sizeof(struct mpt_addr))) == NULL) return;
    (*this)->ip = NULL;
    (*this)->op = NULL;
    (*this)->mask = NULL;
    (*this)->dev = NULL;
}

/**
 * Destructor that deallocates memory after struct mpt_addr * is no longer needed
 *
 * @param this   Pointer to an "mpt_addr *" variable
 */
void mpt_addr_destructor(struct mpt_addr ** this) {
    if(*this == NULL) return;

    if((*this)->ip) free((*this)->ip);
    if((*this)->op) free((*this)->op);
    if((*this)->mask) free((*this)->mask);
    if((*this)->dev) free((*this)->dev);
    free(*this);
}

/**
 * Constructor that allocates memory and initializes the structure of "path up|down" command
 *
 * @note This memory needs to deallocate after use. Use mpt_change_path_destructor()
 *
 * @param this   Pointer to an "mpt_change_path *" variable
 */
void mpt_change_path_constructor(struct mpt_change_path ** this) {
    if((*this = (struct mpt_change_path *)malloc(sizeof(struct mpt_change_path))) == NULL) return;
    (*this)->op = NULL;
    memset((*this)->local_ip, 0, sizeof((*this)->local_ip));
    memset((*this)->remote_ip, 0, sizeof((*this)->remote_ip));
}

/**
 * Destructor that deallocates memory after struct mpt_change_path * is no longer needed
 *
 * @param this   Pointer to an "mpt_change_path *" variable
 */
void mpt_change_path_destructor(struct mpt_change_path ** this) {
    if(*this == NULL) return;

    if((*this)->op) free((*this)->op);
    free(*this);
}

/**
 * Function that creates string from regular expression match
 *
 * @note This function allocates memory for dst string that needs to deallocate after use, but it is done in the destructor functions.
 *
 * @param dst    Pointer to the newly allocated destination string
 * @param src    Original string which contains the matching string
 * @param pmatch Which match need to be saved. N-th element of array return by regexec \n
 *               This contains index of the beginning and the end of the match
 */
void save_match(char ** dst, char * src, regmatch_t pmatch) {
    if(pmatch.rm_so >= 0) {
        *dst = (char*)malloc(pmatch.rm_eo - pmatch.rm_so + 1);
        strncpy(*dst, src+pmatch.rm_so, pmatch.rm_eo - pmatch.rm_so);
        *(*dst + pmatch.rm_eo - pmatch.rm_so) = 0;
    } else {
        *dst = NULL;
    }
}

/**
 * Function to parse CLI input and return command type and structure of parameters.
 *
 * @param cmd      Input string which need to parse
 * @param rettype  Return the matched command. See @ref CMDTYPE.
 * @param ret      Return a struct for that command that contains the parsed arguments
 *
 * @return         PARSE_OK if the command and parameters seem ok, otherwise the code of parameter error. See @ref ReturnParser for return values
 */
int parser(char * cmd, CMDTYPE * rettype, void ** ret) {
    int errcode, rv = PARSE_ERR_NOCMD, i, j;
    regex_t preg;
    char * regexps[] = {
/// <b> CLI commands: </b>
#define CLI_COMMAND(name, regex) regex,
#include "../include/cmd.def"
#undef CLI_COMMAND
        NULL    // end of array
    };

    for(i=0; regexps[i]!=NULL; i++) {
        int domain;
        unsigned char buf[sizeof(struct in6_addr)];
        int pmatchcnt = 1;
        regmatch_t * pmatch = NULL;
        if((errcode=regcomp(&preg, regexps[i], REG_EXTENDED))) {
            char errstr[1024];
            regerror(errcode, &preg, errstr, sizeof(errstr));
            fprintf(stderr, "%s: Invalid regular expression \"%s\": %s\n", __FUNCTION__, regexps[i], errstr);
            return PARSE_ERR_REGEX;
        }

        for(j=0; j<strlen(regexps[i]); j++) {
            if(regexps[i][j] == '(') pmatchcnt++;
        }
        if(pmatchcnt > 0) pmatch = (regmatch_t *) malloc(pmatchcnt*sizeof(regmatch_t));

        if(!regexec(&preg, cmd, pmatchcnt, pmatch, 0)) {
            rv = PARSE_OK;
            *rettype = i;
            *ret = NULL;
            switch(i) {
                case MPT_INT:
                    if(pmatch[3].rm_so >= 0) {
                        struct mpt_int * ret_int = NULL;

                        mpt_int_constructor(&ret_int);
                        if(ret_int == NULL) break;
                        *ret = ret_int;

                        save_match(&(ret_int->interface), cmd, pmatch[3]);
                        save_match(&(ret_int->mark), cmd, pmatch[4]);
                    } else rv = PARSE_ERR_PARAMS;
                    break;
                case MPT_ADDR:
                    if(pmatch[3].rm_so >= 0) {
                        struct mpt_addr * ret_addr = NULL;

                        mpt_addr_constructor(&ret_addr);
                        if(ret_addr == NULL) break;
                        *ret = ret_addr;

                        save_match(&(ret_addr->op), cmd, pmatch[3]);
                        save_match(&(ret_addr->ip), cmd, pmatch[4]);
                        save_match(&(ret_addr->mask), cmd, pmatch[10]);
                        save_match(&(ret_addr->dev), cmd, pmatch[11]);

                        // test ip address
                        if(pmatch[5].rm_so >= 0) domain = AF_INET;
                        else domain = AF_INET6;
                        if(inet_pton(domain, ret_addr->ip, buf) <= 0) {
                            rv = PARSE_ERR_IP;
                            break;
                        }
                        inet_ntop(domain, buf, ret_addr->ip, strlen(ret_addr->ip));

                        // test mask value
                        if(ret_addr->mask) {
                            if(domain == AF_INET) {
                                if(atoi(ret_addr->mask)>32) {
                                    rv = PARSE_ERR_MASK;
                                }
                            } else {
                                if(atoi(ret_addr->mask)>128) {
                                    rv = PARSE_ERR_MASK;
                                }
                            }
                        }
                    } else rv = PARSE_ERR_PARAMS;
                    break;
                case MPT_RELOAD:
                case MPT_RESTART:
                case MPT_SAVE:
                    // commands with 1 optional parameter
                    if(pmatch[2].rm_so >= 0) {
                        save_match((char**)ret, cmd, pmatch[2]);
                    } else if(pmatch[1].rm_eo - pmatch[1].rm_so > 0) rv = PARSE_ERR_PARAMS;
                    break;
                case MPT_CREATE:
                case MPT_DELETE:
                    // commands with 1 mandatory parameter
                    if(pmatch[2].rm_so >= 0) {
                        save_match((char**)ret, cmd, pmatch[2]);
                    } else rv = PARSE_ERR_PARAMS;
                    break;
                case MPT_PATH:
                    {
                        char * temp;
                        int start;
                        struct mpt_change_path * ret_path = NULL;
                        mpt_change_path_constructor(&ret_path);

                        if(ret_path == NULL) break;

                        *ret = ret_path;

                        if(pmatch[2].rm_so < 0) {
                            rv = PARSE_ERR_PARAMS;
                            break;
                        }

                        save_match(&(ret_path->op), cmd, pmatch[2]);

                        save_match(&temp, cmd, pmatch[3]);
                        if(pmatch[4].rm_so >= 0) {
                            domain = AF_INET;
                            ret_path->local_ip[2] = 0xFFFF0000;
                            start = 3;
                        } else {
                            domain = AF_INET6;
                            start = 0;
                        }
                        if(inet_pton(domain, temp, &(ret_path->local_ip[start])) <= 0) {
                            rv = PARSE_ERR_IP;
                            free(temp);
                            break;
                        }
                        free(temp);

                        save_match(&temp, cmd, pmatch[8]);
                        if(pmatch[9].rm_so >= 0) {
                            domain = AF_INET;
                            ret_path->remote_ip[2] = 0xFFFF0000;
                            start = 3;
                        } else {
                            domain = AF_INET6;
                            start = 0;
                        }
                        if(inet_pton(domain, temp, &(ret_path->remote_ip[start])) <= 0) {
                            rv = PARSE_ERR_IP;
                            free(temp);
                            break;
                        }
                        free(temp);
                    }
                    break;
                default:
                    break;
            }

            if(pmatch) free(pmatch);
            regfree(&preg);
            return rv;
        }
        if(pmatch) free(pmatch);
        regfree(&preg);
    }

    // invalid command
    *rettype = MPT_NOCMD;
    return rv;
}

/**
 * Check CLI command syntax on client side to send out only valid commands
 *
 * @param cmd   CLI command input
 * @return      See @ref ReturnParser
 */
int parse_cmd(char * cmd) {
    int rv;
    CMDTYPE rettype = MPT_NOCMD;
    void * ret = NULL;

    rv=parser(cmd, &rettype, &ret);

    switch(rv) {
        case PARSE_OK:
        case PARSE_ERR_REGEX:
            break;
        case PARSE_ERR_IP:
            printf("ERROR: Invalid IP specified\n");
            break;
        case PARSE_ERR_MASK:
            printf("ERROR: Invalid netmask specified\n");
            break;
        case PARSE_ERR_PARAMS:
            printf("ERROR: Wrong parameters specified\n");
            if(rettype == MPT_INT) printf(HELP_INT);
            else if(rettype == MPT_ADDR) printf(HELP_ADDR);
            else if(rettype == MPT_RELOAD) printf(HELP_RELOAD);
            else if(rettype == MPT_RESTART) printf(HELP_RESTART);
            else if(rettype == MPT_DELETE) printf(HELP_DELETE);
            else if(rettype == MPT_SAVE) printf(HELP_SAVE);
            else if(rettype == MPT_PATH) printf(HELP_PATH);
            else if(rettype == MPT_CREATE) printf(HELP_CREATE);
            else printf(HELP);
            break;
        default:
            printf("ERROR: Illegal command\n");
            printf(HELP);
    }

    switch(rettype) {
        case MPT_INT:
            mpt_int_destructor((struct mpt_int **)&ret);
            break;
        case MPT_ADDR:
            mpt_addr_destructor((struct mpt_addr **)&ret);
            break;
        case MPT_PATH:
            mpt_change_path_destructor((struct mpt_change_path **)&ret);
            break;
        case MPT_RELOAD:
        case MPT_RESTART:
        case MPT_DELETE:
        case MPT_SAVE:
        case MPT_CREATE:
            free(ret);
        default:
            break;
    }

    return rv;
}



#ifdef MPT_SERVER
/**
 * Check CLI command syntax on server side and execute that
 * @todo Check result of commands and if something went wrong return an error code and put some explanation in cmd parameter.
 *
 * @param cmd CLI command received
 * @return Execution of command failed or succeeded
 */
int exec_cmd(char * cmd, int sock, struct sockaddr * client, unsigned int csize) {
    int i, rv = PARSE_ERR_NOCMD;
    CMDTYPE rettype = MPT_NOCMD;
    void * ret = NULL;
    char cmdstr[256];

    rv=parser(cmd, &rettype, &ret);
    switch(rettype) {
        case MPT_INT:
            // exec
            if(rv == PARSE_OK) {
                struct mpt_int * ret_int = ret;
                printf("mpt int %s %s\n", ret_int->interface, ret_int->mark);
                if(strcmp(ret_int->mark, "down") == 0) {
                    interface_change_status(ret_int->interface, STAT_IF_DOWN);
                    sprintf(cmdstr, "bin/mpt_int_updown.sh %s %s", ret_int->interface, ret_int->mark);
	            system(cmdstr);
                } else {
                    sprintf(cmdstr, "bin/mpt_int_updown.sh %s %s", ret_int->interface, ret_int->mark);
	            system(cmdstr);
		    connection_type *conn = mp_global_conn;
		    while(conn) {
    			for (i=0; i< conn->path_count; i++) {
        		    if (strcmp(ret_int->interface, conn->mpath[i].interface) == 0 ) {
				peer_route("add", &conn->mpath[i]);
        		    }
    			} // for
    			conn = conn->next;
		    } // while

                    interface_change_status(ret_int->interface, STAT_OK);
                }
            }
            mpt_int_destructor((struct mpt_int **)&ret);
            break;
        case MPT_ADDR:
            //exec
            if(rv == PARSE_OK) {
                struct mpt_addr * ret_addr = ret;

                sprintf(cmdstr, "bin/mpt_addr_adddel.sh %s %s", ret_addr->op, ret_addr->ip);
                if(ret_addr->mask) sprintf(cmdstr+strlen(cmdstr), "/%s", ret_addr->mask);
                else sprintf(cmdstr+strlen(cmdstr), "/24");
                sprintf(cmdstr+strlen(cmdstr), " %s", ret_addr->dev);

                system(cmdstr);
            }
            mpt_addr_destructor((struct mpt_addr **)&ret);
            break;
        case MPT_RELOAD:
            {
                if(ret) {
                    connection_type *conp;
                   DEBUG("\nReloading: %s \n", (char *)ret);
                    conp = conn_search_filename((char *)ret);
                    if (conp) connection_reload(conp);
                    else {
                        sprintf(cmd, "Reload unsuccessful. Config file not found in the connection: %s\n", (char *)ret);
                        printf("%s", cmd);
                        free(ret);
                        return(0);
                    }
                    free(ret);
                } else {
                    connection_reload(NULL);
                }
            }
            break;
        case MPT_RESTART:
            {
                if(ret) {
                    connection_type *conp;
                   // printf("\nResetting: %s \n", (char *)ret);
                    conp = conn_search_filename((char *)ret);
                    if (conp) connection_restart(conp);
                    else {
                        sprintf(cmd, "Restart unsuccessful. Config file not found in the connection: %s\n", (char *)ret);
                        printf("%s", cmd);
                        free(ret);
                        return(0);
                    }
                    free(ret);
                } else {
                    connection_restart(NULL);
                }
            }
            break;
        case MPT_CREATE:
            {
                int create_ret = 0;
                connection_type *local_conn;
                local_conn = conn_search_filename((char *)ret);
                if (local_conn) {
                    create_ret = conn_create(local_conn);
                    if (create_ret) {
                        if(create_ret == 5)
                            sprintf(cmd, "ERROR: Connection already exists (error code:%d)\n", create_ret);
                        else
                            sprintf(cmd, "Connection creation failed (code: %d; Remote server problem?)\n", create_ret);
                        printf("%s\n", cmd);
                        return(0);
                    }
                }
                else {
                  printf("ERROR: Unable to find local connection with a given filename %s\n", (char *)ret);
                  return(0);
                }

            }
            break;
        case MPT_DELETE:
            {
                connection_type * p;
                p = conn_search_filename((char *)ret);

                if (p) {
                        connection_type * del = p;
                        char * newfile;

                        if(p == mp_global_conn) {
                            p = mp_global_conn = p->next;
                            if(p) p->prev = NULL;
                        } else {
                            p = p->prev->next = p->next;
                        }

                        del->next = NULL;
                        connection_stop(del);

                        // rename config file
                        if((newfile = (char*)malloc(strlen(del->filename)+strlen(".disabled")+1))) {
                            sprintf(newfile, "%s.disabled", del->filename);
                            chdir("conf/connections");
                            rename(del->filename, newfile);
                            chdir("../..");
                            free(newfile);
                        }

                        free(del);
                    } else {
                        sprintf(cmd, "Delete unsuccessful. Config file not found: %s\n", (char *)ret);
                        printf("%s", cmd);
                        return(0);
                    }

                free(ret);
            }
            break;
        case MPT_SAVE:
            {
                if (ret) {
                    if (conn_search_filename((char *)ret)) {
                            conn_save_filename( (char *)ret );
                    }
                    else {
                        sprintf(cmd, "Save unsuccessful. Config file not found in connection: %s\n", (char *)ret);
                        printf("%s", cmd);
                        return(0);
                    }
                }
                else {
                    conn_save_filename(NULL);
                }
            }
            break;
        case MPT_PATH:
            if(rv == PARSE_OK) {
                struct mpt_change_path * ret_path = ret;
                connection_type * conn = mp_global_conn;
                int i, status;

                if(strcmp(ret_path->op, "up") == 0) status = STAT_OK;
                else  if(strcmp(ret_path->op, "down") == 0) status = STAT_PATH_DOWN_MANUAL;
                      else {
                            printf("Path change status requested with bad operation: #%s# \n", ret_path->op);
                            mpt_change_path_destructor((struct mpt_change_path **)&ret);
                            break;
                   }

                while(conn) {
                    for(i=0; i<conn->path_count; i++) {
                        if((memcmp(conn->mpath[i].ip_public, ret_path->local_ip, 16) == 0) &&
                           (memcmp(conn->mpath[i].ip_remote, ret_path->remote_ip, 16) == 0)) {
                            path_change_status(conn, i, status);
                        }
                    }
                    conn = conn->next;
                }
            }
            mpt_change_path_destructor((struct mpt_change_path **)&ret);
            break;
        case MPT_STATUS:
            {
                char buff[SIZE_DGRAM];        // This is the receive buffer size too and filled per connections, but was 1528 at testing

                FILE * fd;

                fd = fmemopen(buff, SIZE_DGRAM, "wb");
                memset(buff, 0, SIZE_DGRAM);
                tunnel_print(fd);
                connection_print(fd);
                fflush(fd);
                strcpy(cmd, buff); // The result is given back in cmd
//                if(buff[0] != 0) sendto(sock, buff, strlen(buff)+4, 0, client, csize);

                fclose(fd);
            }
        default:
            break;
    }

    return (rv == PARSE_OK);
}
#endif


/**
 * Receive from the peer with given timeout value. The parameters are the same as recvfrom()
 *     timeout_msec: the receive timeout in milliseconds.
 * @return Returns with the number of received bytes or with -1 on errors.
 */
int getcmd(int sock, char *buff, int blen, int flags,
           struct sockaddr *saddr, unsigned int *ssize, long timeout_msec)
{
    int ret;

    struct timeval tv;

    memset(&tv, 0, sizeof(tv));
    if (!timeout_msec) timeout_msec = 100; //do not use 0 receive timeout
    tv.tv_sec = timeout_msec / 1000;
    tv.tv_usec = (timeout_msec % 1000)*1000; // convert to microseconds
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));

    ret = 0;
    ret=recvfrom(sock, buff, blen, flags, saddr, ssize);

    if (ret <= 0) return(-1);

    if ((unsigned char)buff[0] < 0xA0) {
//           Here we can accept resent GRE in UDP data
            fprintf(stderr, "Getcmd: received data on cmd socket.\n");
            ret = -1;
    }
    return(ret);
}
