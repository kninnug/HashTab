HashTab: a simple but effective hash table implementation.
==========================================================

Author: Marco Gunnink <marco@kninnug.nl>  
Date: 2015-03-06  
Version: 2.0.2

The hash table uses (singly-)linked lists to avoid collisions and incremental
resizing to grow the table when its load factor exceeds a provided value.
When growing the table a new one is allocated, twice the size of the
original. Then on every add operation the new item is added to the other,
and a number of other items are moved to the new one as well. When the
original is empty the new one is merged back in. Items are moved after remove
operations as well. The number of items moved is `size / moveR`, so higher
values of `moveR` move fewer items per operation and a value of 1 is equivalent
to rehashing the entire table immediately.

A table can also be told to shrink when it reaches the inverse of the
provided load factor threshold `(1 - threshold)`. Shrinking is done by
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
  goes below `1 - threshold`.
- `hasher`: The hash function used for adding & retrieving items.
- `cmp`: The compare function used to determine exact equality (since hashes
  may collide).
- `moveR`: The move rate (for lack of better wording). When growing the table
  at every add or remove operation `size / moveR` items are moved. So when
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
- `free`: De-allocate the hash table and it's allocated members. Note that this
  does not free any items that may still be in it. To do this either remove
  the items individually or call `forEach` (see below) with an item-cleanup 
  function.
- `add`: Add a new item to the hash table. Note that this adds the item even if
  it already exists. To replace existing items use insert (see below).
- `insert`: Insert an item: if it can be found in the table replace it, other-
  wise add it.
- `find`: Find an item in the table.
- `forEach`: Iterate over all the items in the table with a callback function.
- `remove`: Remove an item to the hash table.

License: MIT (see LICENSE.md)

[Example hash functions][ghc] collected by [Arash Partow][ap].

   [ghc]: http://www.partow.net/programming/hashfunctions/  "General Hash functions"
   [ap]: http://www.partow.net  "Arash Partow"
