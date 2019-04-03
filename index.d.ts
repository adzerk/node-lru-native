declare module "lru-native3" {
  export interface LRUCacheOptions {
    maxElements?: number
    maxAge?: number
    size: number
    maxLoadFactor: number
  }

  export interface LRUCacheStats {
    size: number
    buckets: number
    loadFactor: number
    maxLoadFactor: number
  }

  type K = string

  export class LRUCache<V> {
    constructor(options: LRUCacheOptions)

    public set(key: K, value: V): void
    public get(key: K): V
    public setMaxElements(n: number)
    public setMaxAge(millis: number)
    public remove(key: K): void
    public clear(): void
    public size(): number
    public stats(): LRUCacheStats
  }
}
