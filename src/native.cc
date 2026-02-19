#include <nan.h>
#include "LRUCache.h"

NAN_MODULE_INIT(Init) {
	LRUCache::Init(target);
}

NAN_MODULE_WORKER_ENABLED(native, Init)
