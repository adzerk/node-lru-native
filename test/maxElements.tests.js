const assert = require("chai").assert
const LRUCache = require("..")

describe("maxElements", () => {
  describe("a cache with a maxElements of 1", () => {
    let cache = undefined
    beforeEach(() => {
      cache = new LRUCache({ maxElements: 1 })
    })

    it("correctly adds a single value", () => {
      cache.set("foo", 42)

      describe("returns added value when requested", () => {
        const value = cache.get("foo")
        assert.equal(value, 42)
      })

      describe("does not count any evictions", () => {
        const stats = cache.stats()
        assert.equal(stats.evictions, 0)
      })
    })

    it("correctly adds two values", () => {
      cache.set("foo", 42)
      cache.set("bar", 21)

      describe("returns undefined when the older value is requested", () => {
        const value = cache.get("foo")
        assert(value === undefined, "value was not undefined")
      })

      describe("counts one eviction", () => {
        const stats = cache.stats()
        assert.equal(stats.evictions, 1)
      })
    })
  })
})
