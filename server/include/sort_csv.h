#ifndef SORT_SCV
#define SORT_SCV

#define LINE_LEN 27 // 4 for (VLAN_ID number) + 1 for ',' + 16 for (MAC) + 5 for mac delimeters (':')
#define MAX_VLAN_ID 4094 //IMPORTANT: vlan_id = 0 is not included according to RFC 2674

#include <stdio.h>

typedef struct vlan_mac_t {
	int vlan_id;
	char mac[22];
} VlanMacEntry;

int sort_csv(int *pairs_num,
			 FILE * vlan_mac_csv,
			 VlanMacEntry *vlan_mac_storage, 
			 VlanMacEntry **vlan_mac_sorted);


int count_entries(FILE *vlan_mac_csv);


#endif // SORT_SCV