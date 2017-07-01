#include "iniparser.h"
#include <ctype.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>

typedef struct
{
    char name[16];
    char ip_address[16];
    char gw_ip_address[16];
    int  is_ppp;
} interface;

struct in_addr get_interface_ip_address(const char *interface_name)
{
    int sock;
    long add;
    struct ifreq req;

    memset(&req, 0, sizeof(struct ifreq));

    strcpy(req.ifr_name, interface_name);

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
	perror("socket");
	// exit(errno);
    }

    if (ioctl(sock, SIOCGIFADDR, &req) == -1) {
	perror("ioctl");
	// exit(errno);
    }
    return ((struct sockaddr_in *)(&(req.ifr_addr)))->sin_addr;
}

struct in_addr get_ppp_interface_dst_ip_address(const char* interface_name)
{
    int sock;
    long add;
    struct ifreq req;

    memset(&req, 0, sizeof(struct ifreq));

    strcpy(req.ifr_name, interface_name);

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
	perror("socket");
	// exit(errno);
    }

    if (ioctl(sock, SIOCGIFDSTADDR, &req) == -1) {
	perror("ioctl");
	// exit(errno);
    }
    return ((struct sockaddr_in *)(&(req.ifr_dstaddr)))->sin_addr;
}

int is_ppp(interface * iface)
{
	struct ifreq req;
	memset(&req, 0, sizeof(struct ifreq));
	strcpy(req.ifr_name, iface->name);
	int sock, err;
	sock = socket(AF_INET, SOCK_DGRAM,0);
	if(sock == -1)
	{
		perror("Socket creation error");
		return errno;
	}
	
	err = ioctl(sock, SIOCGIFFLAGS,&req);
	
	if(err == -1)
	{
		printf("\n%s\n",iface->name);
		perror("Ioctl error");
		return errno;
	}
	
	if(req.ifr_flags & IFF_POINTOPOINT)
		iface->is_ppp = 1;
	else
		iface->is_ppp = 0;
	
	return 0;
}

int main(int argc, char** argv)
{
	
	if(argc<2)
	{
		fprintf(stderr,"Too few arguments\n");
		exit(-1);
		
	}
	
	
	dictionary *conf;
	conf = iniparser_load(argv[1]);
	int path_num,err;
	char *tmp, name_def[]="error", gw_def[] = "0.0.0.0", addr_def[] = "0.0.0.0", key[64], path_index[1], *gw_ipaddr, *local_ipaddr, *interface_name;
	
	path_num = iniparser_getint(conf,"connection:path_count",0);
	if(path_num == 0)
		exit(-1);
	
	interface **interface_array, *temp;
	
	interface_array = (interface**)malloc(sizeof(interface*)*path_num);
	
	memset(interface_array,0,sizeof(interface*)*path_num);
	
	int i;
	for(i = 0; i < path_num; i++)
	{
		
		memset(key,0,sizeof(char)*64);
		
		sprintf(key,"%s%d:%s","path_",i,"gw_ipaddr");
		gw_ipaddr = iniparser_getstring(conf,key,gw_def);
		memset(key,0,sizeof(char)*64);
		sprintf(key,"%s%d:%s","path_",i,"local_ipaddr");
		local_ipaddr = iniparser_getstring(conf,key,addr_def);
		
		sleep(1);
		
		if( (gw_ipaddr[0]!=0 && strcmp(gw_ipaddr,gw_def)) && (local_ipaddr[0] != 0 && strcmp(local_ipaddr, addr_def)) )
			continue;
		
		memset(key,0,sizeof(char)*64);
		sprintf(key,"%s%d:%s","path_",i,"interface_name");
		printf("(%s)\n",key);
		interface_name = iniparser_getstring(conf,key,name_def);
		
		if( interface_name[0] == 0 || strcmp(interface_name,name_def) == 0)
			continue;
		
		temp = (interface*) malloc(sizeof(interface));
		memset(temp,0,sizeof(interface));
		strcpy(temp->name,interface_name);
		err = is_ppp(temp);
		if(err != 0)
		{
			free(temp);
			continue;
		}
		
		
		interface_array[i] = temp;
		
		
		if(gw_ipaddr[0]!=0 && strcmp(gw_ipaddr,gw_def))
		{
			strcpy(interface_array[i]->gw_ip_address,gw_ipaddr);
		}
		
		if(local_ipaddr[0] != 0 && strcmp(local_ipaddr, addr_def))
		{
			strcpy(interface_array[i]->ip_address,local_ipaddr);
		}
		
		printf("Is %s a ppp interface? %d\n",interface_array[i]->name,interface_array[i]->is_ppp);
		printf("%s\ngw: %s\nip: %s\n",interface_array[i]->name, interface_array[i]->gw_ip_address,interface_array[i]->ip_address);
		
		
	}
	
	
	
	FILE *popn_f;
	char popn_s[64], *popn, *end;
	popn_f = popen("ip -4 route list default","r");
	fgets(popn_s,64,popn_f);
	pclose(popn_f);
	printf("\n----------------------------------------------------\n");
	printf("Checking the default gateway...");
	printf("\n----------------------------------------------------\n");
	printf("ip -4 route list default\n");
	
	interface default_gw;
	memset(&default_gw,0, sizeof(interface));
	
	sleep(1);
	if(strlen(popn_s) > 0)
	{
		printf("%s", popn_s);
		popn = strstr(popn_s, "dev");
		//printf("%s\n", popn);
		sleep(1);
		if(popn !=NULL)
		{
			popn = strchr(popn, ' ');
			popn++;
			//printf("-%s\n",popn);

			
			end = strchr(popn, ' ');
			*end = '\0';
			
			//printf("-(%s)\n",popn);
			
			strcpy(default_gw.name,popn);
			
			popn = strstr(popn_s,"via");
			//printf("-(%s)\n",popn);
			popn = strchr(popn,' ');
			//printf("-(%s)\n",popn);
					
			popn++;
			//printf("-(%s)\n",popn);
			end = strchr(popn, ' ');
			*end = '\0';
					
			//printf("-(%s)\n",popn);
			
			strcpy(default_gw.gw_ip_address,popn);
			
			sleep(1);
			for(i=0; i<path_num; i++)
			{
				if(interface_array[i] == NULL)
					continue;
				
				if(strcmp(interface_array[i]->name,default_gw.name) == 0)
				{
					
					memset(key,0,sizeof(char)*64);
					sprintf(key,"%s%d:%s","path_",i,"gw_ipaddr");
					gw_ipaddr = iniparser_getstring(conf,key,gw_def);
					
					if(gw_ipaddr[0] != 0 && strcmp(gw_ipaddr, gw_def))
					{
						printf("Next hop for %s is staticaly configured in the config file.\n\n", interface_array[i]->name);
						break;
						sleep(1);
					}
					
					
					
					strcpy(interface_array[i]->gw_ip_address, default_gw.gw_ip_address);
					printf("Next hop for %s is found: %s\n",interface_array[i]->name,interface_array[i]->gw_ip_address);
					iniparser_set(conf, key, interface_array[i]->gw_ip_address);
					
					if(interface_array[i]->ip_address[0]==0)
					{
						
						printf("\n----------------------------------------------------\n");
						printf("Asking for outside nat address of %s",interface_array[i]->name);
						printf("\n----------------------------------------------------\n");
					
						memset(popn_s, 0, sizeof(char)*64);
						popn_f = popen("./nat_addr.sh","r");
						fgets(popn_s,64,popn_f);
						pclose(popn_f);
				
						if(isdigit(popn_s[0]))
						{
							strcpy(interface_array[i]->ip_address, popn_s);
							memset(key,0,sizeof(char)*64);
							sprintf(key,"%s%d:%s","path_",i,"local_ipaddr");
							iniparser_set(conf, key, interface_array[i]->ip_address);
							printf("%s nat outside address: %s\n",interface_array[i]->name,interface_array[i]->ip_address);
						}
						else
							printf("No answer from the remote server...\n");
						
						}
						
					sleep(1);
					
				}
				
			}
		}
	}
	else
	{
		printf("Default gw is not present.\n");
		sleep(1);
		
	}
	
/*
	for(i = 0; i < path_num; i++)
	{
		
		if(interface_array[i] == NULL)
			continue;
		
		printf("Is %s a ppp interface? %d\n",interface_array[i]->name,interface_array[i]->is_ppp);
		printf("%s\ngw: %s\nip: %s\n",interface_array[i]->name, interface_array[i]->gw_ip_address,interface_array[i]->ip_address);
		sleep(1);
		
	}
*/
	
	/*printf("\n----------------------------------------------------\n");
	printf("Shutting down the interfaces");
	printf("\n----------------------------------------------------\n");*/
	char cmd[64];
	memset(cmd,0,sizeof(char)*64);
	for(i = 0; i<path_num; i++)
	{
		if(interface_array[i] == NULL)
			continue;
		
		if((interface_array[i]->gw_ip_address[0] == 0 || interface_array[i]->ip_address[0]==0) && interface_array[i]->is_ppp != 1)
		{
			printf("\n----------------------------------------------------\n");
			printf("Shutting %s down",interface_array[i]->name);
			printf("\n----------------------------------------------------\n");
			sprintf(cmd,"./down.sh %s",interface_array[i]->name);
			printf("%s\n",cmd);
			system(cmd);
			sleep(1);
		}
	}
	
	/*printf("\n----------------------------------------------------\n");
	printf("");
	printf("\n----------------------------------------------------\n");*/
	
	for(i = 0; i<path_num; i++)
	{
		if(interface_array[i] == NULL)
			continue;
		
		
		
		if(interface_array[i]->gw_ip_address[0] == 0 || interface_array[i]->ip_address[0] == 0)
		{
			
			
			if(interface_array[i]->is_ppp == 0)
			{
				printf("\n----------------------------------------------------\n");
				printf("Turning  %s on",interface_array[i]->name);
				printf("\n----------------------------------------------------\n");
				sleep(1);
				
				system("./def_gw_delete.sh");
				sprintf(cmd,"./up.sh %s",interface_array[i]->name);
				printf("%s\n",cmd);
				system(cmd);
				
				if(interface_array[i]->gw_ip_address[0] == 0 )
				{
					
					printf("\n----------------------------------------------------\n");
					printf("Determining next hop ip address on %s",interface_array[i]->name);
					printf("\n----------------------------------------------------\n");
					sleep(1);
				
					memset(popn_s, 0, sizeof(char)*64);
					popn_f = popen("ip -4 route list default","r");
					fgets(popn_s,64,popn_f);
			
					popn = strstr(popn_s,"via");
					if(popn)
					{
						popn = strchr(popn,' ');
					
					
						popn++;
					
						end = strchr(popn, ' ');
						*end = '\0';
					
						strcpy(interface_array[i]->gw_ip_address, popn);
						pclose(popn_f);
				
						memset(key,0,sizeof(char)*64);
						sprintf(key,"%s%d:%s","path_",i,"gw_ipaddr");
						iniparser_set(conf, key, interface_array[i]->gw_ip_address);
						printf("%s next hop: %s\n",interface_array[i]->name,interface_array[i]->gw_ip_address);
					}
					else
						fprintf(stderr,"Unknown error\n");
				}
				
				if(interface_array[i]->ip_address[0] == 0)
				{
					printf("\n----------------------------------------------------\n");
					printf("Asking for outside nat address of %s",interface_array[i]->name);
					printf("\n----------------------------------------------------\n");
					
					memset(popn_s, 0, sizeof(char)*64);
					popn_f = popen("./nat_addr.sh","r");
					fgets(popn_s,64,popn_f);
					pclose(popn_f);
				
					if(isdigit(popn_s[0]))
					{
						strcpy(interface_array[i]->ip_address, popn_s);
						memset(key,0,sizeof(char)*64);
						sprintf(key,"%s%d:%s","path_",i,"local_ipaddr");
						iniparser_set(conf, key, interface_array[i]->ip_address);
						printf("%s nat outside address: %s\n",interface_array[i]->name,interface_array[i]->ip_address);
					}
					else
						printf("No answer from the remote server...\n");
				}
				sleep(1);
				
			}
			else
			{
				
				
				struct in_addr dst_addr;
			
				dst_addr = get_ppp_interface_dst_ip_address(interface_array[i]->name);
				
				
				if(interface_array[i]->gw_ip_address[0] == 0)
				{	
					printf("\n----------------------------------------------------\n");
					printf("Determining next hop ip address on %s",interface_array[i]->name);
					printf("\n----------------------------------------------------\n");
					sleep(1);
					inet_ntop(AF_INET,&dst_addr,interface_array[i]->gw_ip_address,INET_ADDRSTRLEN);
					memset(key,0,sizeof(char)*64);
					sprintf(key,"%s%d:%s","path_",i,"gw_ipaddr");
					iniparser_set(conf, key, interface_array[i]->gw_ip_address);
					printf("%s next hop: %s\n",interface_array[i]->name,interface_array[i]->gw_ip_address);
				}
				
				if(interface_array[i]->ip_address[0] == 0)
				{
					printf("\n----------------------------------------------------\n");
					printf("Asking for outside nat address of %s",interface_array[i]->name);
					printf("\n----------------------------------------------------\n");
					sleep(1);
					
					char ppp_dst[16];
					inet_ntop(AF_INET,&dst_addr,ppp_dst,INET_ADDRSTRLEN);
					system("./def_gw_delete.sh");
					memset(cmd,0,sizeof(char)*64);
					sprintf(cmd,"./add_def_gw.sh %s %s",ppp_dst, interface_array[i]->name);
					system(cmd);
					
					
					memset(popn_s, 0, sizeof(char)*64);
					popn_f = popen("./nat_addr.sh","r");
					fgets(popn_s,64,popn_f);
					pclose(popn_f);
				
					if(isdigit(popn_s[0]))
					{
						strcpy(interface_array[i]->ip_address, popn_s);
						memset(key,0,sizeof(char)*64);
						sprintf(key,"%s%d:%s","path_",i,"local_ipaddr");
						iniparser_set(conf, key, interface_array[i]->ip_address);
						printf("%s nat outside address: %s\n",interface_array[i]->name,interface_array[i]->ip_address);
					}
					else
						printf("No answer from the remote server...\n");
				}
					
				sleep(1);
				
			}
		}
	}
	
	/*printf("\n----------------------------------------------------\n");
	printf("");
	printf("\n----------------------------------------------------\n");*/
	
	for(i = 0; i<path_num; i++)
	{
		if(interface_array[i] == NULL)
			continue;
		
		
		
		if(interface_array[i]->ip_address[0] == 0)
		{
			
			printf("\n----------------------------------------------------\n");
			printf("Getting local ip address of %s",interface_array[i]->name);
			printf("\n----------------------------------------------------\n");
			sleep(1);
			
			struct in_addr ip_addr;
			
			ip_addr = get_interface_ip_address(interface_array[i]->name);
				
			inet_ntop(AF_INET,&ip_addr,interface_array[i]->ip_address,INET_ADDRSTRLEN);
				
			memset(key,0,sizeof(char)*64);
			sprintf(key,"%s%d:%s","path_",i,"local_ipaddr");
			iniparser_set(conf, key, interface_array[i]->ip_address);
			printf("%s ip: %s\n",interface_array[i]->name,interface_array[i]->ip_address);
		}
	}
	for(i = 0; i<path_num; i++)
	{
		if (interface_array[i] == NULL)
			continue;
		
		free(interface_array[i]);
	}
	
	if(default_gw.name[0]!=0)
	{
		printf("\n----------------------------------------------------\n");
		printf("Restoring the original default gateway");
		printf("\n----------------------------------------------------\n");
		
		system("./def_gw_delete.sh");
		memset(cmd,0,sizeof(char)*64);
		sprintf(cmd,"./add_def_gw.sh %s %s",default_gw.gw_ip_address, default_gw.name);
		system(cmd);
	}
	
	free(interface_array);
	
    FILE *f = fopen(argv[1], "w");

    iniparser_dump_ini(conf,f);
    fclose(f);
    iniparser_freedict(conf);


    return 0;
}