#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
// #include "sort_csv.h"


#define LINE_MAX_LEN 27 // 4 for (VLAN_ID number) + 1 for ',' + 16 for (MAC) + 5 for (delimeters ':')
// #define CSV_FILE_NAME "vlan_mac_10^7.csv"
#define MAX_VLAN_ID 4094 //IMPORTANT: vlan_id = 0 is not included according to RFC 2674

typedef struct vlan_mac_t {
	int vlan_id;
	char mac[22];
} VlanMacEntry;

// int count_entries(FILE *vlan_mac_csv, int *entries_num){
int count_entries(FILE *vlan_mac_csv){
	int c;
	int entries_num = 0;
	while((c = getc(vlan_mac_csv)) != EOF){
		if(c == ','){
			entries_num++;
		}
	}
	rewind(vlan_mac_csv);
	return entries_num;
}

int sort_csv(int *entries_num,
			 FILE * vlan_mac_csv,
			 VlanMacEntry *vlan_mac_storage, 
			 VlanMacEntry **vlan_mac_sorted){

	int vlan_amounts[MAX_VLAN_ID + 1] = {0}; //store amount of each vlan_id type 
	int vlan_start_index[MAX_VLAN_ID + 1] = {0}; //store slot start position of each vlan_id type in sorted array

	char line_buff[LINE_MAX_LEN];
	for (int i = 0; i < *entries_num; i++){
		if(fgets(line_buff, LINE_MAX_LEN, vlan_mac_csv) == NULL){
			perror("[ERROR] parse lines");
		}
		if (sscanf(line_buff, "%d,%s",&vlan_mac_storage[i].vlan_id,
							   		   vlan_mac_storage[i].mac) == 2){	
		} else {
			printf("[WARNING] Wrong format of line №%d in csv file, the line: %s\n", i, line_buff);
		}
		vlan_amounts[vlan_mac_storage[i].vlan_id]++; //count amount of each of vlan_id type
		memset(line_buff, 0, sizeof(line_buff)); //clear buffer from leftovers 
	}

	//count indexies for vlan_id slots to store ptrs
	int sum = 0;
	for(int j = 2; j <= MAX_VLAN_ID; j++){
		sum += vlan_amounts[j - 1];
		vlan_start_index[j] += sum;
	}
	
	//fill in the vlan_mac_sorted with ptrs
	for(int k = 0; k < *entries_num; k++){
		int vlan_id = vlan_mac_storage[k].vlan_id;
		int vlan_id_amount = vlan_amounts[vlan_id];
		int vlan_id_index = vlan_start_index[vlan_id] + vlan_id_amount - 1;
		vlan_mac_sorted[vlan_id_index] = &vlan_mac_storage[k];
		vlan_amounts[vlan_id]--;
	}

	return 0;
}
