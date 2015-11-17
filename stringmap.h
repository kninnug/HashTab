/** ***************************************************************************
 * @file stringmap.h
 *
 * A simple wrapper around HashTab to create string -> item mappings. Usually,
 * you want to use this instead of HashTab directly. The operations are mostly
 * the same as those for HashTab, except that the callbacks are already
 * provided.
 *
 * Quick start:
 * @code{.c}
 *
 * // Make a stringmap
 * hashtab_s * ht = stringmap_make(8, 0.75, 4, 0); // note: returns a hashtab
 * // Add key-value. Note: both they key and value must remain allocated as long
 * // as they are in the stringmap.
 * stringmap_add(ht, "one", "alpha");
 * // Find item
 * char * found = stringmap_find(ht, "one");
 * if(found){ // != NULL
 *     printf("one -> %s\n", found);
 * }else{
 *     printf("not found\n");
 * }
 * // Remove item (optional)
 * stringmap_remove(ht, "one");
 * // Clean up stringmap. Also removes any items left in the table
 * stringmap_free(ht, NULL, NULL); // no callback for freeing items necessary
 *
 * @endcode
 **************************************************************************** */

#ifndef STRINGMAP_H
#define STRINGMAP_H

/**
 * The default hash-function is ELFHash (from GeneralHashFunctions). Re-#define
 * STRINGMAP_HASH before #include-ing this header to use a different one.
 */
#ifndef STRINGMAP_HASH
#	define STRINGMAP_HASH ELFHash
#endif

#include <stdlib.h>
#include <string.h>

struct stringmap;
typedef struct stringmap stringmap_s;

struct stringmap{
	const char * key;
	void * item;
};

struct stringmap__cb{
	void (*cb)(const char * key, void * item, void * ctx);
	void * ctx;
};

/**
 * @private
 *
 * The hash-callback.
 */
static size_t stringmap__hash(const void * v){
	const stringmap_s * sm = v;
	return STRINGMAP_HASH(sm->key, strlen(sm->key));
}

/**
 * @private
 *
 * The compare-callback.
 */
static int stringmap__cmp(const void * va, const void * vb){
	const stringmap_s * a = va;
	const stringmap_s * b = vb;
	
	return strcmp(a->key, b->key);
}

/**
 * @private
 *
 * The free-callback.
 */
static void stringmap__free(void * v, void * vctx){
	stringmap_s * sm = v;
	struct stringmap__cb * ctx = vctx;
	
	if(ctx->cb){
		ctx->cb(sm->key, sm->item, ctx->ctx);
	}
	
	free(v);
}

/**
 * @private
 *
 * ForEach-callback.
 */
static void stringmap__forEach(void * v, void * vctx){
	stringmap_s * sm = v;
	struct stringmap__cb * ctx = vctx;
	
	ctx->cb(sm->key, sm->item, ctx->ctx);
}

/**
 * @private
 *
 * Allocs and inits a stringmap_s.
 */
static inline stringmap_s * stringmap__mk(const char * key, void * item){
	stringmap_s * ret = malloc(sizeof *ret);
	ret->key = key;
	ret->item = item;
	return ret;
}

/**
 * Makes a HashTab for use by the stringmap functions. The parameters are the
 * same as those for hashtab_make. Callbacks are provided by stringmap.
 *
 * @param sz The initial size.
 * @param treshold The load threshold for resizement.
 * @param moveR The move-rate.
 * @param shrink Whether to shrink.
 * @return A hashtab_s for use by the stringmap functions.
 */
static inline hashtab_s * stringmap_make(size_t sz, float treshold, size_t moveR, size_t shrink){
	return hashtab_make(sz, stringmap__hash, stringmap__cmp, treshold, moveR, shrink);
}

/**
 * Add an item to the stringmap.
 *
 * @param ht The hashtab/stringmap to add to.
 * @param key The key.
 * @param item The item.
 * @return A stringmap_s item containing the key/value pair.
 */
static inline stringmap_s * stringmap_add(hashtab_s * ht, const char * key, void * item){
	stringmap_s * ret = stringmap__mk(key, item);
	
	hashtab_add(ht, ret);
	
	return ret;
}

/**
 * Insert an item to the stringmap. If an item with the same key already exists
 * it is replaced by the new value.
 *
 * @param ht The hashtab/stringmap to add to.
 * @param key The key.
 * @param item The item.
 * @return If an item with the same key existed, its key/item pair is returned,
 *         otherwise NULL.
 */
static inline stringmap_s * stringmap_insert(hashtab_s * ht, const char * key, void * item){
	stringmap_s * ret = stringmap__mk(key, item);
	
	stringmap_s * found = hashtab_insert(ht, ret);
	
	return found;
}

/**
 * Find an item by its key in the stringmap.
 *
 * @param ht The hashtab/stringmap to search in.
 * @param key The key to search for.
 * @return The corresponding item, or NULL if there is none with that key.
 */
static inline void * stringmap_find(hashtab_s * ht, const char * key){
	stringmap_s find = {key, NULL};
	stringmap_s * found = hashtab_find(ht, &find);
	
	if(found){
		return found->item;
	}
	
	return NULL;
}

/**
 * Remove an item from the stringmap.
 *
 * @param ht The hashtab/stringmap to remove from.
 * @param key The key to remove.
 * @return The item that's been removed, or NULL if there is none with that key.
 */
static inline void * stringmap_remove(hashtab_s * ht, const char * key){
	stringmap_s find = {key, NULL};
	stringmap_s * found = hashtab_remove(ht, &find);
	void * ret = NULL;
	
	if(found){
		ret = found->item;
		free(found);
	}
	
	return ret;
}

/**
 * Call a function for every item in the stringmap.
 *
 * @param ht The hashtab/stringmap to iterate over.
 * @param cb The callback to invoke.
 * @param ctx An additional context-pointer for the callback.
 */
static inline void stringmap_forEach(hashtab_s * ht, void (*cb)(const char * key,
		void * item, void * ctx), void * ctx){
	struct stringmap__cb feCtx = {cb, ctx};
	
	hashtab_forEach(ht, stringmap__forEach, &feCtx);
}

/**
 * Free the stringmap.
 *
 * @param ht The hashtab/stringmap to free.
 * @param cb A callback to free the items left in the stringmap. May be NULL if
 *        no freeing is necessary.
 * @param ctx An additional context-pointer for the callback.
 */
static inline void stringmap_free(hashtab_s * ht, void (*cb)(const char * key,
		void * item, void * ctx), void * ctx){
	struct stringmap__cb frCtx = {cb, ctx};
	
	hashtab_free(ht, stringmap__free, &frCtx);
}

#endif
