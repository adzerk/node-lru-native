#ifndef LRUCACHE_H
#define LRUCACHE_H

#include <time.h>
#include <node.h>
#include <node_object_wrap.h>
#include <string>
#include <list>

#ifdef __APPLE__
#include <tr1/unordered_map>
#define unordered_map std::tr1::unordered_map
#else
#include <unordered_map>
#define unordered_map std::unordered_map
#endif

using namespace v8;

class LRUCache : public node::ObjectWrap {

public:
  static void init(Handle<Object> exports);

private:
  LRUCache();
  ~LRUCache();

  typedef std::list< std::string > KeyList;

  struct HashEntry
  {
    Persistent<Value> value;
    KeyList::iterator pointer;
    unsigned long timestamp;

    HashEntry(Local<Value> value, KeyList::iterator pointer, unsigned long timestamp)
    {
      Isolate* isolate = Isolate::GetCurrent();
      this->value.Reset(isolate, value);
      this->pointer = pointer;
      this->timestamp = timestamp;
    }
  };

  typedef unordered_map< std::string, HashEntry* > HashMap;

  HashMap data;
  KeyList lru;

  size_t maxElements;
  unsigned long maxAge;

  void disposeAll();
  void evict();
  void remove(HashMap::const_iterator itr);

  static void New(const FunctionCallbackInfo<Value> &args);
  static void Get(const FunctionCallbackInfo<Value> &args);
  static void Set(const FunctionCallbackInfo<Value> &args);
  static void Remove(const FunctionCallbackInfo<Value> &args);
  static void Clear(const FunctionCallbackInfo<Value> &args);
  static void Size(const FunctionCallbackInfo<Value> &args);
  static void Stats(const FunctionCallbackInfo<Value> &args);
};

#endif //LRUCACHE_H
