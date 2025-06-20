#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../include/xxhash.h"
#include "../include/sort_csv.h"
#include "../include/serv_utils.h"
#include "../include/cli_store_utils.h"

#define PACK_SIZE 40 //amount of vlan_id-mac pairs in one 'sendto' call
#define REQUEST_LEN 6 //max amount of digits in request number from client
#define MAX_CLIENTS 1000
#define OLD_CLI_PERC 10 //% of clients to delete if hash table is full, must be greater than zero
#define HASH_TABLE_SIZE 16384 //chose value: 256,512,1024,2048,4096,8192,16384,32768,65536,131072,262144,524288...
#define CSV_FILE_NAME "vlan_mac.csv" //file name in case path to file was not in start options


int main(int argc, char *argv[]){

	FILE *csv_fd = handle_opts(argc, argv, CSV_FILE_NAME);

	int pairs_num = count_entries(csv_fd);
	if ((pairs_num) == 0){
		printf("[ERROR] CSV with {vlan_id/mac} pairs is empty, exiting...\n");
		fclose(csv_fd);
		exit(EXIT_FAILURE);
	}

	VlanMacEntry *vlan_mac_storage = (VlanMacEntry *)malloc(sizeof(VlanMacEntry) * pairs_num);
	if(vlan_mac_storage == NULL){
		perror("[ERROR] malloc vlan_mac_storage");
		exit(EXIT_FAILURE);
	}
	VlanMacEntry **vlan_mac_sorted = (VlanMacEntry **)calloc(pairs_num, sizeof(VlanMacEntry *));
	if(vlan_mac_sorted == NULL){
		perror("[ERROR] calloc vlan_mac_sorted");
		free(vlan_mac_storage);
		exit(EXIT_FAILURE);
	} 

	sort_csv(&pairs_num, csv_fd, vlan_mac_storage, vlan_mac_sorted);
	fclose(csv_fd);

	//print out sorted pairs, for debugging
	// for(int i = 0; i < pairs_num; i++){
	// 	int vlan = vlan_mac_sorted[i]->vlan_id;
	// 	printf("pairN: %d VLAN:_%d\n", i, vlan);
	// }

	int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd == -1){
		perror("[ERROR] socket:");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in cli_addr;
	struct sockaddr_in serv_addr;

	memset(&cli_addr, 0, sizeof(cli_addr));
	memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_port = 0;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int flags = fcntl(sock_fd, F_GETFL, 0);
	fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);
	
	if(bind(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
		perror("[ERROR] bind:");
		close(sock_fd);
		exit(EXIT_FAILURE);
	}

	//get assigned port and print in terminal to pass the port to clients
	struct sockaddr_in sock_self = {0};
	socklen_t sock_len = sizeof(sock_self);
	if (getsockname(sock_fd,(struct sockaddr *) &sock_self, &sock_len)< 0){
		perror("[ERROR] getsockname:");
		close(sock_fd);
		exit(EXIT_FAILURE);
	}
	int port = ntohs(sock_self.sin_port);
	printf("Server is listening on port %d...\n"
		   "Press Ctrl + C to exit\n", port);

	//set up epoll
	int epoll_fd = epoll_create1(0);
	if (epoll_fd == -1){
		perror("[ERROR] create epoll:");
		close(sock_fd);
		exit(EXIT_FAILURE);
	}

	struct epoll_event ev;
	ev.data.fd = sock_fd; 
	ev.events = EPOLLIN | EPOLLET;

	if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &ev) < 0){
		perror("[ERROR] add socket fd to epoll:");
		close(sock_fd);
		exit(EXIT_FAILURE);
	}

	struct epoll_event event_buff;
	char recv_buff[REQUEST_LEN];
	memset(recv_buff, 0, REQUEST_LEN);

	//set up pools
	Client cli_pool[MAX_CLIENTS];
	memset(cli_pool, 0, sizeof(Client) * MAX_CLIENTS);
	time_t freelist[MAX_CLIENTS] = {0}; //indexies of free slots in cli_pool, 0 in free slots, unix time in occupied slots
	//TODO: uncomment when implemented
	// int old_cli_buff_size = (int)(MAX_CLIENTS * (OLD_CLI_PERC / 100.0));
	// int old_cli_buff[old_cli_buff_size];
	Client *hash_table[HASH_TABLE_SIZE] = {0};

	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	while(1){
		int epoll_ret = epoll_wait(epoll_fd, &event_buff, 1, -1);
		if (epoll_ret < 0 && errno != EAGAIN){
			perror("[ERROR] epoll wait:");
			close(sock_fd);
			exit(EXIT_FAILURE);
		}
		
		ssize_t recv_ret = recvfrom(sock_fd, recv_buff, sizeof(recv_buff), 0, (struct sockaddr *)&client_addr, &client_len);
		uint32_t request_net;
		memcpy(&request_net, recv_buff, sizeof(request_net));
		uint32_t request = ntohl(request_net);

		size_t hash = get_hash(&client_addr, HASH_TABLE_SIZE);
		
		// printf("Received number: %d\n Hash: %ld\n\n", request, hash); //for debugging

		Client * cli;
		int free_slot = get_free_slot(freelist, MAX_CLIENTS);
		if(free_slot > 0){
			cli = handle_cli(hash, freelist, HASH_TABLE_SIZE, free_slot, cli_pool, hash_table, &client_addr);
		} else {
			//TODO: uncomment when implemented
			// del_old_cli(freelist, MAX_CLIENTS, HASH_TABLE_SIZE, old_cli_buff_size, old_cli_buff, cli_pool, hash_table);
			del_first_cli(cli_pool, freelist, hash_table, HASH_TABLE_SIZE);
			free_slot = get_free_slot(freelist, MAX_CLIENTS);
			cli = handle_cli(hash, freelist, HASH_TABLE_SIZE, free_slot, cli_pool, hash_table, &client_addr);
		}
		int resp_ret = send_resp(sock_fd, cli, pairs_num, request, client_len, PACK_SIZE, (struct sockaddr *)&client_addr, vlan_mac_sorted);
		if(resp_ret == 1) del_cli(hash, cli, cli_pool, freelist, hash_table); //cli has recieved all data
	}

	free(vlan_mac_storage);
	free(vlan_mac_sorted);
	close(epoll_fd);
	close(sock_fd);
	return 0;
}
