/**
 * @file main.c
 * @brief Main code of the MPT server application
 */

#include <string.h>

#include <sys/time.h>
#include "multipath.h"
#include "mp_local.h"

#include "cli.h"
#include "auth.h"


/**
 * Main function of server applcation that creates the tunnel interface, starts threads and configures the connections
 */
int main(int argc, char **argv)
{
//char buff[SIZE_DGRAM];
    puts("Initializing...");
    initialize();
    puts("Initialization is done.");

    puts("Loading Tunnel interface configuration file...");
    interface_load("conf/interface.conf", NULL);
    puts("Interface configuration loaded.");

    puts("Loading connection configurations...");
    connection_load_dir(NULL);
    puts("Connection configuration is loaded.");

    puts("\nStarting MPT Tunnel & Service\n");

    tunnel_start(NULL);
    tunnel_print(NULL);

	connection_start(NULL);
	connection_print(NULL);

    char c[100], *str;
    str = c;
    while (1) {
        printf("\n");
        printf(" Type q to quit, or \n");
        printf("      p to print connection information.\n");
        printf("      r to restart MPT service and reload connection configuration.\n");
        c[0]='0'; c[1]=0;
        if (argc != 2 || strcmp(argv[1], "-d") ) str = fgets(c, 100, stdin);
        printf("Cmd(%d):%c\n", (int)strlen(str), c[0]);
        if (c[0] == 'q') break;
        if (c[0] == 'p') {
            tunnel_print(stdout);
            connection_print(stdout);
        }
        if (c[0] == 'r') {
            puts("Restarting all connections.");
            connection_reload(NULL);
            connection_print(stdout);
        }
        sleep(2);
    } // while 1
    
    //XXX there must be a better way to do that
    system("while ip rule delete from 0/0 to 0/0 table 9999 2>/dev/null; do true; done");

    connection_stop(NULL);
    tunnel_stop(NULL);

    return 0;
}
