/**
 * HashTab: a simple but effective hash table implementation.
 * @author  Marco Gunnink <marco@kninnug.nl>
 * @date    2015-11-09
 * @version 2.2.0
 * @file    hashtab.c
 *
 * To use: copy hashtab.c & hashtab.h into your project. 
 *
 *     #include "hashtab.h"
 *
 * and compile with: 
 *
 *     cc myProgram.c hashtab.c
 *
 * For some example hash functions, copy GeneralHashFunctions.h & .c.
 *
 *     #include "GeneralHashFunctions.h"
 *
 * and compile with:
 *
 *     cc myProgram.c hashtab.c GeneralHashFunctions.c
 *
 * 1. Define callbacks for hashing and comparison.
 * 2. Allocate and initialise a hash table with hashtab_make.
 * 3. Add or insert items with hashtab_add or hashtab_insert.
 * 4. Retrieve items with hashtab_find.
 * 5. Clean up the hash table with hashtab_free.
 * 
 * The hash table uses (singly-)linked lists to avoid collisions and incremental
 * resizing to grow the table when its load factor exceeds a provided value.
 * When growing the table a new one is allocated, twice the size of the
 * original. Then on every add operation the new item is added to the other,
 * and a number of other items are moved to the new one as well. When the
 * original is empty the new one is merged back in. Items are moved after remove
 * operations as well. The number of items moved is (size / moveR), so higher
 * values of moveR move fewer items per operation and a value of 1 is equivalent
 * to rehashing the entire table immediately.
 * 
 * A table can also be told to shrink when it reaches the inverse of the
 * provided load factor threshold (1 - threshold). Shrinking is done by
 * rehashing the table, a relatively expensive operation, hence for most
 * applications not worth it. Since the idea behind shrinking tables is to
 * conserve space this is done inline. Consequently: some items may be rehashed
 * twice, but it avoids allocating a new table to rehash into: saving those
 * precious bytes.
 * 
 * Hash tables here have the following properties:
 * - size: The number of buckets in the table.
 * - length: The number of stored items in the table.
 * - Load factor (computed): length / size, how much of the table is filled.
 * - threshold: When the load factor exceeds this, the table is grown. If
 *   the table is configured to shrink this will happen when the load factor
 *   goes below (1 - threshold).
 * - hasher: The hash function used for adding & retrieving items.
 * - cmp: The compare function used to determine exact equality (since hashes
 *   may collide).
 * - moveR: The move rate (for lack of better wording). When growing the table
 *   at every add or remove operation (size / moveR) items are moved. So when
 *   this is 1 it becomes equivalent to growing & migrating the table in 1 go.
 * - shrink: When the table is configured to shrink this keeps the original
 *   size so the table is never shrunk below that. Otherwise it's 0.
 * - grows: The number of times the table has grown.
 * - shrinks: The number of times the table has shrunk.
 * - data: The buckets.
 * - other: When growing the table this is where the new (and old) items are
 *   moved to. Otherwise it's NULL.
 * 
 * The following operations are supported (see doc-comments for more info):
 * - make: Allocate a new hash table and fill it with the relevant information.
 * - free: De-allocate the hash table and its allocated members. Note that this
 *   does not free any items that may still be in it. To do this either remove
 *   the items individually or call forEach (see below) with an item-cleanup 
 *   function.
 * - add: Add a new item to the hash table. Note that this adds the item even if
 *   it already exists. To replace existing items use insert (see below).
 * - insert: Insert an item: if it can be found in the table replace it,
 *   otherwise add it.
 * - find: Find an item in the table.
 * - forEach: Iterate over all the items in the table with a callback function.
 * - remove: Remove an item from the hash table.
 * - copy: Make a shallow or deep copy.
 *
 * By default the hashtab_s and linklist_s types and related functions are 
 * exported, when compiled with HASHTAB_NO_EXPORT_LL defined it will not export
 * linklist_s or its related functions, though they will still be compiled for
 * internal use.
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

/* Inspired by the Minix source code. */
#ifndef PUBLIC
#define PUBLIC
#endif
#ifndef PRIVATE
#define PRIVATE static
#endif

#ifndef LLEXPORT
#define LLEXPORT
#endif

#ifdef HASHTAB_NO_EXPORT_LL
#undef LLEXPORT
#define LLEXPORT static
typedef struct hashtab_linklist linklist_s;
#endif

PRIVATE void * safeMalloc(size_t n){
	void * p = malloc(n);
	if(!p){
		fprintf(stderr, "malloc(%u) failed\n", n);
		exit(1);
	}
	return p;
}

PRIVATE void * safeRealloc(void * p, size_t n){
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

LLEXPORT void linklist_free(linklist_s * ll){
	if(ll){
		linklist_free(ll->next);
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
 
PUBLIC size_t hashtab_length(hashtab_s * ht){
	return ht->length + (ht->other ? hashtab_length(ht->other) : 0);
}

/**
 * @private
 *
 * Adds the provided link to the hash table. The next pointer in ll will be
 * overwritten, regardless of its original content. Updates ht->first as well.
 *
 * @param ht The hash table.
 * @param ll The link.
 * @return The hash of the item added.
 */
PRIVATE size_t hashtab_addLink(hashtab_s * ht, linklist_s * link){
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
 * @private
 *
 * Adds all the links in the linklist to the hash table.
 *
 * @param ht The hash table.
 * @param link The linklist.
 * @return The number of items added.
 */
PRIVATE size_t hashtab_linkAdd(hashtab_s * ht, linklist_s * ll){
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
 * @private
 *
 * Finds the first non-empty (non-NULL) slot (bucket) in the hash table.
 *
 * @param ht The hash table.
 * @param i The starting position. If not 0 the caller must make sure there are
 *        no filled slots before this.
 * @return The updated ht->first.
 */
PRIVATE size_t hashtab_findFirst(hashtab_s * ht, size_t i){
	for(; i < ht->size && ht->data[i] == NULL; ++i);
	
	return ht->first = i;
}

/**
 * @private
 * 
 * Migrates exisiting items to the other table. Called when growing.
 *
 * @param ht The hash table.
 */
PRIVATE void hashtab_moveOver(hashtab_s * ht){
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
 * @private 
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
PRIVATE size_t hashtab_rehashLink(linklist_s * ll, hashtab_s * ht, 
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
 * @private
 *
 * Re-hashes the entire table after either growing or shrinking. This re-hashing
 * is done inline, so some items may be re-hashed (at most) twice. Also safeReallocs
 * the data store of the table either before or after.
 *
 * @param ht The hash table.
 * @param newSize The new size of the table.
 */
PRIVATE void hashtab_rehash(hashtab_s * ht, size_t newSize){
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
 * @private
 *
 * Grows the table by either re-hashing (see above) or allocating a new table
 * for incremental resizing. Growing always happens by doubling the size.
 *
 * @param ht The hash table.
 */
PRIVATE void hashtab_grow(hashtab_s * ht){
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
 * @private
 *
 * Finds the link containing item.
 *
 * @param ht The hash table.
 * @param item The item to find.
 * @return The link containing the item, or NULL if it's not in the table.
 */
PRIVATE linklist_s * hashtab_findLink(hashtab_s * ht, const void * item){
	size_t hash = ht->hasher(item) % ht->size;
	
	linklist_s * datum = linklist_find(ht->data[hash], item, ht->cmp);
	
	if(datum == NULL && ht->other){
		return hashtab_findLink(ht->other, item);
	}
	
	return datum;
}

PUBLIC hashtab_s * hashtab_make(size_t size, 
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

PUBLIC size_t hashtab_add(hashtab_s * ht, void * item){
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

PUBLIC void * hashtab_find(hashtab_s * ht, const void * item){
	linklist_s * link = hashtab_findLink(ht, item);
	
	return link ? link->item : NULL;
}

PUBLIC void * hashtab_insert(hashtab_s * ht, void * item){
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

PUBLIC void hashtab_forEach(hashtab_s * ht, 
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

PUBLIC void * hashtab_remove(hashtab_s * ht, const void * item){
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

PUBLIC hashtab_s * hashtab_copy(const hashtab_s * src, void * (cpy)(const void
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

PUBLIC void hashtab_free(hashtab_s * ht){
	size_t i;
	
	if(ht->other){
		hashtab_free(ht->other);
	}
	
	for(i = 0; i < ht->size; ++i){
		linklist_free(ht->data[i]);
	}
	
	free(ht->data);
	free(ht);
}

/* Print meta-data about a hash table. */
PUBLIC void hashtab_printHead(hashtab_s * ht, int other){
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
PUBLIC void hashtab_print(hashtab_s * ht, void (*callback)(const void * item)){
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
