HashTab: a simple but effective hash table implementation.
==========================================================

Author:  Marco Gunnink <marco@kninnug.nl>  
Date:    2015-11-18  
Version: 3.0.0  

Quicker start:

If you only want to map strings (`const char *`) to 'items', use `stringmap.h`,
which is simpler. Copy `hashtab.h`, `hashtab.c` and `stringmap.h` into your
project (there is no `stringmap.c`). And compile along with your other files:

    cc myProgram.c hashtab.c

In `myProgram.c`:

    #include <stdio.h>
    #include "stringmap.h" // includes hashtab.h as well
    
    int main(){
        size_t size = 8, moveR = 4;
        int shrink = 0;
        float threshold = 0.75;
        
        // Make stringmap (a hashtab)
        hashtab_s * ht = stringmap_make(size, threshold, moveR, shrink);
        
        // Add items. Note that they must remain allocated as long as they are
        // in the hashtab/stringmap.
        stringmap_add(ht, "one", "alpha");
        stringmap_add(ht, "two", "beta");
        
        // Find item
        char * found = stringmap_find(ht, "one");
        if(found){ // != NULL
            printf("Found: %s", found);
        }else{
            printf("Not found");
        }
        
        // Remove item
        stringmap_remove(ht, "one");
        
        // Clean up. No callbacks for freeing the items here. If your items do
        // need additional cleanup, define a callback that accepts a
        // - const char * key
        // - void * item
        // - void * ctx
        // and pass it as the second argument. The third argument (`ctx) is 
        // passed to the callback for additional data.
        stringmap_free(ht, NULL, NULL);
        
        return 0;
    }

Quick start:

Copy `hashtab.h` & `hashtab.c` into your project. Include `hashtab.h` and
compile `hashtab.c` along with your other files:

    cc myProgram.c hashtab.c

In `myProgram.c`:

    #include <stdio.h>
    #include "hashtab.h"
    
    // Hash callback
    size_t intHash(const void * v){
        return *(const int*)v;
    }
    
    // Comparison callback
    int intCmp(const void * va, const void * vb){
        return *(const int*)va - *(const int*)vb;
    }
    
    int main(){
        size_t size = 8, moveR = 4;
        int shrink = 1;
        float threshold = 0.75;
        
        int items[] = {12, 34, 56, 78, 90};
        
        // Make hash table
        hashtab_s * ht = hashtab_make(size, intHash, intCmp, threshold, moveR,
            shrink);
        
        // Fill with items
        for(int i = 0; i < sizeof items / sizeof *items; i++){
            hashtab_add(ht, items + i); // &items[i]
        }
        
        // Find an item
        int * found = hashtab_find(ht, (int[]){34}); // compound literal for pointer to int of 34
        if(found){ // != NULL
            printf("Found: %i", *found);
        }else{
            printf("Not found");
        }
        
        // Clean up. No additional freeing necessary for the items
        hashtab_free(ht, NULL, NULL);
        
        return 0;
    }

In short:

 1. Define callbacks for hashing and (exact) comparison.
 2. Allocate and initialize a hash-table with `hashtab_make`.
 3. Fill it with `hashtab_add` and/or `hashtab_insert`.
 4. Find items with `hashtab_find`.
 5. Remove items with `hashtab_remove`.
 6. Clean up the hash-table with `hashtab_free`.
 
See the doc-comments in hashtab.h or generated documentation for more details.

The hash table uses (singly-)linked lists to avoid collisions and incremental
resizing to grow the table when its load factor exceeds a provided value.
When growing the table a new one is allocated, twice the size of the
original. Then on every add operation the new item is added to the other,
and a number of other items are moved to the new one as well. When the
original is empty the new one is merged back in. Items are moved after remove
operations as well. The number of items moved is (`size / moveR`), so higher
values of moveR move fewer items per operation and a value of 1 is equivalent
to rehashing the entire table immediately.

A table can also be told to shrink when it reaches the inverse of the
provided load factor threshold (`1 - threshold`). Shrinking is done by
rehashing the table, a relatively expensive operation, hence for most
applications not worth it. Since the idea behind shrinking tables is to
conserve space this is done inline. Consequently: some items may be rehashed
twice, but it avoids allocating a new table to rehash into: saving those
precious bytes.

Hash tables here have the following properties:

 - `size`: The number of buckets in the table.
 - `length`: The number of stored items in the table.
 - Load factor (computed): `length / size`, how much of the table is filled.
 - `threshold`: When the load factor exceeds this, the table is grown. If
  the table is configured to shrink this will happen when the load factor
  goes below (`1 - threshold`).
 - `hasher`: The hash function used for adding & retrieving items.
 - `cmp`: The compare function used to determine exact equality (since hashes
  may collide).
 - `moveR`: The move rate (for lack of better wording). When growing the table
  at every add or remove operation (size / moveR) items are moved. So when
  this is 1 it becomes equivalent to growing & migrating the table in 1 go.
 - `shrink`: When the table is configured to shrink this keeps the original
  size so the table is never shrunk below that. Otherwise it's 0.
 - `grows`: The number of times the table has grown.
 - `shrinks`: The number of times the table has shrunk.
 - `data`: The buckets.
 - `other`: When growing the table this is where the new (and old) items are
  moved to. Otherwise it's `NULL`.

The following operations are supported (see doc-comments for more info):

 - `make`: Allocate a new hash table and fill it with the relevant information.
 - `free`: De-allocate the hash table and its allocated members. Note that this
  does not free any items that may still be in it. To do this either remove
  the items individually or call forEach (see below) with an item-cleanup 
  function.
 - `add`: Add a new item to the hash table. Note that this adds the item even if
  it already exists. To replace existing items use insert (see below).
 - `insert`: Insert an item: if it can be found in the table replace it,
  otherwise add it.
 - `find`: Find an item in the table.
 - `forEach`: Iterate over all the items in the table with a callback function.
 - `remove`: Remove an item from the hash table.
 - `copy`: Make a shallow or deep copy.

By default the `hashtab_s` and `linklist_s` types and related functions are 
exported, when compiled with `HASHTAB_NO_EXPORT_LL` defined it will not export
linklist_s or its related functions, though they will still be compiled for
internal use.

License MIT:

Copyright (c) 2015 Marco Gunnink

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

The software is provided "as is", without warranty of any kind, express or
implied, including but not limited to the warranties of merchantability,
fitness for a particular purpose and noninfringement. In no event shall the
authors or copyright holders be liable for any claim, damages or other
liability, whether in an action of contract, tort or otherwise, arising from,
out of or in connection with the software or the use or other dealings in
the software.

[Example hash functions][ghc] collected by [Arash Partow][ap].

   [ghc]: http://www.partow.net/programming/hashfunctions/  "General Hash functions"
   [ap]: http://www.partow.net  "Arash Partow"
