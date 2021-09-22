#include "LRUCache.h"
#include <math.h>
#ifdef _MSC_VER
#include <Windows.h>
#include <stdint.h>

// https://stackoverflow.com/a/26085827
int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
  // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
  // This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
  // until 00:00:00 January 1, 1970
  static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

  SYSTEMTIME  system_time;
  FILETIME    file_time;
  uint64_t    time;

  GetSystemTime( &system_time );
  SystemTimeToFileTime( &system_time, &file_time );
  time =  ((uint64_t)file_time.dwLowDateTime )      ;
  time += ((uint64_t)file_time.dwHighDateTime) << 32;

  tp->tv_sec  = (long) ((time - EPOCH) / 10000000L);
  tp->tv_usec = (long) (system_time.wMilliseconds * 1000);
  return 0;
}

#else
#include <sys/time.h>
#endif

using namespace v8;

unsigned long getCurrentTime() {
  timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

#define ASSERT_ARG_LENGTH(length) \
  if (info.Length() != length) { \
    return Nan::ThrowRangeError("Incorrect number of arguments, exceeded " # length); \
  }

#define ASSERT_ARG_TYPE_IS_STRING(idx) \
  if (!info[idx]->IsString()) { \
    return Nan::ThrowTypeError("Invalid argument, type must be string. pos: " # idx); \
  }

#define GET_FIELD(obj, name) \
  Nan::Get(obj, Nan::New(name).ToLocalChecked())

#define SET_FIELD(obj, name, val) \
  Nan::Set(obj, Nan::New(name).ToLocalChecked(), val)

#define IS_UNDEFINED(val) (val->IsUndefined())
#define IS_UINT32(val)    (!IS_UNDEFINED(val) && val->IsUint32())
#define IS_NUM(val)       (!IS_UNDEFINED(val) && val->IsNumber())

inline std::string convertArgToString(const Local<Value> arg) {
  Nan::Utf8String value(arg);
  return std::string(*value, static_cast<std::size_t>(value.length()));
}

template<typename T>
inline T convertLocalValueToRawType(const Local<Value> arg) {
  return Nan::To<T>(arg).FromJust();
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
  Nan::SetPrototypeMethod(tpl, "setMaxAge", SetMaxAge);
  Nan::SetPrototypeMethod(tpl, "setMaxElements", SetMaxElements);

  constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("LRUCache").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_METHOD(LRUCache::New) {
  if (info.IsConstructCall()) {
    LRUCache* cache = new LRUCache();

    if (info.Length() > 0 && info[0]->IsObject()) {
      Local<Object> config = Nan::To<Object>(info[0]).ToLocalChecked();
      Local<Value> prop;

      prop = GET_FIELD(config, "maxElements").ToLocalChecked();
      if (IS_UINT32(prop)) {
        cache->maxElements = convertLocalValueToRawType<uint32_t>(prop);
      }

      prop = GET_FIELD(config, "maxAge").ToLocalChecked();
      if (IS_UINT32(prop)) {
        cache->maxAge = convertLocalValueToRawType<uint32_t>(prop);
      }

      prop = GET_FIELD(config, "maxLoadFactor").ToLocalChecked();
      if (IS_NUM(prop)) {
        cache->data.max_load_factor(convertLocalValueToRawType<double>(prop));
      }

      prop = GET_FIELD(config, "size").ToLocalChecked();
      if (IS_UINT32(prop)) {
        cache->data.rehash(ceil(convertLocalValueToRawType<uint32_t>(prop) / cache->data.max_load_factor()));
      }
    }

    cache->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  }
  else {
    const int argc = 1;
    Local<Value> argv[argc] = { info[0] };
    Local<v8::Function> ctor = Nan::New<v8::Function>(constructor);
    info.GetReturnValue().Set(Nan::NewInstance(ctor, argc, argv).ToLocalChecked());
  }
}

NAN_METHOD(LRUCache::Get) {
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(info.This());

  ASSERT_ARG_LENGTH(1);
  ASSERT_ARG_TYPE_IS_STRING(0);

  std::string key = convertArgToString(info[0]);
  const HashMap::const_iterator itr = cache->data.find(key);

  // If the specified entry doesn't exist, return undefined.
  if (itr == cache->data.end()) {
    info.GetReturnValue().SetUndefined();
    return;
  }

  HashEntry* entry = itr->second;

  unsigned long now = getCurrentTime();
  if (cache->maxAge > 0 && now - entry->timestamp > cache->maxAge) {
  // if (cache->maxAge > 0 && getCurrentTime() - entry->timestamp > cache->maxAge) {
    // The entry has passed the maximum age, so we need to remove it.
    cache->remove(itr);

    // Return undefined.
    info.GetReturnValue().SetUndefined();
  }
  else {
    /* 2021-09-22 NOTE: The following section of code changes the behavior of our
     * usage of this LRUCache, by causing the `maxAge` timer to be reset each
     * time we get a cache entry. This causes complications if the entry is under
     * load but the desired outcome is to observe a new value for the entry. We
     * are commenting this line out in order to optimize for the downstream user
     * usage, as opposed to proper cache behavior.
    // Update timestamp
    // entry->touch(now);
    */

    // Move the value to the end of the LRU list.
    cache->lru.splice(cache->lru.end(), cache->lru, entry->pointer);

    // Return the value.
    info.GetReturnValue().Set(Nan::New(entry->value));
  }
}

NAN_METHOD(LRUCache::Set) {
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(info.This());
  unsigned long now = getCurrentTime();

  ASSERT_ARG_LENGTH(2);
  ASSERT_ARG_TYPE_IS_STRING(0);

  std::string key = convertArgToString(info[0]);
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


  ASSERT_ARG_LENGTH(1);
  ASSERT_ARG_TYPE_IS_STRING(0);

  std::string key = convertArgToString(info[0]);
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
  SET_FIELD(stats, "size", Nan::New<Number>(cache->data.size()));
  SET_FIELD(stats, "buckets", Nan::New<Number>(cache->data.bucket_count()));
  SET_FIELD(stats, "loadFactor", Nan::New<Number>(cache->data.load_factor()));
  SET_FIELD(stats, "maxLoadFactor", Nan::New<Number>(cache->data.max_load_factor()));
  SET_FIELD(stats, "evictions", Nan::New<Number>(cache->evictions));

  info.GetReturnValue().Set(stats);
}

NAN_METHOD(LRUCache::SetMaxAge) {
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(info.This());

  ASSERT_ARG_LENGTH(1);

  cache->maxAge = convertLocalValueToRawType<int64_t>(info[0]);
  cache->gc(getCurrentTime(), true);
}

NAN_METHOD(LRUCache::SetMaxElements) {
  LRUCache* cache = ObjectWrap::Unwrap<LRUCache>(info.This());

  ASSERT_ARG_LENGTH(1);

  cache->maxElements = convertLocalValueToRawType<int64_t>(info[0]);
  while (cache->maxElements > 0 && cache->data.size() > cache->maxElements) {
    cache->evict();
  }
}


LRUCache::LRUCache() :
  maxElements(0),
  maxAge(0),
  evictions(0)
{}

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

  this->evictions++;

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

void LRUCache::gc(unsigned long now, bool force) {
  HashMap::iterator itr;
  HashEntry* entry;

  // If there is no maximum age, we won't evict based on age.
  if (this->maxAge == 0) {
    return;
  }

  static unsigned long counter = 0;
  if (!force && (++counter)%10 != 0) {
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
