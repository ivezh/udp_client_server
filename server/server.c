#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "sort_csv.h"

#define REQUEST_LEN 6 //defines how many digits in mumber from client can be recieved
#define LINE_LEN 27 //length of line with vlan_id-mac pair
#define CSV_FILE_NAME "vlan_mac_10000.csv"
#define PACK_SIZE 40 //amount of vlan_id-mac pairs in one 'sendto' call


typedef struct client{
	struct sockaddr_in *cli_addr;
	int cli_offset;
} Client;

int send_resp(int offset,
			  int sock_fd,
			  uint32_t pairs_num,
			  uint32_t req_amount,  
			  socklen_t client_len, 
			  uint32_t lines_per_pack,
			  struct sockaddr *client_addr, 
			  VlanMacEntry **vlan_mac_sorted){

	char buff[LINE_LEN];
	char payload[LINE_LEN * lines_per_pack];

	int frame_start = offset;
	int frame_end = (req_amount >= pairs_num) ? pairs_num : req_amount;
	memset(payload, 0, sizeof(payload));
	while(frame_start < frame_end){
		int count = 0;
		while(count < lines_per_pack && frame_start + count < frame_end){
			snprintf(buff, sizeof(buff), "%d, %s\n", vlan_mac_sorted[frame_start + count]->vlan_id,
													 vlan_mac_sorted[frame_start + count]->mac);
			strncat(payload, buff, strlen(buff));
			count++;
		}
		frame_start = frame_start + count;
		int send_ret = sendto(sock_fd, payload, strlen(payload), 0, client_addr, client_len);
		memset(payload, 0, sizeof(payload));        
	}
	return 0;
}


int main(){

	FILE *csv_fd = fopen(CSV_FILE_NAME, "r");
	if(csv_fd == NULL){
		perror("[ERROR] could not open csv file:");
		exit(EXIT_FAILURE);
	}

	int pairs_num = count_entries(csv_fd);
	if ((pairs_num) == 0){
		printf("[ERROR] CSV with {vlan_id/mac} pairs is empty, exiting...\n");
		fclose(csv_fd);
		exit(EXIT_FAILURE);
	}

	VlanMacEntry *vlan_mac_storage = (VlanMacEntry *)malloc(sizeof(VlanMacEntry) * pairs_num);
	if(vlan_mac_storage == NULL){
		perror("[ERROR] allocate memory for vlan_mac_storage");
		exit(EXIT_FAILURE);
	}
	VlanMacEntry **vlan_mac_sorted = (VlanMacEntry **)calloc(pairs_num, sizeof(VlanMacEntry *));
	if(vlan_mac_sorted == NULL){
		perror("[ERROR] allocate memory for vlan_mac_sorted");
		exit(EXIT_FAILURE);
	} 

	sort_csv(&pairs_num, csv_fd, vlan_mac_storage, vlan_mac_sorted);
	fclose(csv_fd);

	// print out sorted pairs for tests
	// for(int i = 0; i < pairs_num; i++){
	// 	int vlan = vlan_mac_sorted[i]->vlan_id;
	// 	printf("pairN: %d VLAN:_%d\n", i, vlan);
	// }

	//set up socket
	int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd == -1){
		perror("[ERROR] create server UDP socket:");
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
		perror("[ERROR] bind server socket:");
		close(sock_fd);
		exit(EXIT_FAILURE);
	}

	//get assigned port and print in terminal to pass to clients
	struct sockaddr_in sock_self = {0};
	socklen_t sock_len = sizeof(sock_self);
	if (getsockname(sock_fd,(struct sockaddr *) &sock_self, &sock_len)< 0){
		perror("[ERROR] getting assigned port:");
		close(sock_fd);
		exit(EXIT_FAILURE);
	}
	int port = ntohs(sock_self.sin_port);
	printf("Server is listening on port %d...\n", port);

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

	while(1){
		int epoll_ret = epoll_wait(epoll_fd, &event_buff, 1, -1);
		if (epoll_ret < 0 && errno != EAGAIN){
			perror("[ERROR] epoll wait:");
			close(sock_fd);
			exit(EXIT_FAILURE);
		}
		
		struct sockaddr_in client_addr;
		socklen_t client_len = sizeof(client_addr);
		ssize_t recv_ret = recvfrom(sock_fd, recv_buff, sizeof(recv_buff), 0, (struct sockaddr *)&client_addr, &client_len);
		uint32_t request_net;
		memcpy(&request_net, recv_buff, sizeof(request_net));
		uint32_t request = ntohl(request_net);
		printf("Received number: %d\n", request); //for tests

		int ret = send_resp(0, sock_fd, pairs_num, request, client_len, PACK_SIZE, (struct sockaddr *)&client_addr, vlan_mac_sorted);

	}

	close(epoll_fd);
	close(sock_fd);
	return 0;
}
