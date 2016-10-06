#include "LRUCache.h"
#include <sys/time.h>
#include <math.h>

using namespace v8;

#ifndef __APPLE__
#include <unordered_map>
#endif

unsigned long getCurrentTime() {
  timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

std::string getStringValue(v8::Handle<Value> value) {
  String::Utf8Value keyUtf8Value(value);
  return std::string(*keyUtf8Value);
}

Nan::Persistent<Function> LRUCache::constructor;

NAN_MODULE_INIT(LRUCache::Init) {
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("LRUCache").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(6);

  Nan::SetPrototypeMethod(tpl, "get", Get);
  Nan::SetPrototypeMethod(tpl, "set", Set);
  Nan::SetPrototypeMethod(tpl, "remove", Remove);
  Nan::SetPrototypeMethod(tpl, "clear", Clear);
  Nan::SetPrototypeMethod(tpl, "size", Size);
  Nan::SetPrototypeMethod(tpl, "stats", Stats);

  constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("LRUCache").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(LRUCache::New) {
  if (info.IsConstructCall()) {
    LRUCache* cache = new LRUCache();

    if (info.Length() > 0 && info[0]->IsObject()) {
      Local<Object> config = info[0]->ToObject();
      Local<Value> prop;

      prop = config->Get(Nan::New("maxElements").ToLocalChecked());
      if (!prop->IsUndefined() && prop->IsUint32()) {
        cache->maxElements = prop->Uint32Value();
      }

      prop = config->Get(Nan::New("maxAge").ToLocalChecked());
      if (!prop->IsUndefined() && prop->IsUint32()) {
        cache->maxAge = prop->Uint32Value();
      }

      prop = config->Get(Nan::New("maxLoadFactor").ToLocalChecked());
      if (!prop->IsUndefined() && prop->IsNumber()) {
        cache->data.max_load_factor(prop->NumberValue());
      }

      prop = config->Get(Nan::New("size").ToLocalChecked());
      if (!prop->IsUndefined() && prop->IsUint32()) {
        cache->data.rehash(ceil(prop->Uint32Value() / cache->data.max_load_factor()));
      }
    }

    cache->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  }
  else {
    const int argc = 1;
    Local<Value> argv[argc] = { info[0] };
    Local<v8::Function> ctor = Nan::New<v8::Function>(constructor);
    info.GetReturnValue().Set(ctor->NewInstance(argc, argv));
  }
}

NAN_METHOD(LRUCache::Get) {
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(info.This());

  if (info.Length() != 1) {
    Nan::ThrowRangeError("Incorrect number of arguments for get(), expected 1");
  }

  std::string key = getStringValue(info[0]);
  const HashMap::const_iterator itr = cache->data.find(key);

  // If the specified entry doesn't exist, return undefined.
  if (itr == cache->data.end()) {
    info.GetReturnValue().SetUndefined();
    return;
  }

  HashEntry* entry = itr->second;

  unsigned long now = getCurrentTime();
  if (cache->maxAge > 0 && now - entry->timestamp > cache->maxAge) {
    // The entry has passed the maximum age, so we need to remove it.
    cache->remove(itr);

    // Return undefined.
    info.GetReturnValue().SetUndefined();
  }
  else {
    if (cache->maxAge > 0) {
      //update last access timestamp
      entry->touch(now);
    }

    // Move the value to the end of the LRU list.
    cache->lru.splice(cache->lru.end(), cache->lru, entry->pointer);

    // Return the value.
    info.GetReturnValue().Set(Nan::New(entry->value));
  }
}

NAN_METHOD(LRUCache::Set) {
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(info.This());
  unsigned long now = cache->maxAge == 0 ? 0 : getCurrentTime();

  if (info.Length() != 2) {
    Nan::ThrowRangeError("Incorrect number of arguments for set(), expected 2");
  }

  std::string key = getStringValue(info[0]);
  Local<Value> value = info[1];
  const HashMap::iterator itr = cache->data.find(key);

  if (itr == cache->data.end()) {
    // We're adding a new item. First ensure we have space.
    if (cache->maxElements > 0 && cache->data.size() == cache->maxElements) {
      cache->evict();
    }

    // Add the value to the end of the LRU list.
    KeyList::iterator pointer = cache->lru.insert(cache->lru.end(), key);

    // Add the entry to the key-value map.
    HashEntry* entry = new HashEntry(value, now, pointer);
    cache->data.insert(std::make_pair(key, entry));
  }
  else {
    // We're replacing an existing value.
    HashEntry* entry = itr->second;

    // Dispose the old value in the key-value map, replacing it with the new one, and update the timestamp.
    entry->set(value, now);

    // Move the value to the end of the LRU list.
    cache->lru.splice(cache->lru.end(), cache->lru, entry->pointer);
  }

  // Remove items that have exceeded max age.
  cache->gc(now);

  // Return undefined.
  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(LRUCache::Remove) {
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(info.This());

  if (info.Length() != 1) {
    Nan::ThrowRangeError("Incorrect number of arguments for remove(), expected 1");
  }

  std::string key = getStringValue(info[0]);
  const HashMap::iterator itr = cache->data.find(key);

  if (itr != cache->data.end()) {
    cache->remove(itr);
  }

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(LRUCache::Clear) {
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(info.This());

  cache->disposeAll();
  cache->data.clear();
  cache->lru.clear();

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(LRUCache::Size) {
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(info.This());
  info.GetReturnValue().Set(Nan::New<Number>(cache->data.size()));
}

NAN_METHOD(LRUCache::Stats) {
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(info.This());

  Local<Object> stats = Nan::New<Object>();
  stats->Set(Nan::New("size").ToLocalChecked(), Nan::New<Number>(cache->data.size()));
  stats->Set(Nan::New("buckets").ToLocalChecked(), Nan::New<Number>(cache->data.bucket_count()));
  stats->Set(Nan::New("loadFactor").ToLocalChecked(), Nan::New<Number>(cache->data.load_factor()));
  stats->Set(Nan::New("maxLoadFactor").ToLocalChecked(), Nan::New<Number>(cache->data.max_load_factor()));

  info.GetReturnValue().Set(stats);
}

LRUCache::LRUCache() {
  this->maxElements = 0;
  this->maxAge = 0;
}

LRUCache::~LRUCache() {
  this->disposeAll();
}

void LRUCache::disposeAll() {
  for (HashMap::iterator itr = this->data.begin(); itr != this->data.end(); itr++) {
    HashEntry* entry = itr->second;
    entry->dispose();
    delete entry;
  }
}

void LRUCache::evict() {
  const HashMap::iterator itr = this->data.find(this->lru.front());

  if (itr == this->data.end()) {
    return;
  }

  HashEntry* entry = itr->second;

  // Dispose the V8 handle contained in the entry.
  entry->dispose();

  // Remove the entry from the hash and from the LRU list.
  this->data.erase(itr);
  this->lru.pop_front();

  // Free the entry itself.
  delete entry;
}

void LRUCache::remove(const HashMap::const_iterator itr) {
  HashEntry* entry = itr->second;

  // Dispose the V8 handle contained in the entry.
  entry->dispose();

  // Remove the entry from the hash and from the LRU list.
  this->data.erase(itr);
  this->lru.erase(entry->pointer);

  // Free the entry itself.
  delete entry;
}

void LRUCache::gc(unsigned long now) {
  HashMap::iterator itr;
  HashEntry* entry;

  // If there is no maximum age, we won't evict based on age.
  if (this->maxAge == 0) {
    return;
  }

  static unsigned long counter = 0;
  if ((++counter)%100 != 0) {
    return;
  }

  while (!this->lru.empty()) {
    itr = this->data.find(this->lru.front());

    if (itr == this->data.end())
      break;

    entry = itr->second;

    // Stop removing when live entry is encountered.
    if (now - entry->timestamp < this->maxAge)
      break;

    // Dispose the V8 handle contained in the entry.
    entry->dispose();

    // Remove the entry from the hash and from the LRU list.
    this->data.erase(itr);
    this->lru.pop_front();

    // Free the entry itself.
    delete entry;
  }
}
