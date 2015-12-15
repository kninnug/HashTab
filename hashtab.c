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

typedef hashtab__varray_s varray_s;

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
 * Start varray functions
 */
 
#ifndef VAPRIVATE
#	define VAPRIVATE static
#endif
 
#define varray_size(va) (va->size)
#define varray_length(va) (va->length)
#define varray_data(va) (va->data)
#define varray_list(va) varray_data(va)

VAPRIVATE varray_s * varray_init(varray_s * va, size_t sz){
	if(!sz){
		sz = 2;
	}
	
	va->size = sz;
	va->length = 0;
	va->data = safeMalloc(va->size * sizeof *va->data);
	
	return va;
}

VAPRIVATE varray_s * varray_make(size_t sz){
	varray_s * ret = safeMalloc(sizeof *ret);
	
	return varray_init(ret, sz);
}

VAPRIVATE varray_s * varray_copy(varray_s * src, void * (*cpy)(const void * item,
		void * ctx), void * ctx){
	varray_s * ret = varray_make(src->size);
	size_t i;
	
	ret->length = src->length;
	
	for(i = 0; i < ret->length; i++){
		ret->data[i] = cpy(src->data[i], ctx);
	}
	
	return ret;
}

VAPRIVATE void varray_free(varray_s * va, void (*cb)(void * item, void * ctx),
		void * ctx){
	size_t i;
	
	if(cb){
		for(i = 0; i < va->length; i++){
			cb(va->data[i], ctx);
		}
	}
	
	free(va->data);
	free(va);
}

static inline varray_s * varray_grow(varray_s * va){
	if(va->size > va->length){
		return va;
	}
	
	va->size = va->size * 2 + 1;
	va->data = safeRealloc(va->data, va->size * sizeof *va->data);
	
	return va;
}

VAPRIVATE varray_s * varray_add(varray_s * va, void * item){
	varray_grow(va);
	
	va->data[va->length++] = item;
	
	return va;
}

VAPRIVATE void * varray_insert(varray_s * va, void * item,
		int (*cmp)(const void * a, const void * b)){
	size_t i;
	void * ret = NULL;
	
	for(i = 0; i < va->length; i++){
		if(!cmp(item, va->data[i])){
			ret = va->data[i];
			va->data[i] = item;
		}
	}
	
	if(!ret){
		varray_add(va, item);
	}
	
	return ret;
}

VAPRIVATE size_t varray_findIdx(varray_s * va, const void * item, 
		int (*cmp)(const void * a, const void * b)){
	size_t i;
	
	for(i = 0; i < va->length; i++){
		if(!cmp(item, va->data[i])){
			return i;
		}
	}
	
	return va->size;
}

VAPRIVATE void * varray_find(varray_s * va, const void * item, 
		int (*cmp)(const void * a, const void * b)){
	size_t idx = varray_findIdx(va, item, cmp);
	
	if(idx == va->size){
		return NULL;
	}
	
	return va->data[idx];
}

VAPRIVATE void * varray_remove(varray_s * va, const void * item, 
		int (*cmp)(const void * a, const void * b)){
	size_t idx = varray_findIdx(va, item, cmp);
	void * ret = NULL;
	
	if(idx == va->size){
		return NULL;
	}
	
	ret = va->data[idx];
	va->length--;
	if(idx < va->length){ // move last item into idx
		va->data[idx] = va->data[va->length];
	}
	va->data[va->length] = NULL;
	
	return ret;
}

VAPRIVATE void varray_forEach(varray_s * va, void (*cb)(void * item, void * ctx),
		void * ctx){
	size_t i;
	for(i = 0; i < va->length; i++){
		cb(va->data[i], ctx);
	}
}

VAPRIVATE void varray_print(varray_s * va, void (*cb)(const void * item)){
	size_t i;
	
	if(!va->length){
		return;
	}
	
	cb(va->data[0]);
	
	for(i = 1; i < va->length; i++){
		printf(", ");
		cb(va->data[i]);
	}
}

/*
 * Start hashtab functions.
 */
 
size_t hashtab_length(hashtab_s * ht){
	return ht->length + (ht->other ? hashtab_length(ht->other) : 0);
}

static hashtab_s * hashtab_addItem(hashtab_s * ht, void * item){
	size_t hash = ht->hasher(item) % ht->size;
	
	if(!ht->data[hash]){
		ht->data[hash] = varray_make(1);
	}
	varray_add(ht->data[hash], item);
	
	++ht->length;
	
	if(ht->first > hash){
		ht->first = hash;
	}
	
	return ht;
}

static size_t hashtab_addVarray(hashtab_s * ht, varray_s * va){
	size_t i;
	for(i = 0; i < varray_length(va); i++){
		hashtab_addItem(ht, varray_data(va)[i]);
	}
	return varray_length(va);
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
static size_t hashtab_findFirst(hashtab_s * ht, size_t i){
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
static void hashtab_moveOver(hashtab_s * ht){
	size_t i, moved;
	varray_s * va;
	
	for(i = 0; i < ht->size / ht->moveR && ht->length > 0; ++i){
		va = ht->data[ht->first];
		ht->data[ht->first] = NULL;
		moved = hashtab_addVarray(ht->other, va);
		varray_free(va, NULL, NULL);
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

static size_t hashtab_rehashVarray(varray_s * va, hashtab_s * ht,
		varray_s ** newData, size_t newSize){
	size_t i, first = newSize, hash;
	void * item;
	
	for(i = 0; i < varray_length(va); i++){
		item = varray_data(va)[i];
		hash = ht->hasher(item) % newSize;
		
		if(!newData[hash]){
			newData[hash] = varray_make(1);
		}
		
		varray_add(newData[hash], item);
		
		if(hash < first){
			first = hash;
		}
	}
	
	return first;
}

/**
 * @private
 *
 * Re-hashes the entire table after either growing or shrinking. This re-hashing
 * is done inline, so some items may be re-hashed (at most) twice. Also reallocs
 * the data store of the table either before or after.
 *
 * @param ht The hash table.
 * @param newSize The new size of the table.
 */
static void hashtab_rehash(hashtab_s * ht, size_t newSize){
	size_t i, tfirst, first = newSize;
	varray_s * tdata;
	
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
		tfirst = hashtab_rehashVarray(tdata, ht, ht->data, newSize);
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
		hashtab_addItem(ht, item);
	}
	
	return ht->length;
}

void * hashtab_find(hashtab_s * ht, const void * item){
	size_t hash = ht->hasher(item) % ht->size;
	
	if(!ht->data[hash]){
		return NULL;
	}
	
	return varray_find(ht->data[hash], item, ht->cmp);
}

void * hashtab_insert(hashtab_s * ht, void * item){
	size_t hash = ht->hasher(item) % ht->size;
	varray_s * va = ht->data[hash];
	void * ret = NULL;
	
	if(va){
		ret = varray_insert(va, item, ht->cmp);
	}else{
		hashtab_add(ht, item);
	}
	
	return ret;
}

void hashtab_forEach(hashtab_s * ht, void (*cb)(void * item, void * ctx),
		void * ctx){
	size_t i;
	
	for(i = 0; i < ht->size; ++i){
		if(ht->data[i]){
			varray_forEach(ht->data[i], cb, ctx);
		}
	}
	
	if(ht->other){
		hashtab_forEach(ht->other, cb, ctx);
	}
}

void * hashtab_remove(hashtab_s * ht, const void * item){
	void * ret = NULL;
	size_t hash = ht->hasher(item) % ht->size;
	
	if(ht->data[hash]){
		ret = varray_remove(ht->data[hash], item, ht->cmp);
	}
	
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
			ret->data[i] = varray_copy(src->data[i], cpy, ctx);
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
		if(ht->data[i]){
			varray_free(ht->data[i], cb, ctx);
		}
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
		if(ht->data[i]){
			varray_print(ht->data[i], callback);
		}
		putchar('\n');
	}
	
	if(ht->other){
		hashtab_print(ht->other, callback);
	}
}
