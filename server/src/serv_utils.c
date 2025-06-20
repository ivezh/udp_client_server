#include <stdlib.h>
#include "../include/sort_csv.h"
#include "../include/serv_utils.h"
#include "../include/cli_store_utils.h"


FILE * handle_opts(int argc, char *argv[], const char *file_name){
	FILE *csv_fd;
	if(argc == 1){
		csv_fd = fopen(file_name, "r");
		if(csv_fd == NULL){
			printf("!!! Could not find default file \"%s\" in current directory.\n"
				   "Place csv file with vlan_id/mac pairs in current directory," 
				   "or provide a valid path at start: ./server <file_path>\n", file_name);
			exit(EXIT_FAILURE);
		}
	} else {
		csv_fd = fopen(argv[1], "r");
		if(csv_fd == NULL){
			printf("[ERROR] could not find csv file by path: %s", argv[1]);
			exit(EXIT_FAILURE);
		}
	}
	return csv_fd;
}

inline int send_resp(int sock_fd,
					 Client *client,
					 uint32_t pairs_num,
					 uint32_t req_amount,  
					 socklen_t client_len,
					 uint32_t pairs_per_pack,
					 struct sockaddr *client_addr,
					 VlanMacEntry **vlan_mac_sorted){
	char buff[LINE_LEN];
	char payload[LINE_LEN * pairs_per_pack];
	memset(payload, 0, sizeof(payload));

	int start = client->offset; // storage index from which the pairs will be sent  
	int end = (req_amount >= pairs_num || (req_amount + start) >= pairs_num) ? pairs_num : req_amount + start; // storage index of the last pair to send
	
	while(start < end){
		int count = 0;
		while(count < pairs_per_pack && start + count < end){
			snprintf(buff, sizeof(buff), "%d, %s\n", vlan_mac_sorted[start + count]->vlan_id,
													 vlan_mac_sorted[start + count]->mac);
			strncat(payload, buff, strlen(buff));
			count++;
		}
		start += count;
		int ret = sendto(sock_fd, payload, strlen(payload), 0, client_addr, client_len);
		if(ret == -1){
			perror("[ERROR] sendto:");
		}
		memset(payload, 0, sizeof(payload));  
	}
	client->offset = end;
	bool is_all_pairs_sent = (end == pairs_num) ? true : false;
	if (is_all_pairs_sent) {
		return 1; //case: nothing left to send for the cli, so cli can be deleted from cli storage
	} else {
		return 0;
	}
}