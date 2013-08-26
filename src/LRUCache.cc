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
  return tv.tv_sec * 1000 + tv.tv_usec;
}

std::string getStringValue(Handle<Value> value)
{
  String::Utf8Value keyUtf8Value(value);
  return std::string(*keyUtf8Value);
}

void LRUCache::init(Handle<Object> exports)
{
  Local<String> className = String::NewSymbol("LRUCache");

  Local<FunctionTemplate> constructor = FunctionTemplate::New(New);
  constructor->SetClassName(className);

  Handle<ObjectTemplate> instance = constructor->InstanceTemplate();
  instance->SetInternalFieldCount(6);

  Handle<ObjectTemplate> prototype = constructor->PrototypeTemplate();
  prototype->Set("get", FunctionTemplate::New(Get)->GetFunction());
  prototype->Set("set", FunctionTemplate::New(Set)->GetFunction());
  prototype->Set("remove", FunctionTemplate::New(Remove)->GetFunction());
  prototype->Set("clear", FunctionTemplate::New(Clear)->GetFunction());
  prototype->Set("size", FunctionTemplate::New(Size)->GetFunction());
  prototype->Set("stats", FunctionTemplate::New(Stats)->GetFunction());

  exports->Set(className, Persistent<Function>::New(constructor->GetFunction()));
}

Handle<Value> LRUCache::New(const Arguments& args)
{
  LRUCache* cache = new LRUCache();

  if (args.Length() > 0 && args[0]->IsObject())
  {
    Local<Object> config = args[0]->ToObject();

    Local<Value> maxElements = config->Get(String::NewSymbol("maxElements"));
    if (maxElements->IsUint32())
      cache->maxElements = maxElements->Uint32Value();

    Local<Value> maxAge = config->Get(String::NewSymbol("maxAge"));
    if (maxAge->IsUint32())
      cache->maxAge = maxAge->Uint32Value();

    Local<Value> maxLoadFactor = config->Get(String::NewSymbol("maxLoadFactor"));
    if (maxLoadFactor->IsNumber())
      cache->data.max_load_factor(maxLoadFactor->NumberValue());

    Local<Value> size = config->Get(String::NewSymbol("size"));
    if (size->IsUint32())
      cache->data.rehash(ceil(size->Uint32Value() / cache->data.max_load_factor()));
  }

  cache->Wrap(args.This());
  return args.This();
}

LRUCache::LRUCache()
{
  this->maxElements = 0;
  this->maxAge = 0;
}

LRUCache::~LRUCache()
{
  this->disposeAllHandles();
}

void LRUCache::disposeAllHandles()
{
  for (HashMap::iterator itr = this->data.begin(); itr != this->data.end(); itr++)
  {
    Persistent<Value> value = itr->second.value;
    value.Dispose();
  }
}

void LRUCache::evict()
{
  const HashMap::iterator itr = this->data.find(this->lru.front());

  if (itr == this->data.end())
    return;

  Persistent<Value> value = itr->second.value;
  value.Dispose();

  this->data.erase(itr);
  this->lru.pop_front();
}

Handle<Value> LRUCache::Get(const Arguments& args)
{
  HandleScope scope;
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(args.This());
  unsigned long now = getCurrentTime();

  if (args.Length() != 1)
    return ThrowException(Exception::RangeError(String::New("Incorrect number of arguments for get(), expected 1")));

  std::string key = getStringValue(args[0]);
  const HashMap::const_iterator itr = cache->data.find(key);

  // If the specified entry doesn't exist, return undefined.
  if (itr == cache->data.end())
    return scope.Close(Handle<Value>());

  HashEntry entry = itr->second;

  if (cache->maxAge > 0 && now - entry.timestamp > cache->maxAge)
  {
    // The entry has passed the maximum age, so we need to remove it. Dispose the handle first.
    entry.value.Dispose();

    // Remove the entry from the hash and from the LRU list.
    cache->data.erase(itr);
    cache->lru.erase(entry.pointer);

    // Return undefined.
    return scope.Close(Handle<Value>());
  }
  else
  {
    // Move the value to the end of the LRU list.
    cache->lru.splice(cache->lru.end(), cache->lru, entry.pointer);

    // Return the value.
    return scope.Close(entry.value);
  }
}

Handle<Value> LRUCache::Set(const Arguments& args)
{
  HandleScope scope;
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(args.This());
  unsigned long now = getCurrentTime();

  if (args.Length() != 2)
    return ThrowException(Exception::RangeError(String::New("Incorrect number of arguments for set(), expected 2")));

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
    HashEntry entry = HashEntry(Persistent<Value>::New(value), pointer, now);
    cache->data.insert(std::make_pair(key, entry));
  }
  else
  {
    HashEntry entry = itr->second;

    // We're replacing an existing value, so dispose of the old reference to ensure it gets GC'd.
    entry.value.Dispose();

    // Replace the value in the key-value map with the new one, and update the timestamp.
    entry.value = Persistent<Value>::New(value);
    entry.timestamp = now;

    // Move the value to the end of the LRU list.
    cache->lru.splice(cache->lru.end(), cache->lru, entry.pointer);
  }

  // Return undefined.
  return scope.Close(Handle<Value>());
}

Handle<Value> LRUCache::Remove(const Arguments& args)
{
  HandleScope scope;
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(args.This());

  if (args.Length() != 1)
    return ThrowException(Exception::RangeError(String::New("Incorrect number of arguments for remove(), expected 1")));

  std::string key = getStringValue(args[0]);
  const HashMap::iterator itr = cache->data.find(key);

  if (itr != cache->data.end())
  {
    HashEntry entry = itr->second;
    entry.value.Dispose();

    cache->data.erase(itr);
    cache->lru.remove(key);
  }

  return scope.Close(Handle<Value>());
}

Handle<Value> LRUCache::Clear(const Arguments& args)
{
  HandleScope scope;
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(args.This());

  cache->disposeAllHandles();
  cache->data.clear();
  cache->lru.clear();

  return scope.Close(Handle<Value>());
}

Handle<Value> LRUCache::Size(const Arguments& args)
{
  HandleScope scope;
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(args.This());
  return scope.Close(Integer::New(cache->data.size()));
}

Handle<Value> LRUCache::Stats(const Arguments& args)
{
  HandleScope scope;
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(args.This());

  Local<Object> stats = Object::New();
  stats->Set(String::NewSymbol("size"), Integer::New(cache->data.size()));
  stats->Set(String::NewSymbol("buckets"), Integer::New(cache->data.bucket_count()));
  stats->Set(String::NewSymbol("loadFactor"), Number::New(cache->data.load_factor()));
  stats->Set(String::NewSymbol("maxLoadFactor"), Number::New(cache->data.max_load_factor()));

  return scope.Close(stats);
}
