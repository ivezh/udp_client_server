#ifndef CLI_STORAGE
#define CLI_STORAGE

#include <time.h>
#include <stdint.h> 
#include <stdbool.h>            
#include <netinet/in.h>     
#include <sys/socket.h>     
#include "xxhash.h"

typedef struct {
    struct sockaddr_in client_addr;  
    uint32_t offset;                                
} Client;

size_t get_hash(const struct sockaddr_in* addr, size_t hash_table_size);

//find free slot in storage for new client
int get_free_slot(int * freelist, int free_list_size);

//delete one client
void del_cli(size_t hash,
			 Client *cli,
			 Client *cli_pool, 
			 time_t *freelist, 
			 Client **hash_table);

void del_first_cli(Client *cli_pool, 
						  time_t *freelist, 
						  Client **hash_table,
						  size_t hash_table_size);

//TODO: uncomment when implemented			 
//clean storage from old clients
// void del_old_cli(time_t *freelist,
// 				 int max_clients,
// 				 int hash_t_size,
// 				 int old_cli_amount,
// 				 int *old_cli_buff,
// 				 Client *cli_pool,
// 				 Client **hash_table);

//add new client or return ptr if client is already exist
Client *handle_cli(size_t hash,
				   time_t *freelist,
				   int hash_t_size,
				   int free_slot_index, 
				   Client *cli_pool,
				   Client **hash_table,  
				   struct sockaddr_in *addr);
#endif // CLI_STORAGE