HashTab: a simple but effective hash table implementation.
==========================================================

Author: Marco Gunnink <marco@kninnug.nl>
Date: 2015-11-09
Version 2.2.0

To use: copy hashtab.c & hashtab.h into your project. 

    #include "hashtab.h"

and compile with: 

    cc myProgram.c hashtab.c

For some example hash functions, copy GeneralHashFunctions.h & .c.

    #include "GeneralHashFunctions.h"

and compile with:

    cc myProgram.c hashtab.c GeneralHashFunctions.c

1. Define callbacks for hashing and comparison.
2. Allocate and initialise a hash table with `hashtab_make`.
3. Add or insert items with `hashtab_add` or `hashtab_insert`.
4. Retrieve items with `hashtab_find`.
5. Clean up the hash table with `hashtab_free`.

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

License: MIT (see LICENSE)

[Example hash functions][ghc] collected by [Arash Partow][ap].

   [ghc]: http://www.partow.net/programming/hashfunctions/  "General Hash functions"
   [ap]: http://www.partow.net  "Arash Partow"
