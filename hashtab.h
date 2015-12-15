/**
 * HashTab: a simple but effective hash table implementation.
 * @author  Marco Gunnink <marco@kninnug.nl>
 * @date    2015-11-18
 * @version 3.0.0
 * @file    hashtab.h
 *
 * See README.md for a quick start and more general comments.
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

#ifndef HASHTAB_H
#define HASHTAB_H

#include <stdlib.h>

#ifndef HASHTAB_NO_EXPORT_LL

/** A simple singly-linked list. */
typedef struct linklist{
	/** The stored item. */
	void * item;
	/** The next link (or NULL). */
	struct linklist * next;
} linklist_s;

#endif /* HASHTAB_NO_EXPORT_LL */

/** A simple but effective hash table. */
typedef struct hashtab{
	/** The number of items. */
	size_t length;
	/** The size. */
	size_t size;
	
	/** @private The first non-empty slot. */
	size_t first;
	
	/** The hash function for items. */
	size_t (*hasher)(const void * item);
	/** The compare function for items. */
	int (*cmp)(const void * a, const void * b);
	/** The load factor threshold. */
	float threshold;
	/** The move rate. */
	size_t moveR;
	/** Whether the table shrinks. */
	size_t shrink;
	
	/** The number of times the table grew. */
	size_t grows;
	/** The number of times the table shrunk. */
	size_t shrinks;
	
	/** @private The items. */
#ifdef HASHTAB_NO_EXPORT_LL
	struct hashtab_linklist{
		void * item;
		struct hashtab_linklist * next;
	} ** data;
#else
	linklist_s ** data;
#endif
	
	/** @private When migrating: the next table, otherwise NULL. */
	struct hashtab * other;
} hashtab_s;

/**
 * The size of the hash table.
 *
 * @param ht The hash table.
 * @return The size.
 */
#define hashtab_size(ht) (ht->size)

/**
 * The number of items in the hash table.
 *
 * @param ht The hash table.
 * @return The number of items.
 */
size_t hashtab_length(hashtab_s * ht);

/**
 * The load factor of the hash table: length / size.
 *
 * @param ht The hash table.
 * @return The load factor.
 */
#define hashtab_load(ht) ((float)hashtab_length(ht) / (float)hashtab_size(ht))

/**
 * How often the hash table has grown.
 *
 * @param ht The hash table.
 * @return The number of grows.
 */
#define hashtab_grows(ht) (ht->grows)

/**
 * How often the hash table has shrunk.
 *
 * @param ht The hash table.
 * @return The number of shrinks.
 */
#define hashtab_shrinks(ht) (ht->shrinks)

#ifndef HASHTAB_NO_EXPORT_LL

/**
 * Allocate a linklist and fill it with an item and point it to the next.
 *
 * @param item The item.
 * @param next The next node, or NULL.
 * @return The new link.
 */
linklist_s * linklist_make(void * item, linklist_s * next);

/**
 * Add the supplied item to the linklist. If ll has no item it will be stored
 * there, else a new link will be made, item will be stored there and it's
 * next will point to ll.
 *
 * @param ll The linklist to add to.
 * @param item The item.
 * @return Either ll (with ll->item == item) or a new link, that points to ll.
 */
linklist_s * linklist_add(linklist_s * ll, void * item);

/**
 * Find the link that contains item.
 *
 * @param ll The linklist to start with.
 * @param item The item to find.
 * @param cmp A function that gets item as its first argument, and a candidate
 *        as the second. It must return 0 if they are considered equal.
 * @return The link containing item, or NULL if not found.
 */
linklist_s * linklist_find(linklist_s * ll, const void * item, 
		int (*cmp)(const void * needle, const void * hay));

/**
 * Apply a function for each item in a linklist.
 * 
 * @param ll The linklist.
 * @param callback The function. The first argument is the item in the list,
 *        the second is ctx.
 * @param ctx A context also supplied to the callback.
 */
void linklist_forEach(linklist_s * ll, void (*callback)(void * item, void * ctx), 
		void * ctx);

/**
 * Remove the link containing item.
 *
 * @param ll The linklist.
 * @param item The item.
 * @param cmp A function that gets item as its first argument, and a candidate
 *        as the second. It must return 0 if they are considered equal.
 * @param [out] ret The removed item will be placed here, if not NULL.
 * @return The remaining list (possibly NULL).
 */
linklist_s * linklist_remove(linklist_s * ll, const void * item, 
		int (*cmp)(const void * a, const void * b), void ** ret);

/**
 * Makes a copy of the linklist. The cpy-callback is used to make copies of the
 * items. If cpy is NULL the original pointers are copied.
 *
 * @param src The original linklist.
 * @param cpy The callback to copy items with.
 * @param ctx A context-pointer for the callback.
 * @return A copy of the linklist.
 */
linklist_s * linklist_copy(const linklist_s * src, void * (*cpy)(const
		void * item, void * ctx), void * ctx);

/**
 * Free the linklist.
 *
 * @param ll The linklist.
 * @param cb A callback to free the items left in the linklist. May be NULL,
 *        if the items need no freeing.
 * @param ctx A context pointer for the callback.
 */
void linklist_free(linklist_s * ll, void (*cb)(void * item, void * ctx),
		void * ctx);

/**
 * (Debug function, replacable by forEach)
 * 
 * Prints all the links in a linklist, separated by '->'s.
 * 
 * @param ll The linklist.
 * @param callback The routine to print items with.
 */
void linklist_print(linklist_s * ll, void (*callback)(const void * item));

#endif /* HASHTAB_NO_EXPORT_LL */

/**
 * Allocate and initialize a new hash table.
 *
 * @param size The (initial) size.
 * @param hasher The hash function used for adding and finding items. It is fed
 *        one argument: an item, and must return its hash value. Hashes need not
 *        be perfect (unique).
 * @param cmp A function that gets item as its first argument, and a candidate
 *        as the second. It must return 0 if they are considered equal.
 * @param threshold The load threshold for enlarging the hash table.
 * @param moveR The move rate: when migrating to a new (bigger) hash table 
 *        size / moveR items are moved per add operation. Hence higher values
 *        move fewer items at a time.
 * @param shrink Whether the table should shrink when the table load goes below
 *        1 - threshold.
 * @return A new hash table.
 */
hashtab_s * hashtab_make(size_t size, 
		size_t (*hasher)(const void * item),
		int (*cmp)(const void * a, const void * b), float threshold, 
		size_t moveR, int shrink);

/**
 * Add an item to the hash table. Note that this means the item is added even if
 * it already exists in the table. To replace-or-add use hashtab_insert.
 *
 * @param ht The hash table.
 * @param item The item.
 * @return The new length of the table.
 */
size_t hashtab_add(hashtab_s * ht, void * item);

/**
 * Find an item in the hash table.
 *
 * @param ht The hash table.
 * @param item The item to find.
 * @return The item or NULL if it's not in the table.
 */
void * hashtab_find(hashtab_s * ht, const void * item);

/**
 * Insert an item into the hash table. If item isn't in the table yet it is
 * added, otherwise the existing item is replaced and returned.
 *
 * @param ht The hash table.
 * @param item The item to add.
 * @return The old item, or NULL if the new item was added.
 */
void * hashtab_insert(hashtab_s * ht, void * item);

/**
 * Apply a function to each item in the hash table.
 *
 * @param ht The hash table.
 * @param callback The function. The first argument is the item in the list,
 *        the second is ctx.
 * @param ctx A context also supplied to the callback.
 */
void hashtab_forEach(hashtab_s * ht, 
		void (*callback)(void * item, void * ctx), void * ctx);

/**
 * Remove an item from the hash table.
 *
 * @param ht The hash table.
 * @param item The item to remove.
 * @return The item.
 */
void * hashtab_remove(hashtab_s * ht, const void * item);

/**
 * Returns a copy of the hash table. The cpy-callback is called for every item
 * to make a copy of it. If cpy is NULL the item-pointers are copied shallowly.
 *
 * @param src The original hash table.
 * @param cpy The callback to make copies of the items.
 * @param ctx A context pointer for the callback.
 * @return A copy of the hash table.
 */
hashtab_s * hashtab_copy(const hashtab_s * src, void * (*cpy)(const void
		* item, void * ctx), void * ctx);

/**
 * Free the hash table.
 *
 * @param ht The hash table.
 * @param cb A callback to free the items left in the hash table. May be NULL,
 *        if the items need no freeing.
 * @param ctx A context pointer for the callback.
 */
void hashtab_free(hashtab_s * ht, void (*cb)(void * item, void * ctx),
		void * ctx);

/**
 * (Debug function, replacable by forEach)
 * 
 * Prints meta-data about a hash table.
 * 
 * @param ht The hash table.
 * @param other Whether to recurse for migrating tables (other) as well.
 */
void hashtab_printHead(hashtab_s * ht, int other);

/**
 * (Debug function, replacable by forEach)
 * 
 * Prints a hash table.
 * 
 * @param ht The hash table.
 * @param callback The routine to print items with.
 */
void hashtab_print(hashtab_s * ht, void (*callback)(const void * item));

void printMemStats();

#endif
