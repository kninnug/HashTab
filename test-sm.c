#include <stdlib.h>
#include <stdio.h>

#include "hashtab.h"
#include "GeneralHashFunctions.h"
#include "stringmap.h"

int main(){
	size_t size = 8, moveR = 4;
	int shrink = 1;
	float threshold = 0.75;
	
	const char * keys[] = {"Alef", "Bet", "Gimel", "Dalet", "He", "Vav", 
			"Zayin", "Het", "Tet", "Yod", "Kaf", "Lamed", "Mem", "Nun", 
			"Samekh", "Ayin", "Pe", "Tsadi", "Qof", "Resh", "Shin", "Tav"};
	size_t len = sizeof keys / sizeof *keys;
	
	int values[sizeof keys / sizeof *keys] = {0};
	
	srand(0);
	
	hashtab_s * ht = stringmap_make(size, threshold, moveR, shrink);
	
	for(size_t i = 0; i < len; i++){
		values[i] = rand() % 100;
		
		stringmap_add(ht, keys[i], values + i); // &values[i]
	}
	
	printf("Find key (empty line to quit): "); fflush(stdout);
	char key[128] = {0};
	while(fgets(key, sizeof key, stdin) && key[0] != '\n'){
		key[strlen(key) - 1] = 0; /* remove '\n' */
		
		int * found = stringmap_find(ht, key);
		
		if(found){
			printf("Found: %s = %i\n", key, *found);
		}else{
			printf("Not found: %s\n", key);
		}
		
		printf("Find key (empty line to quit): "); fflush(stdout);
	}
	
	stringmap_free(ht, NULL, NULL);
	
	return 0;
}
