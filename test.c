/**
 * Test file for HashTab. See hashtab.{h,c} for detailed documentation.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hashtab.h"
#include "GeneralHashFunctions.h"

typedef struct KVtest{
	const char * key;
	int value;
} KVtest_t;

size_t KVhash(const void * v){
	const KVtest_t * t = v;
	/* Try playing around with other hash functions from GeneralHashFunctions.h */
	return ELFHash(t->key, strlen(t->key));
}

int KVcmp(const void * va, const void * vb){
	const KVtest_t * a = va;
	const KVtest_t * b = vb;
	
	return strcmp(a->key, b->key);
}

void KVprint(const void * v){
	const KVtest_t * a = v;
	printf("%s : %i", a->key, a->value);
}

void KVfree(void * v, void * ctx){
	free(v);
}

void * KVcopy(const void * v, void * ctx){
	const KVtest_t * src = v;
	KVtest_t * ret = malloc(sizeof *ret);
	ret->key = src->key;
	ret->value = src->value;
	
	return ret;
}

int main(void){
	size_t size = 8, moveR = 4, i;
	int shrink = 1;
	float threshold = 0.75;
	
	const char * testStrs[] = {"Alef", "Bet", "Gimel", "Dalet", "He", "Vav", 
			"Zayin", "Het", "Tet", "Yod", "Kaf", "Lamed", "Mem", "Nun", 
			"Samekh", "Ayin", "Pe", "Tsadi", "Qof", "Resh", "Shin", "Tav",
			
			"alpha", "beta", "gamma", "delta", "epsilon", "zdeta", "eta", 
			"theta", "iota", "kappa", "lambda", "mu", "nu", "xi", "omicron", 
			"pi", "rho", "sigma", "tau", "upsilon", "phi", "chi", "psi", "omega"};
	
	KVtest_t * tmp;
	
	srand(0);
	
	hashtab_t * ht = hashtab_make(size, KVhash, KVcmp, threshold, moveR, 
			shrink);
	hashtab_t * ht2;
	
	for(i = 0; i < sizeof testStrs / sizeof *testStrs; i++){
		tmp = malloc(sizeof *tmp);
		tmp->key = testStrs[i];
		tmp->value = rand();
		
		hashtab_add(ht, tmp);
	}
	
	hashtab_print(ht, KVprint);
	
	ht2 = hashtab_copy(ht, KVcopy, NULL);
	
	hashtab_print(ht2, KVprint);
	
	hashtab_forEach(ht, KVfree, NULL);
	hashtab_forEach(ht2, KVfree, NULL);
	
	hashtab_free(ht);
	hashtab_free(ht2);
	
	return 0;
}
