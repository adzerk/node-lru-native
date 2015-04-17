#include "LRUCache.h"
#include <sys/time.h>
#include <math.h>

#ifndef __APPLE__
#include <unordered_map>
#endif

using namespace v8;

unsigned long getCurrentTime()
{
  timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

std::string getStringValue(Handle<Value> value)
{
  String::Utf8Value keyUtf8Value(value);
  return std::string(*keyUtf8Value);
}

void LRUCache::init(Handle<Object> exports)
{
  Local<String> className = NanNew("LRUCache");

  Local<FunctionTemplate> constructor = NanNew<FunctionTemplate>(New);
  constructor->SetClassName(className);

  Handle<ObjectTemplate> instance = constructor->InstanceTemplate();
  instance->SetInternalFieldCount(6);

  Handle<ObjectTemplate> prototype = constructor->PrototypeTemplate();
  prototype->Set(NanNew("get"), NanNew<FunctionTemplate>(Get)->GetFunction());
  prototype->Set(NanNew("set"), NanNew<FunctionTemplate>(Set)->GetFunction());
  prototype->Set(NanNew("remove"), NanNew<FunctionTemplate>(Remove)->GetFunction());
  prototype->Set(NanNew("clear"), NanNew<FunctionTemplate>(Clear)->GetFunction());
  prototype->Set(NanNew("size"), NanNew<FunctionTemplate>(Size)->GetFunction());
  prototype->Set(NanNew("stats"), NanNew<FunctionTemplate>(Stats)->GetFunction());
  
  exports->Set(className, constructor->GetFunction());
}

NAN_METHOD(LRUCache::New)
{
  LRUCache* cache = new LRUCache();

  if (args.Length() > 0 && args[0]->IsObject())
  {
    Local<Object> config = args[0]->ToObject();

    Local<Value> maxElements = config->Get(NanNew("maxElements"));
    if (maxElements->IsUint32())
      cache->maxElements = maxElements->Uint32Value();

    Local<Value> maxAge = config->Get(NanNew("maxAge"));
    if (maxAge->IsUint32())
      cache->maxAge = maxAge->Uint32Value();

    Local<Value> maxLoadFactor = config->Get(NanNew("maxLoadFactor"));
    if (maxLoadFactor->IsNumber())
      cache->data.max_load_factor(maxLoadFactor->NumberValue());

    Local<Value> size = config->Get(NanNew("size"));
    if (size->IsUint32())
      cache->data.rehash(ceil(size->Uint32Value() / cache->data.max_load_factor()));
  }

  cache->Wrap(args.This());

  NanReturnValue(args.This());
}

LRUCache::LRUCache()
{
  this->maxElements = 0;
  this->maxAge = 0;
}

LRUCache::~LRUCache()
{
  this->disposeAll();
}

void LRUCache::disposeAll()
{
  for (HashMap::iterator itr = this->data.begin(); itr != this->data.end(); itr++)
  {
    HashEntry* entry = itr->second;
    NanDisposePersistent(entry->value);
    delete entry;
  }
}

void LRUCache::evict()
{
  const HashMap::iterator itr = this->data.find(this->lru.front());

  if (itr == this->data.end())
    return;

  HashEntry* entry = itr->second;

  // Dispose the V8 handle contained in the entry.
  NanDisposePersistent(entry->value);

  // Remove the entry from the hash and from the LRU list.
  this->data.erase(itr);
  this->lru.pop_front();

  // Free the entry itself.
  delete entry;
}

void LRUCache::remove(const HashMap::const_iterator itr)
{
  HashEntry* entry = itr->second;

  // Dispose the V8 handle contained in the entry.
  NanDisposePersistent(entry->value);

  // Remove the entry from the hash and from the LRU list.
  this->data.erase(itr);
  this->lru.erase(entry->pointer);

  // Free the entry itself.
  delete entry;
}

NAN_METHOD(LRUCache::Get)
{
  NanScope();
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(args.This());

  if (args.Length() != 1)
    return NanThrowRangeError("Incorrect number of arguments for get(), expected 1");

  std::string key = getStringValue(args[0]);
  const HashMap::const_iterator itr = cache->data.find(key);

  // If the specified entry doesn't exist, return undefined.
  if (itr == cache->data.end())
    NanReturnUndefined();

  HashEntry* entry = itr->second;

  if (cache->maxAge > 0 && getCurrentTime() - entry->timestamp > cache->maxAge)
  {
    // The entry has passed the maximum age, so we need to remove it.
    cache->remove(itr);

    // Return undefined.
    NanReturnUndefined();
  }
  else
  {
    // Move the value to the end of the LRU list.
    cache->lru.splice(cache->lru.end(), cache->lru, entry->pointer);

    // Return the value.
    NanReturnValue(entry->value);
    //args.GetReturnValue().Set(scope.Escape(Local<Value>::New(isolate, entry->value)));
  }
}

NAN_METHOD(LRUCache::Set)
{
  NanScope();
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(args.This());
  unsigned long now = cache->maxAge == 0 ? 0 : getCurrentTime();

  if (args.Length() != 2)
    return NanThrowRangeError("Incorrect number of arguments for set(), expected 2");

  std::string key = getStringValue(args[0]);
  Local<Value> value = args[1];
  const HashMap::iterator itr = cache->data.find(key);

  if (itr == cache->data.end())
  {
    // We're adding a new item. First ensure we have space.
    if (cache->maxElements > 0 && cache->data.size() == cache->maxElements)
      cache->evict();

    // Add the value to the end of the LRU list.
    KeyList::iterator pointer = cache->lru.insert(cache->lru.end(), key);

    // Add the entry to the key-value map.
    HashEntry* entry = new HashEntry(value, pointer, now);
    cache->data.insert(std::make_pair(key, entry));
  }
  else
  {
    HashEntry* entry = itr->second;

    // We're replacing an existing value, so dispose the old V8 handle to ensure it gets GC'd.
    NanDisposePersistent(entry->value);

    // Replace the value in the key-value map with the new one, and update the timestamp.
    NanAssignPersistent(entry->value, value);
    entry->timestamp = now;

    // Move the value to the end of the LRU list.
    cache->lru.splice(cache->lru.end(), cache->lru, entry->pointer);
  }

  // Return undefined.
  NanReturnUndefined();
}

NAN_METHOD(LRUCache::Remove)
{
  NanScope();
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(args.This());

  if (args.Length() != 1)
    NanThrowRangeError("Incorrect number of arguments for remove(), expected 1");

  std::string key = getStringValue(args[0]);
  const HashMap::iterator itr = cache->data.find(key);

  if (itr != cache->data.end())
    cache->remove(itr);
  
  // Return undefined.
  NanReturnUndefined();
}

NAN_METHOD(LRUCache::Clear)
{
  NanScope();
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(args.This());

  cache->disposeAll();
  cache->data.clear();
  cache->lru.clear();

  // Return undefined.
  NanReturnUndefined();
}

NAN_METHOD(LRUCache::Size)
{
  NanScope();
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(args.This());
  NanReturnValue(NanNew<Integer>(static_cast<uint32_t>(cache->data.size())));
}

NAN_METHOD(LRUCache::Stats)
{
  NanScope();
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(args.This());

  Local<Object> stats = NanNew<Object>();
  stats->Set(NanNew("size"), NanNew<Integer>(static_cast<uint32_t>(cache->data.size())));
  stats->Set(NanNew("buckets"), NanNew<Integer>(static_cast<uint32_t>(cache->data.bucket_count())));
  stats->Set(NanNew("loadFactor"), NanNew<Integer>(static_cast<uint32_t>(cache->data.load_factor())));
  stats->Set(NanNew("maxLoadFactor"), NanNew<Integer>(static_cast<uint32_t>(cache->data.max_load_factor())));

  NanReturnValue(stats);
}
