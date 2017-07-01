/**
 * @file
 * @brief Main code of the client application
 */


#include "cli.h"
#include "multipath.h"
#include "mp_local.h"

#include "iniparser.h"

//#define exerror(c,z)  {fprintf(stderr,"%s Errno: %d\n",c, z); exit(z);}


/**
 * Main function of client application that parses the CLI input and sends that to the server.
 */
int main( int argc, char ** argv )
{
    int i, sock6, st, port, off = 0, rv;
    struct sockaddr_in6 sockaddr;
    unsigned int socklen = sizeof(sockaddr);
    char ccmd[SIZE_DGRAM];
    char *cmd;

    memset(ccmd, 0, SIZE_DGRAM);
    ccmd[0]=CMD_LOCAL_REPLY;
    cmd = ccmd+4;

	long timeout = 5;

   	port = 65010;
	dictionary* conf;
	conf = iniparser_load("conf/interface.conf");
	if(conf == NULL)
	{
		printf("Configuration file is not present, or it is invalid.\nDefault values:\tCMD_port: %d\n\t\tcmd_timeout: %ld\n",port,timeout);
	}
	else
	{
		port = iniparser_getint(conf,"general:cmdport_local", 60456);
		timeout = iniparser_getint(conf, "general:cmd_timeout", 15);

		printf("mpt configuration:\n\tLocal cmd port: %d\n\tcmd timeout: %ld\n",port,timeout);
	}
	timeout = timeout *1000; // convert to msec



    if (argc == 1 || (argc==2 && (strncmp(argv[1], "-h", 2)==0 || !strcmp(argv[1], "--help")))) {
        printf("\n");
        printf("   Usage: mpt [-6] [-port_number] mpt_command mpt_args\n");
        printf("        -6 : Use IPv6 to communicate with mptsrv\n");
        printf("        -port_number : The port number of the mptsrv (default: %d)\n", port);
        printf("\n");
        printf("   Commands:\n\n");
        printf(HELP);
        return 1;
    }

    memset(&sockaddr, 0, socklen);
    st = 1;
    if (strcmp(argv[1], "-6")==0) {
        inet_pton(AF_INET6, "::1", &(sockaddr.sin6_addr));
        st = 2;
    }
    else
        inet_pton(AF_INET6, "::FFFF:127.0.0.1", &(sockaddr.sin6_addr));


    if (argc > st && argv[st][0]=='-') {
        port = atoi(&argv[st][1]);
        st += 1;
        printf("Using CMD_port number from command line argument: %d \n", port);
    }
    sockaddr.sin6_port   = htons(port);
    sockaddr.sin6_family = AF_INET6;
    if ((sock6 = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
            exerror("Socket creation error.", errno);

    setsockopt(sock6, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&off, sizeof off);
    // IPV6_only is turned off, so this socket will be used for IPv4 too.

    strcpy(cmd, "");
    for (i=st; i<argc; i++) {
        if (i != st) strcat(cmd, " ");
        strcat(cmd, argv[i]);
    }

    if((rv = parse_cmd(cmd)) == PARSE_OK) {
        sendto(sock6, ccmd, strlen(cmd)+5, 0, (struct sockaddr *)&sockaddr, socklen);
        for(i=0; i<2; ) {
            st = getcmd(sock6, ccmd, SIZE_DGRAM, 0, (struct sockaddr *)&sockaddr, &socklen, timeout);
//    printf("i:%d st: %d ccmd0=%X\n",i, st, ccmd[0]);
            if ((unsigned char)ccmd[0] == (unsigned char)CMD_LOCAL_REPLY_END) break;
            if (st <= 4) { printf("ERROR: No answer from mptsrv - unsuccessful receive %d\n", ++i); }
            if (st > 4) printf("%s\n", cmd);
        }
    }

    iniparser_freedict(conf);
    close(sock6);

    return rv;
}
