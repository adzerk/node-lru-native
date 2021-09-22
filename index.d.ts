declare module "lru-native2" {
  interface LRUCacheOptions {
    maxElements?: number
    maxAge?: number
    size: number
    maxLoadFactor: number
  }

  interface LRUCacheStats {
    size: number
    buckets: number
    loadFactor: number
    maxLoadFactor: number
    evictions: number
  }

  type K = string

  class LRUCache<V> {
    constructor(options: LRUCacheOptions)

    public set(key: K, value: V): void
    public get(key: K): V | undefined
    public setMaxElements(n: number): void
    public setMaxAge(millis: number): void
    public remove(key: K): void
    public clear(): void
    public size(): number
    public stats(): LRUCacheStats
  }

  export = LRUCache
}
