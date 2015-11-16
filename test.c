/**
 * Test file for HashTab. See hashtab.{h,c} for detailed documentation.
 *
 * This implements a map from strings to ints using HashTab.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "hashtab.h"
#include "GeneralHashFunctions.h"

/* Simple string-key to int-value mapping */
typedef struct KVtest{
	const char * key;
	int value;
} KVtest_s;

/* Hash callback */
size_t KVhash(const void * v){
	const KVtest_s * t = v;
	/* Try playing around with other hash functions from GeneralHashFunctions.h */
	return ELFHash(t->key, strlen(t->key));
}

/* Comparison callback */
int KVcmp(const void * va, const void * vb){
	const KVtest_s * a = va;
	const KVtest_s * b = vb;
	
	return strcmp(a->key, b->key);
}

/* Print callback */
void KVprint(const void * v){
	const KVtest_s * a = v;
	printf("%s = %i", a->key, a->value);
}

/* Free callback */
void KVfree(void * v, void * ctx){
	free(v);
}

/* Copy callback */
void * KVcopy(const void * v, void * ctx){
	const KVtest_s * src = v;
	KVtest_s * ret = malloc(sizeof *ret);
	ret->key = src->key;
	ret->value = src->value;
	
	return ret;
}

/* Helper function to find */
int findKV(hashtab_s * ht, const char * key){
	/* note that a (pointer to) full structure is needed because the hashtab
	   doesn't by itself know about the keys and values */
	KVtest_s find = {key, 0}; /* set key, value is ignored */
	KVtest_s * found = hashtab_find(ht, &find);
	
	if(found){
		return found->value;
	}
	
	return INT_MIN; /* return special value for not found */
}

int main(void){
	size_t size = 8, moveR = 4;
	int shrink = 1;
	float threshold = 0.75;
	
	/* keys */
	const char * testStrs[] = {"Alef", "Bet", "Gimel", "Dalet", "He", "Vav", 
			"Zayin", "Het", "Tet", "Yod", "Kaf", "Lamed", "Mem", "Nun", 
			"Samekh", "Ayin", "Pe", "Tsadi", "Qof", "Resh", "Shin", "Tav",
			
			"alpha", "beta", "gamma", "delta", "epsilon", "zdeta", "eta", 
			"theta", "iota", "kappa", "lambda", "mu", "nu", "xi", "omicron", 
			"pi", "rho", "sigma", "tau", "upsilon", "phi", "chi", "psi", "omega"};
	
	srand(0);
	
	/* Make hashtab */
	hashtab_s * ht = hashtab_make(size, KVhash, KVcmp, threshold, moveR, 
			shrink);
	
	/* Fill with random values */
	for(size_t i = 0; i < sizeof testStrs / sizeof *testStrs; i++){
		KVtest_s * tmp = malloc(sizeof *tmp);
		tmp->key = testStrs[i];
		tmp->value = rand() % 100;
		
		hashtab_add(ht, tmp);
	}
	
	/* Print the hashtab and its contents, use the print callback */
	hashtab_print(ht, KVprint);
	
	/* Find values using keys from user input */
	printf("Find key (empty line to quit): "); fflush(stdout);
	char key[128] = {0};
	while(fgets(key, sizeof key, stdin) && key[0] != '\n'){
		key[strlen(key) - 1] = 0; /* remove '\n' */
		
		int found = findKV(ht, key);
		
		if(found != INT_MIN){
			printf("Found: %s = %i\n", key, found);
		}else{
			printf("Not found: %s\n", key);
		}
		
		printf("Find key (empty line to quit): "); fflush(stdout);
	}
	
	/* Clean up values with the free callback and the hash table itself */
	hashtab_free(ht, KVfree, NULL);
	
	return 0;
}
