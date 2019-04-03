// tslint:disable-next-line: variable-name no-require-imports
const LRUCache = require("../index.js")
const cache = new LRUCache()

cache.set("test", 1)
console.log("Set/Get")
console.log(cache.get("test"))
console.log("\n\n")

const n = 10
const smallerThanN = 5
const t = 1000
const smallerThanT = 500
const largerThanT = 1600

console.log("SetMaxElements: remain 5 items")
for (let i = 0; i < n; i++) {
  cache.set("test" + i, i)
}
cache.setMaxElements(smallerThanN)
for (let i = 0; i < n; i++) {
  console.log(i, cache.get("test" + i))
}
console.log("\n\n")

console.log("SetMaxAge #1: remain 5 items")
cache.setMaxAge(t)
for (let i = 0; i < n; i++) {
  console.log(i, cache.get("test" + i))
}
console.log("\n\n")

setTimeout(() => {
  console.log("SetMaxAge #2: remain 5 items")
  for (let i = 0; i < n; i++) {
    console.log(i, cache.get("test" + i))
  }
  console.log("\n\n")
}, smallerThanT)

setTimeout(() => {
  console.log("SetMaxAge #3: remain 0 items")
  for (let i = 0; i < n; i++) {
    console.log(i, cache.get("test" + i))
  }
  console.log("\n\n")
}, largerThanT)
