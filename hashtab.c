/**
 * HashTab: a simple but effective hash table implementation.
 * @author  Marco Gunnink <marco@kninnug.nl>
 * @date    2015-11-18
 * @version 3.0.0
 * @file    hashtab.c
 *
 * See README.md for more info.
 *
 * License MIT:
 *
 * Copyright (c) 2015 Marco Gunnink
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * The software is provided "as is", without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and noninfringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising from,
 * out of or in connection with the software or the use or other dealings in
 * the software.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "hashtab.h"

#pragma GCC diagnostic ignored "-Wformat"

#ifdef HAVE_UTIL_H
#include HAVE_UTIL_H
#else

#ifndef LLEXPORT
#define LLEXPORT
#endif

#ifdef HASHTAB_NO_EXPORT_LL
#undef LLEXPORT
#define LLEXPORT static
typedef struct hashtab_linklist linklist_s;
#endif

static void * safeMalloc(size_t n){
	void * p = malloc(n);
	if(!p){
		fprintf(stderr, "malloc(%u) failed\n", n);
		exit(1);
	}
	return p;
}

static void * safeRealloc(void * p, size_t n){
	void * np = realloc(p, n);
	if(!np){
		fprintf(stderr, "realloc(%p, %u) failed\n", p, n);
		exit(1);
	}
	return np;
}

#endif

/*
 * Start linklist functions.
 */

LLEXPORT linklist_s * linklist_make(void * item, linklist_s * next){
	linklist_s * ret = safeMalloc(sizeof *ret);
	ret->item = item;
	ret->next = next;
	
	return ret;
}

LLEXPORT linklist_s * linklist_add(linklist_s * ll, void * item){
	if(ll->item == NULL){
		ll->item = item;
		
		return ll;
	}
	
	return linklist_make(item, ll);
}

LLEXPORT linklist_s * linklist_find(linklist_s * ll, const void * item, 
		int (*cmp)(const void * needle, const void * hay)){
	while(ll){
		if(cmp(item, ll->item) == 0){
			return ll;
		}
		
		ll = ll->next;
	}
	
	return NULL;
}

LLEXPORT void linklist_forEach(linklist_s * ll, 
		void (*callback)(void * item, void * ctx), void * ctx){
	while(ll){
		callback(ll->item, ctx);
		ll = ll->next;
	}
}

LLEXPORT linklist_s * linklist_remove(linklist_s * ll, const void * item, 
		int (*cmp)(const void * a, const void * b), void ** ret){
	linklist_s * next;
	if(!ll){
		return NULL;
	}
	
	if(cmp(item, ll->item) == 0){
		if(ret){
			*ret = ll->item;
		}
		
		next = ll->next;
		free(ll);
		return next;
	}
	
	ll->next = linklist_remove(ll->next, item, cmp, ret);
	return ll;
}

LLEXPORT void linklist_free(linklist_s * ll, void (*cb)(void * item, void * ctx),
		void * ctx){
	if(ll){
		linklist_free(ll->next, cb, ctx);
		if(cb){
			cb(ll->item, ctx);
		}
		free(ll);
	}
}

LLEXPORT linklist_s * linklist_copy(const linklist_s * src, void * (cpy)(const
		void * item, void * ctx), void * ctx){
	linklist_s * ret = safeMalloc(sizeof *ret);
	
	if(src->next){
		ret->next = linklist_copy(src->next, cpy, ctx);
	}else{
		ret->next = NULL;
	}
	
	if(cpy){
		ret->item = cpy(src->item, ctx);
	}else{
		ret->item = src->item;
	}
	
	return ret;
}

/* Print a linklist, using callback to print each item itself. */
LLEXPORT void linklist_print(linklist_s * ll, void (*callback)(const void * item)){
	while(ll){
		callback(ll->item);
		
		if(ll->next){
			printf(" -> ");
		}
		
		ll = ll->next;
	}
}

/*
 * Start hashtab functions.
 */
 
size_t hashtab_length(hashtab_s * ht){
	return ht->length + (ht->other ? hashtab_length(ht->other) : 0);
}

/**
 * @static
 *
 * Adds the provided link to the hash table. The next pointer in ll will be
 * overwritten, regardless of its original content. Updates ht->first as well.
 *
 * @param ht The hash table.
 * @param ll The link.
 * @return The hash of the item added.
 */
static size_t hashtab_addLink(hashtab_s * ht, linklist_s * link){
	size_t hash = ht->hasher(link->item) % ht->size;
	
	link->next = NULL;
	if(ht->data[hash] != NULL){
		link->next = ht->data[hash];
	}
	
	ht->data[hash] = link;
	++ht->length;
	
	if(ht->first > hash){
		ht->first = hash;
	}
	
	return hash;
}

/**
 * @static
 *
 * Adds all the links in the linklist to the hash table.
 *
 * @param ht The hash table.
 * @param link The linklist.
 * @return The number of items added.
 */
static size_t hashtab_linkAdd(hashtab_s * ht, linklist_s * ll){
	size_t ret = 0;
	linklist_s * next;
	
	while(ll){
		next = ll->next;
		
		hashtab_addLink(ht, ll);
		
		ll = next;
		++ret;
	}
	
	return ret;
}

/**
 * @static
 *
 * Finds the first non-empty (non-NULL) slot (bucket) in the hash table.
 *
 * @param ht The hash table.
 * @param i The starting position. If not 0 the caller must make sure there are
 *        no filled slots before this.
 * @return The updated ht->first.
 */
static size_t hashtab_findFirst(hashtab_s * ht, size_t i){
	for(; i < ht->size && ht->data[i] == NULL; ++i);
	
	return ht->first = i;
}

/**
 * @static
 * 
 * Migrates exisiting items to the other table. Called when growing.
 *
 * @param ht The hash table.
 */
static void hashtab_moveOver(hashtab_s * ht){
	size_t i, moved;
	linklist_s * link;
	
	for(i = 0; i < ht->size / ht->moveR && ht->length > 0; ++i){
		link = ht->data[ht->first];
		ht->data[ht->first] = NULL;
		moved = hashtab_linkAdd(ht->other, link);
		ht->length -= moved;
	
		hashtab_findFirst(ht, ht->first);
	}
	
	/* Merge the other table back into this one. */
	if(ht->length == 0){
		free(ht->data);
		ht->data = ht->other->data;
		ht->size = ht->other->size;
		ht->length = ht->other->length;
		ht->first = ht->other->first;
		
		free(ht->other);
		ht->other = NULL;
	}
}

/**
 * @static 
 *
 * Re-hashes and moves all the links in the given linklist with the new size to
 * the new data store.
 *
 * @param ll The linklist.
 * @param ht The hash table.
 * @param newData The new data store (bucket list).
 * @param newSize The size of the new data store.
 * @return The smallest non-empty index.
 */
static size_t hashtab_rehashLink(linklist_s * ll, hashtab_s * ht, 
		linklist_s ** newData, size_t newSize){
	size_t hash, first = newSize;
	linklist_s * next;
	
	while(ll){
		next = ll->next;
		ll->next = NULL;
		
		hash = ht->hasher(ll->item) % newSize;
		if(newData[hash] != NULL){
			ll->next = newData[hash];
		}
		newData[hash] = ll;
		
		if(hash < first){
			first = hash;
		}
		
		ll = next;
	}
	
	return first;
}

/**
 * @static
 *
 * Re-hashes the entire table after either growing or shrinking. This re-hashing
 * is done inline, so some items may be re-hashed (at most) twice. Also safeReallocs
 * the data store of the table either before or after.
 *
 * @param ht The hash table.
 * @param newSize The new size of the table.
 */
static void hashtab_rehash(hashtab_s * ht, size_t newSize){
	size_t i, tfirst, first = newSize;
	linklist_s * tdata;
	
	if(newSize > ht->size){ /* growth */
		ht->data = safeRealloc(ht->data, newSize * sizeof *ht->data);
		for(i = ht->size; i < newSize; i++){
			ht->data[i] = NULL;
		}
	}
	
	for(i = 0; i < ht->size; ++i){
		if(ht->data[i] == NULL){
			continue;
		}
		
		tdata = ht->data[i];
		ht->data[i] = NULL;
		tfirst = hashtab_rehashLink(tdata, ht, ht->data, newSize);
		if(tfirst < first){
			first = tfirst;
		}
	}
	ht->first = first;
	
	if(newSize < ht->size){ /* shrinkage */
		ht->data = safeRealloc(ht->data, ht->size * sizeof *ht->data);
	}
	
	ht->size = newSize;
}

/**
 * @static
 *
 * Grows the table by either re-hashing (see above) or allocating a new table
 * for incremental resizing. Growing always happens by doubling the size.
 *
 * @param ht The hash table.
 */
static void hashtab_grow(hashtab_s * ht){
	/* special case: no incremental resizing, but a complete rehash. */
	if(ht->moveR == 1){
		hashtab_rehash(ht, ht->size * 2);
	}else{
		ht->other = hashtab_make(ht->size * 2, ht->hasher, ht->cmp, 
				ht->threshold, ht->moveR, ht->shrink);
	}
	
	++ht->grows;
}

/**
 * @static
 *
 * Finds the link containing item.
 *
 * @param ht The hash table.
 * @param item The item to find.
 * @return The link containing the item, or NULL if it's not in the table.
 */
static linklist_s * hashtab_findLink(hashtab_s * ht, const void * item){
	size_t hash = ht->hasher(item) % ht->size;
	
	linklist_s * datum = linklist_find(ht->data[hash], item, ht->cmp);
	
	if(datum == NULL && ht->other){
		return hashtab_findLink(ht->other, item);
	}
	
	return datum;
}

hashtab_s * hashtab_make(size_t size, 
		size_t (*hasher)(const void * item), 
		int (*cmp)(const void * a, const void * b), float threshold, 
		size_t moveR, int shrink){
	hashtab_s * ret = safeMalloc(sizeof *ret);
	ret->size = size;
	ret->length = 0;
	
	ret->hasher = hasher;
	ret->cmp = cmp;
	ret->threshold = threshold;
	ret->moveR = moveR;
	ret->shrink = (shrink ? 1 : 0) * size;
	
	ret->first = ret->size;
	
	ret->grows = 0;
	ret->shrinks = 0;
	
	ret->data = safeMalloc(size * sizeof *(ret->data));
	ret->other = NULL;
	
	for(size_t i = 0; i < size; i++){
		ret->data[i] = NULL;
	}
	
	return ret;
}

size_t hashtab_add(hashtab_s * ht, void * item){
	if(!ht->other && hashtab_load(ht) > ht->threshold){
		hashtab_grow(ht);
	}
	
	if(ht->other != NULL){
		hashtab_add(ht->other, item);
		
		hashtab_moveOver(ht);
	}else{
		hashtab_addLink(ht, linklist_make(item, NULL));
	}
	
	return ht->length;
}

void * hashtab_find(hashtab_s * ht, const void * item){
	linklist_s * link = hashtab_findLink(ht, item);
	
	return link ? link->item : NULL;
}

void * hashtab_insert(hashtab_s * ht, void * item){
	linklist_s * link = hashtab_findLink(ht, item);
	void * ret = NULL;
	
	if(link){
		ret = link->item;
		link->item = item;
	}else{
		hashtab_add(ht, item);
	}
	
	return ret;
}

void hashtab_forEach(hashtab_s * ht, 
		void (*callback)(void * item, void * ctx), void * ctx){
	size_t i;
	
	for(i = 0; i < ht->size; ++i){
		if(ht->data[i]){ /* not strictly necessary, but avoids useless calls */
			linklist_forEach(ht->data[i], callback, ctx);
		}
	}
	
	if(ht->other){
		hashtab_forEach(ht->other, callback, ctx);
	}
}

void * hashtab_remove(hashtab_s * ht, const void * item){
	void * ret = NULL;
	size_t hash = ht->hasher(item) % ht->size;
	
	ht->data[hash] = linklist_remove(ht->data[hash], item, ht->cmp, &ret);
	if(ht->other){
		if(ret == NULL){
			ret = hashtab_remove(ht->other, item);
		}else{
			--ht->length;
		}
		
		hashtab_moveOver(ht);
	}else if(ret != NULL){
		--ht->length;
		
		if(hash == ht->first){
			hashtab_findFirst(ht, hash);
		}
		
		if(ht->shrink && ht->size > ht->shrink &&
				hashtab_load(ht) < 1.0f - ht->threshold){
			hashtab_rehash(ht, (ht->size / 2 > ht->shrink ? 
					ht->size / 2 : ht->shrink));
			++ht->shrinks;
		}
	}
	
	return ret;
}

hashtab_s * hashtab_copy(const hashtab_s * src, void * (cpy)(const void
		* item, void * ctx), void * ctx){
	hashtab_s * ret = safeMalloc(sizeof *ret);
	size_t i;
	
	memcpy(ret, src, sizeof *ret);
	
	ret->data = safeMalloc(ret->size * sizeof *ret->data);
	
	for(i = 0; i < ret->size; i++){
		if(src->data[i]){
			ret->data[i] = linklist_copy(src->data[i], cpy, ctx);
		}else{
			ret->data[i] = NULL;
		}
	}
	
	if(src->other){
		ret->other = hashtab_copy(src->other, cpy, ctx);
	}
	
	return ret;
}

void hashtab_free(hashtab_s * ht, void (*cb)(void * item, void * ctx),
		void * ctx){
	size_t i;
	
	if(ht->other){
		hashtab_free(ht->other, cb, ctx);
	}
	
	for(i = 0; i < ht->size; ++i){
		linklist_free(ht->data[i], cb, ctx);
	}
	
	free(ht->data);
	free(ht);
}

/* Print meta-data about a hash table. */
void hashtab_printHead(hashtab_s * ht, int other){
	printf("size:    %u\n"
		   "length:  %u\n"
		   "load:    %f\n"
		   "thresh:  %f\n"
		   "first:   %u\n"
		   "grows:   %u\n"
		   "shrinks: %u\n"
		   "moveR:   %u\n"
		   "other:   %s\n\n", ht->size, ht->length, hashtab_load(ht), 
				ht->threshold, ht->first, ht->grows, ht->shrinks, 
				ht->moveR, (ht->other ? "yes" : "no"));
	if(other && ht->other){
		hashtab_printHead(ht->other, other);
	}
}

/* Print the hash table using callback to print each item itself. */
void hashtab_print(hashtab_s * ht, void (*callback)(const void * item)){
	size_t i;
	
	hashtab_printHead(ht, 0);
	
	for(i = 0; i < ht->size; ++i){
		printf("	%u: ", i);
		linklist_print(ht->data[i], callback);
		putchar('\n');
	}
	
	if(ht->other){
		hashtab_print(ht->other, callback);
	}
}
