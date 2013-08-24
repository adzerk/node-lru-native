assert = require('chai').assert
LRUCache = require '..'
SegfaultHandler = require 'segfault-handler'

SegfaultHandler.registerHandler()

describe 'the cache', ->

	cache = new LRUCache {maxElements: 100, maxAge: 100}

	it 'should work', (done) ->
		cache.set('key1', 'value1')
		value = cache.get('key1')
		assert.equal value, 'value1'
		setTimeout (->
			value = cache.get('key1')
			assert.equal value, undefined
			done()
		), 1000
