const assert = require('chai').assert;
const LRUCache = require('..');

describe('maxAge', () => {
  describe('a cache with a maxAge of 100ms', () => {
    let cache = undefined;
    beforeEach(() => {
      cache = new LRUCache({maxAge: 100});
    });

    describe('when a value is requested less than 100ms after it is added', () => {
      it('should return the value', () => {
        cache.set('foo', 42);
        value = cache.get('foo');
        assert.equal(value, 42);
      });
    });

    describe('when the value is requested more than 100ms after it is added', () => {
      it('should return undefined', (done) => {
        cache.set('foo', 42);
        setTimeout(() => {
          value = cache.get('foo');
          assert(value === undefined, `value was #{value}, expected undefined`);
          done();
        }, 200);
      });
    });
  });
});
