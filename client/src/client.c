#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../include/cli_utils.h"

#define LINE_LEN 27 //len of one record with vlan_id/mac pair
#define PACK_SIZE 40 //max amount of vlan_id-mac pairs in one packet from server
#define MAX_USER_INPUT 8 //max amount of chars to be read from stdin

int main(int argc, char *argv[]){

	int serv_port = 0;
	char *serv_ip = "0";

	read_opts(argc, argv, &serv_port, &serv_ip); //validate start up options

	int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd == -1){
		perror("[ERROR] socket:");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(serv_port);
	inet_pton(AF_INET, serv_ip, &(serv_addr.sin_addr.s_addr));
	struct timeval timeout = {.tv_sec = 1, .tv_usec = 0};
	setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	uint32_t rec_pairs_num = 0;
	char buff[MAX_USER_INPUT] = {0};
	char recv_buff[PACK_SIZE * LINE_LEN];
	socklen_t serv_len = sizeof(serv_addr);
	
	set_input_mode(); //set terminal to non-canonical mode

	int inp_ret;
	while(1){
		inp_ret = read_input(buff, &rec_pairs_num, MAX_USER_INPUT);
		if(inp_ret == 1){
			reset_input_mode();
			close(sock_fd);
			exit(EXIT_SUCCESS);
		}
		if(inp_ret == 0){
			uint32_t net_rec_pairs_num = htonl(rec_pairs_num);
			ssize_t send_ret = sendto(sock_fd, &net_rec_pairs_num, sizeof(net_rec_pairs_num),0,
									 (struct sockaddr *)&serv_addr, serv_len);
			if(send_ret == -1){
				perror("[ERROR] sendto:");
				printf("Failed to send request, try again");
				continue;
			}
			ssize_t recv_ret;
			while((recv_ret = recvfrom(sock_fd, recv_buff, sizeof(recv_buff), 0,
									  (struct sockaddr *)&serv_addr, &serv_len)) > 0){
				printf("\n%s", recv_buff);
				memset(recv_buff, 0, sizeof(recv_buff));
			}
			if(recv_ret == -1){
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					continue;
				} else {
					perror("[ERROR] recvfrom:");
					break;
				}
			}
		}
		if(inp_ret == -1){
			perror("[ERROR] error read input:");
			continue;
		}
	}
	reset_input_mode();
	close(sock_fd);
	return 0;
}