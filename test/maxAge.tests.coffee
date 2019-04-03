assert = require('chai').assert
LRUCache = require '..'

describe 'maxAge', ->

  describe 'a cache with a maxAge of 100ms', ->

    cache = undefined
    beforeEach -> cache = new LRUCache {maxAge: 100}

    describe 'when a value is requested less than 100ms after it is added', ->

      it 'should return the value', ->
        cache.set 'foo', 42
        value = cache.get 'foo'
        assert.equal value, 42

    describe 'when the value is requested more than 100ms after it is added', ->

      it 'should return undefined', (done) ->
        cache.set 'foo', 42
        setTimeout (->
          value = cache.get 'foo'
          assert value is undefined, "value was #{value}, expected undefined"
          done()
        ), 125

    describe 'when the value is reset less than 100ms after it is added', ->

      it 'should return the value', (done) ->
        cache.set 'foo', 42
        setTimeout (->
          cache.set 'foo', 42
        ), 75
        setTimeout (->
          value = cache.get 'foo'
          assert.equal value, 42
          done()
        ), 125

    describe 'when the value is reset less than 100ms after it is added, with updateTimestamp false', ->

      it 'should return undefined', (done) ->
        cache.set 'foo', 42
        setTimeout (->
          cache.set 'foo', 42, false
        ), 75
        setTimeout (->
          value = cache.get 'foo'
          assert value is undefined, "value was #{value}, expected undefined"
          done()
        ), 125
