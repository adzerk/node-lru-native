const assert = require("chai").assert
const LRUCache = require("..")

describe("maxAge", () => {
  describe("a cache with a maxAge of 100ms", () => {
    let cache = undefined
    beforeEach(() => {
      cache = new LRUCache({ maxAge: 100 })
    })

    it("does not expire value added and requested in less than 100ms", () => {
      cache.set("foo", 42)

      describe("returns correct value", () => {
        const value = cache.get("foo")
        assert.equal(value, 42)
      })

      describe("counts no evictions", () => {
        const stats = cache.stats()
        assert.equal(stats.evictions, 0)
      })
    })

    it("expires value requested 100ms after addition", () => {
      cache.set("foo", 42)

      const awaitExpiration = new Promise(resolve => setTimeout(resolve, 200))

      describe("returns undefined", () =>
        awaitExpiration.then(() => {
          const value = cache.get("foo")
          assert(value === undefined, `value was ${value}, expected undefined`)
        })
      )

      describe("counts one eviction", () =>
        awaitExpiration.then(() => {
          const stats = cache.stats()
          assert.equal(stats.evictions, 1)
        })
      )
    })
  })
})
