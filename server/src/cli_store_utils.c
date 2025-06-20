#include <time.h>
#include <stdint.h>
#include <string.h> 
#include <stdbool.h>
#include <netinet/in.h>     
#include <sys/socket.h>     
#include "../include/cli_store_utils.h"

inline size_t get_hash(const struct sockaddr_in* addr, size_t hash_table_size) {
    uint64_t key = ((uint64_t)ntohl(addr->sin_addr.s_addr) << 16) | ntohs(addr->sin_port);
    XXH64_hash_t hash = XXH3_64bits(&key, sizeof(key));
    return (size_t)(hash & (hash_table_size - 1));
}

inline int get_free_slot(int * freelist, int free_list_size){
	for(int i = 0; i < free_list_size; i++){
		if(freelist[i] == 0){
			return i;
		}
	}
	return -1;
}

//Delete one client
inline void del_cli(size_t hash,
					Client *cli,
					Client *cli_pool, 
					time_t *freelist, 
					Client **hash_table){
	hash_table[hash] = 0;
	ptrdiff_t i = cli - cli_pool; //since freelist and cli_pool are of the same size
	freelist[i] = 0;
	memset(cli, 0, sizeof(*cli));
}

//delete the very first client in cli pool
void del_first_cli(Client *cli_pool, 
						  time_t *freelist, 
						  Client **hash_table,
						  size_t hash_table_size){
	Client *cli = &cli_pool[0];
	size_t hash = get_hash(&cli->client_addr, hash_table_size);
	hash_table[hash] = 0;
	ptrdiff_t i = cli - cli_pool;
	freelist[i] = 0;
	memset(cli, 0, sizeof(*cli));
}

//TODO: add algorythm for deleting old clients by their timestamps in asc order
//find min timestamp
// static void find_min(int old_cli_amount, time_t *min, int *old_cli_buff){
// 	*min = old_cli_buff[0];
// 	for(int i = 1; i < old_cli_amount; i++){
// 		if(old_cli_buff[i] < *min){
// 			*min = old_cli_buff[i];
// 		}
// 	}
// }

/*
Clean storage from old clients, sorting them by their unix timestamp in freelist
Almost impossible case. Used when no space left for new clients in hashtable
*/

//del first N old clients from cli pool to get free space for new clients
// inline void del_old_cli(time_t *freelist,
// 						int max_clients,
// 						int hash_t_size,
// 						int old_cli_amount,
// 						int *old_cli_buff,
// 						Client *cli_pool,
// 						Client **hash_table){
// 	// time_t min = 0;
// 	// time_t *old_cli_time[old_cli_amount]; //store timestamps of clients to del
// 	// for(int i = 0; i < old_cli_amount; i++){
// 	// 	old_cli_time[i] = 0;
// 	// }
// 	// int *old_cli_indx[old_cli_amount]; //store indexies of clients to del
// 	// for(int j = 0; j < old_cli_amount; j++){
// 	// 	old_cli_indx[j] = 0;
// 	// }
// }

//add new client or return ptr if client is already exist
inline Client *handle_cli(size_t hash,
						  time_t *freelist,
						  int hash_t_size,
						  int free_slot_index, 
						  Client *cli_pool,
						  Client **hash_table,  
						  struct sockaddr_in *addr){
	//case: add new client
	if(hash_table[hash] == 0){
		hash_table[hash] = &cli_pool[free_slot_index]; 
		cli_pool[free_slot_index].client_addr = *addr;
		freelist[free_slot_index] = time(NULL);
		// freelist[free_slot_index] = 1;
		return hash_table[hash];
	}

	//case: already existing client 
	if(memcmp(&hash_table[hash]->client_addr, addr, sizeof(*addr)) == 0){
		return hash_table[hash];

	//case: hash collision, do linear probing 
	} else {
		int iter = 0;
		for(int i = hash - 1; i < hash_t_size; i++){
			//edge case: collision happend in the most last element of hash table
			if(i + 1 == hash_t_size){
				i = 0;
			}
			if(hash_table[i] == 0){
				hash_table[i] = &cli_pool[free_slot_index]; 
				cli_pool[free_slot_index].client_addr = *addr;
				return hash_table[i];
			}
			if(memcmp(&hash_table[i]->client_addr, addr, sizeof(*addr)) == 0){
				return hash_table[i];
			}
			//edge case: no free slots in hash table, avoid infinity loop
			if(iter++ == hash_t_size){
				return NULL;
			}
		}
	}
	return NULL;
}
