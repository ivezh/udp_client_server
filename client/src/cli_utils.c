#include "./cli_utils.h"
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <ctype.h>
#include <sys/epoll.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <stdbool.h>

struct termios saved_attributes;

char *help = "Launch: <server_IP(v4)> <server_port>\n"
			 "\t - Start with server IP and port;\n"
			 "\t - Enter '127.0.0.1' as IP address if server and client share same host;\n"
			 "Usage:\n"
			 "\t - After start enter {N} - number of VLAN_ID/MAC pairs to receive from server;\n"
			 "\t - Enter <space> to receive next {N} pairs or new {N};\n"
			 "\t - Enter <q> to exit;\n";

bool isValidIp(char *ip){
	struct sockaddr_in sa;
	int res = inet_pton(AF_INET, ip, &(sa.sin_addr));
	return res != 0;
}

bool isValidPort(char *port){
	int port_dec = strtol(port, NULL, 10);
    return (port_dec >= 1024 && port_dec <= 65535);
}

void set_input_mode(void){
    struct termios tattr;

    if (!isatty(STDIN_FILENO)) {
        fprintf(stderr, "Not a terminal.\n");
        exit(EXIT_FAILURE);
    }

    tcgetattr(STDIN_FILENO, &saved_attributes);
    atexit(reset_input_mode);
    tcgetattr(STDIN_FILENO, &tattr);

    tattr.c_lflag &= ~(ICANON | ECHO | ISIG);
	tattr.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &tattr);
}

void reset_input_mode (void){
	tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);
}

void read_opts(int argc, char *argv[], int *port, char **ip){
	if(argc != 3){
		printf("%s", help);
		exit(EXIT_SUCCESS);
	}

	if(!isValidIp(argv[1])){
		printf("Invalid server IP, expected like: 198.123.0.100 or 127.0.0.1 for localhost\n");
		exit(EXIT_FAILURE);
	}
	*ip = argv[1];

	if(!isValidPort(argv[2])){
		printf("Invalid server port, expected integer value in range [1024:65535]\n");
		exit(EXIT_FAILURE);
	} 
	*port = strtol(argv[2], NULL, 10);
}

int read_input(char *buff, uint32_t *rec_pairs_num, int max_input_len){
	printf("\nNew {N} or <space>:");
	fflush(stdout);
    
	int chars_amount = 0; //current amount of entered chars
    while (chars_amount < max_input_len - 1){
        char c;
        int read_ret = read(STDIN_FILENO, &c, 1);
		if (read_ret < 0) {
			perror("[ERROR] read input:");
			continue;
		}
		
		if (c == 'q') return 1; //case: exit program
		if (c == '0' && chars_amount == 0) continue; //ignore leading zeros
		if (c == '\n' && chars_amount == 0) continue; //ignore alone '\n'
        if (c == ' ' && *rec_pairs_num != 0) return 0; //case: request next N vlan_id/mac pairs
		if (isdigit((unsigned char)c) == 0 && c != '\n' && c != 'q' && c != ' ') continue; //ignore non-digit chars
		if (c == ' ' && *rec_pairs_num == 0){
			printf("\n {N} is not yet entered for using spaces, set {N} first\n");
			break;
		} else {
			fputc(c, stdout);
			fflush(stdout);
		}
		buff[chars_amount++] = c;
		if(c == '\n' && chars_amount != 0){
			buff[chars_amount + 1] = '\0';
			if(sscanf(buff, "%d", rec_pairs_num) == 1 && *(int *)rec_pairs_num > 0){
				return 0;
			} else {
				continue; //case: invalid input
			}
		}
    }
	return -1;
}
