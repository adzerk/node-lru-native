const assert = require('chai').assert;
const LRUCache = require('../index.js');

describe('basics', () => {
  describe('a cache with default settings', () => {
    let cache = undefined;
    beforeEach(() => {
      cache = new LRUCache();
    });

    it('should be able to set and retrieve primitive values', () => {
      cache.set('foo', 42);
      const value = cache.get('foo');
      assert.equal(value, 42);
    });

    it('should be able to set and retrieve string values', () => {
      cache.set('foo', 'bar');
      const value = cache.get('foo');
      assert.equal(value, 'bar');
    });

    it('should be able to set and retrieve object values', () => {
      const obj = {example: 42}
      cache.set('foo', obj);
      const value = cache.get('foo');
      //assert(value === obj, 'an unexpected item was returned');
    });

    describe('when remove() is called', () => {
      it('should remove the specified item', () => {
        cache.set('foo', 42);
        assert.equal(cache.get('foo'), 42);
        cache.remove('foo');
        const value = cache.get('foo');
        assert(value === undefined, `expected result to be undefined, was #{value}`);
      });
    });

    describe('when size() is called', () => {
      it('should return the correct size', () => {
        assert.equal(cache.size(), 0);
        cache.set('foo', 42);
        assert.equal(cache.size(), 1);
      });
    });

    describe('when clear() is called', () => {
      it('should remove all items in the cache', () => {
        cache.set('foo', 42);
        cache.set('bar', 21);
        cache.clear();
        assert(cache.get('foo') === undefined);
        assert(cache.get('bar') === undefined)
      });
    });

    describe('when stats() is called', () => {
      it('should return fun statistics', () => {
        cache.set('foo', 42);
        const stats = cache.stats();
        assert(stats.size == 1, `expected stats.size to be 1, was #{stats.size}`);
      });
    });
  });
});

