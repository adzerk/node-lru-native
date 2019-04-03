#include <nan.h>
#include "LRUCache.h"

NAN_MODULE_INIT(Init) {
  LRUCache::Init(target);
}

NODE_MODULE(native, Init)
