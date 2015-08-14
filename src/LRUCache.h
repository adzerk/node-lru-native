#ifndef LRUCACHE_H
#define LRUCACHE_H

#include <time.h>
#include <node.h>
#include <node_object_wrap.h>
#include <nan.h>
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
      NanAssignPersistent(this->value, value);
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

  static NAN_METHOD(New);
  static NAN_METHOD(Get);
  static NAN_METHOD(Set);
  static NAN_METHOD(Remove);
  static NAN_METHOD(Clear);
  static NAN_METHOD(Size);
  static NAN_METHOD(Stats);
};

#endif //LRUCACHE_H
