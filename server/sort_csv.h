#ifndef SORT_SCV
#define SORT_SCV

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