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
  Isolate* isolate = Isolate::GetCurrent();
  Local<String> className = String::NewFromUtf8(isolate, "LRUCache", String::kInternalizedString);

  Local<FunctionTemplate> constructor = FunctionTemplate::New(isolate, New);
  constructor->SetClassName(className);

  Handle<ObjectTemplate> instance = constructor->InstanceTemplate();
  instance->SetInternalFieldCount(6);

  Handle<ObjectTemplate> prototype = constructor->PrototypeTemplate();
  prototype->Set(String::NewFromUtf8(isolate, "get"), FunctionTemplate::New(isolate, Get)->GetFunction());
  prototype->Set(String::NewFromUtf8(isolate, "set"), FunctionTemplate::New(isolate, Set)->GetFunction());
  prototype->Set(String::NewFromUtf8(isolate, "remove"), FunctionTemplate::New(isolate, Remove)->GetFunction());
  prototype->Set(String::NewFromUtf8(isolate, "clear"), FunctionTemplate::New(isolate, Clear)->GetFunction());
  prototype->Set(String::NewFromUtf8(isolate, "size"), FunctionTemplate::New(isolate, Size)->GetFunction());
  prototype->Set(String::NewFromUtf8(isolate, "stats"), FunctionTemplate::New(isolate, Stats)->GetFunction());
  
  exports->Set(className, constructor->GetFunction());
}

void LRUCache::New(const FunctionCallbackInfo<Value>& args)
{
  Isolate* isolate = Isolate::GetCurrent();
  LRUCache* cache = new LRUCache();

  if (args.Length() > 0 && args[0]->IsObject())
  {
    Local<Object> config = args[0]->ToObject();

    Local<Value> maxElements = config->Get(String::NewFromUtf8(isolate, "maxElements", String::kInternalizedString));
    if (maxElements->IsUint32())
      cache->maxElements = maxElements->Uint32Value();

    Local<Value> maxAge = config->Get(String::NewFromUtf8(isolate, "maxAge", String::kInternalizedString));
    if (maxAge->IsUint32())
      cache->maxAge = maxAge->Uint32Value();

    Local<Value> maxLoadFactor = config->Get(String::NewFromUtf8(isolate, "maxLoadFactor", String::kInternalizedString));
    if (maxLoadFactor->IsNumber())
      cache->data.max_load_factor(maxLoadFactor->NumberValue());

    Local<Value> size = config->Get(String::NewFromUtf8(isolate, "size", String::kInternalizedString));
    if (size->IsUint32())
      cache->data.rehash(ceil(size->Uint32Value() / cache->data.max_load_factor()));
  }

  cache->Wrap(args.This());
  args.GetReturnValue().Set(args.This());
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
    entry->value.Reset();
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
  entry->value.Reset();

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
  entry->value.Reset();

  // Remove the entry from the hash and from the LRU list.
  this->data.erase(itr);
  this->lru.erase(entry->pointer);

  // Free the entry itself.
  delete entry;
}

void LRUCache::Get(const FunctionCallbackInfo<Value>& args)
{
  Isolate* isolate = Isolate::GetCurrent();
  EscapableHandleScope scope(isolate);
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(args.This());

  if (args.Length() != 1)
    isolate->ThrowException(Exception::RangeError(String::NewFromUtf8(isolate, "Incorrect number of arguments for get(), expected 1")));

  std::string key = getStringValue(args[0]);
  const HashMap::const_iterator itr = cache->data.find(key);

  // If the specified entry doesn't exist, return undefined.
  if (itr == cache->data.end())
    args.GetReturnValue().Set(scope.Escape(Local<Value>()));

  HashEntry* entry = itr->second;

  if (cache->maxAge > 0 && getCurrentTime() - entry->timestamp > cache->maxAge)
  {
    // The entry has passed the maximum age, so we need to remove it.
    cache->remove(itr);

    // Return undefined.
    args.GetReturnValue().Set(scope.Escape(Local<Value>()));
  }
  else
  {
    // Move the value to the end of the LRU list.
    cache->lru.splice(cache->lru.end(), cache->lru, entry->pointer);

    // Return the value.
    args.GetReturnValue().Set(scope.Escape(Local<Value>::New(isolate, entry->value)));
  }
}

void LRUCache::Set(const FunctionCallbackInfo<Value>& args)
{
  Isolate* isolate = Isolate::GetCurrent();;
  EscapableHandleScope scope(isolate);
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(args.This());
  unsigned long now = cache->maxAge == 0 ? 0 : getCurrentTime();

  if (args.Length() != 2)
    isolate->ThrowException(Exception::RangeError(String::NewFromUtf8(isolate, "Incorrect number of arguments for set(), expected 2")));

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
    entry->value.Reset();

    // Replace the value in the key-value map with the new one, and update the timestamp.
    entry->value.Reset(isolate, value);
    entry->timestamp = now;

    // Move the value to the end of the LRU list.
    cache->lru.splice(cache->lru.end(), cache->lru, entry->pointer);
  }

  // Return undefined.
  args.GetReturnValue().Set(scope.Escape(Local<Value>()));
}

void LRUCache::Remove(const FunctionCallbackInfo<Value>& args)
{
  Isolate* isolate = Isolate::GetCurrent();
  EscapableHandleScope scope(isolate);
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(args.This());

  if (args.Length() != 1)
    isolate->ThrowException(Exception::RangeError(String::NewFromUtf8(isolate, "Incorrect number of arguments for remove(), expected 1")));

  std::string key = getStringValue(args[0]);
  const HashMap::iterator itr = cache->data.find(key);

  if (itr != cache->data.end())
    cache->remove(itr);

  args.GetReturnValue().Set(scope.Escape(Local<Value>()));
}

void LRUCache::Clear(const FunctionCallbackInfo<Value>& args)
{
  Isolate* isolate = Isolate::GetCurrent();
  EscapableHandleScope scope(isolate);
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(args.This());

  cache->disposeAll();
  cache->data.clear();
  cache->lru.clear();

  args.GetReturnValue().Set(scope.Escape(Local<Value>()));
}

void LRUCache::Size(const FunctionCallbackInfo<Value>& args)
{
  Isolate* isolate = Isolate::GetCurrent();
  EscapableHandleScope scope(isolate);
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(args.This());
  args.GetReturnValue().Set(scope.Escape(Integer::New(isolate, cache->data.size())));
}

void LRUCache::Stats(const FunctionCallbackInfo<Value>& args)
{
  Isolate* isolate = Isolate::GetCurrent();
  EscapableHandleScope scope(isolate);
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(args.This());

  Local<Object> stats = Object::New(isolate);
  stats->Set(String::NewFromUtf8(isolate, "size", String::kInternalizedString), Integer::New(isolate, cache->data.size()));
  stats->Set(String::NewFromUtf8(isolate, "buckets", String::kInternalizedString), Integer::New(isolate, cache->data.bucket_count()));
  stats->Set(String::NewFromUtf8(isolate, "loadFactor", String::kInternalizedString), Number::New(isolate, cache->data.load_factor()));
  stats->Set(String::NewFromUtf8(isolate, "maxLoadFactor", String::kInternalizedString), Number::New(isolate, cache->data.max_load_factor()));

  args.GetReturnValue().Set(scope.Escape(stats));
}
