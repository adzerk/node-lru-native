lru-native2
===========

This is an implementation of a simple in-memory cache for node.js, supporting LRU (least-recently-used) eviction
and TTL expirations.

It was developed as an alternative to the (excellent) [node-lru-cache](https://github.com/isaacs/node-lru-cache)
library for use with hashes with a very large number of items. V8 normally does a good job of optimizing the
in-memory representation of objects, but it isn't optimized for an object that holds a huge amount of data.
When you add a very large number of properties (particularly with non-integer keys) to an object, performance
begins to suffer.

Rather than rely on V8 to figure out what we're trying to do, `node-lru-native` is a light wrapper around
`std::unordered_map` from C++11. A `std::list` is used to track accesses so we can evict the least-recently-used
item when necessary.

Based on the [node-hashtable](https://github.com/isaacbwagner/node-hashtable) library by Issac Wagner.

# Usage

Install via npm:

```
$ npm install lru-native2
```

Then:

```javascript
var LRUCache = require('lru-native2');
var cache = new LRUCache({ maxElements: 1000 });
cache.set('some-key', 42);
var value = cache.get('some-key');
```

If you'd like to tinker, you can build the extension using [node-gyp](https://github.com/TooTallNate/node-gyp):

```
$ npm install -g node-gyp
$ node-gyp configure
$ node-gyp build
```

# Configuration

To configure the cache, you can pass a hash to the `LRUCache` constructor with the following options:

```
var cache = new LRUCache({

  // The maximum number of items to add to the cache before evicting the least-recently-used item.
  // Default: 0, meaning there is no maximum.
  maxElements: 10000,

  // The maximum age (in milliseconds) of an item.
  // The item will be removed if get() is called and the item is too old.
  // Default: 0, meaning items will never expire.
  maxAge: 60000,

  // The initial number of items for which space should be allocated.
  // The cache will resize dynamically if necessary.
  size: 1000,

  // The maximum load factor for buckets in the unordered_map.
  // Typically you won't need to change this.
  maxLoadFactor: 2.0

});
```

# API

## set(key, value)

Adds the specified item to the cache with the specified key.

## get(key)

Returns the item with the specified key, or `undefined` if no item exists with that key.

## remove(key)

Removes the item with the specified key if it exists.

## clear()

Removes all items from the cache.

## size()

Returns the number of items in the cache.

## stats()

Returns a hash containing internal information about the cache.

## setMaxElements(maxElements)

Set the maximum number of items

## setMaxAge(maxAge)

Set the maximum age (in milliseconds) of an item


# Changelog

- 1.0.0 -- Update the timestamp when get a value(like common LRU cache). Added SetMaxAge(), SetMaxElements()
- *forked*
- 0.4.0 -- Added support for newer versions of Node via NAN
- 0.3.0 -- Changed memory allocation strategy, fixed issue where remove() would do a seek through the LRU list, code cleanup
- 0.2.0 -- Fixed issue where maxAge-based removal would result in a seek through the LRU list
- 0.1.0 -- Initial release
