#ifndef LRUCACHE_H
#define LRUCACHE_H

#include <time.h>
#include <node.h>
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

class LRUCache : public node::ObjectWrap
{

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

    HashEntry(Persistent<Value> value, KeyList::iterator pointer, unsigned long timestamp)
    {
      this->value = value;
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

  static Handle<Value> New(const Arguments &args);
  static Handle<Value> Get(const Arguments &args);
  static Handle<Value> Set(const Arguments &args);
  static Handle<Value> Remove(const Arguments &args);
  static Handle<Value> Clear(const Arguments &args);
  static Handle<Value> Size(const Arguments &args);
  static Handle<Value> Stats(const Arguments &args);

};

#endif //LRUCACHE_H