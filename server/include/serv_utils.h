#ifndef SERV_UTILS
#define SERV_UTILS

#include <stdio.h>
#include "../include/sort_csv.h"
#include "../include/cli_store_utils.h"

FILE * handle_opts(int argc, char *argv[], const char *file_name);

int send_resp(int sock_fd,
			  Client *client,
			  uint32_t pairs_num,
			  uint32_t req_amount,  
			  socklen_t client_len,
			  uint32_t pairs_per_pack,
			  struct sockaddr *client_addr,
			  VlanMacEntry **vlan_mac_sorted);

#endif // SERV_UTILS