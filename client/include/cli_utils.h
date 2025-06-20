#ifndef CLI_UTILS
#define CLI_UTILS

#include <termios.h>
#include <stdbool.h>
#include <stdint.h>

bool isValidIp(char *ip);

bool isValidPort(char *port);

void set_input_mode(void);

void reset_input_mode (void);

void read_opts(int argc, char *argv[], int *port, char **ip);

int read_input(char *buff, uint32_t *rec_pairs_num, int max_input_len);

# endif // CLI_UTILS
