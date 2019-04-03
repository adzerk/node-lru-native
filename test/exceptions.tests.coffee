assert = require('chai').assert
LRUCache = require '..'

describe 'exceptions', ->

  describe 'a cache with default settings', ->

    cache = undefined
    beforeEach -> cache = new LRUCache()

    describe 'when get() is called with no arguments', ->
      it 'should throw a RangeError', ->
        assert.throw (-> cache.get()), RangeError

    describe 'when get() is called with two arguments', ->
      it 'should throw a RangeError', ->
        assert.throw (-> cache.get('foo', 'bar')), RangeError

    describe 'when set() is called with no arguments', ->
      it 'should throw a RangeError', ->
        assert.throw (-> cache.set()), RangeError

    describe 'when set() is called with one argument', ->
      it 'should throw a RangeError', ->
        assert.throw (-> cache.set('foo')), RangeError

    describe 'when set() is called with four arguments', ->
      it 'should throw a RangeError', ->
        assert.throw (-> cache.set('foo', 'bar', 'baz', 'baf')), RangeError

    describe 'when remove() is called with no arguments', ->
      it 'should throw a RangeError', ->
        assert.throw (-> cache.remove()), RangeError

    describe 'when remove() is called with two arguments', ->
      it 'should throw a RangeError', ->
        assert.throw (-> cache.remove('foo', 'bar')), RangeError
