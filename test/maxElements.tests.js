const assert = require('chai').assert;
const LRUCache = require('..');

describe('maxElements', () => {
  describe('a cache with a maxElements of 1', () => {
    let cache = undefined;
    beforeEach(() => {
      cache = new LRUCache({maxElements: 1});
    });

    describe('when a single value is added', () => {
      it('should return the value when requested', () => {
        cache.set('foo', 42);
        const value = cache.get('foo');
        assert.equal(value, 42);
      });
    });

    describe('when two values are added', () => {
      it('should return undefined when the older value is requested', () => {
        cache.set('foo', 42);
        cache.set('bar', 21);
        const value = cache.get('foo');
        assert(value === undefined, 'value was not undefined');
      });
    });
  });
});
