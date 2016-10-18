#ifndef LRUCACHE_H
#define LRUCACHE_H

#include <nan.h>
#include <time.h>
#include <string>
#include <list>

using namespace v8;

#ifdef __APPLE__
#include <tr1/unordered_map>
#define unordered_map std::tr1::unordered_map
#else
#include <unordered_map>
#define unordered_map std::unordered_map
#endif

class LRUCache : public Nan::ObjectWrap {

public:
	static NAN_MODULE_INIT(Init);

private:

	LRUCache();
	~LRUCache();

	static NAN_METHOD(New);
	static NAN_METHOD(Get);
	static NAN_METHOD(Set);
	static NAN_METHOD(Remove);
	static NAN_METHOD(Clear);
	static NAN_METHOD(Size);
	static NAN_METHOD(Stats);
	static NAN_METHOD(SetMaxAge);
	static NAN_METHOD(SetMaxElements);

	static Nan::Persistent<Function> constructor;
	typedef std::list<std::string> KeyList;

	struct HashEntry {
		Nan::Persistent<Value> value;
		KeyList::iterator pointer;
		unsigned long timestamp;

		HashEntry(Local<Value> value, unsigned long timestamp, KeyList::iterator pointer) {
			this->set(value, timestamp);
			this->pointer = pointer;
		}

		void set(Local<Value> value, unsigned long timestamp) {
			this->value.Reset(value);
			this->timestamp = timestamp;
		}

		void touch(unsigned long timestamp) {
			this->timestamp = timestamp;
		}

		void dispose() {
			this->value.Reset();
		}
	};

	typedef unordered_map<std::string, HashEntry*> HashMap;

	HashMap data;
	KeyList lru;

	size_t maxElements;
	unsigned long maxAge;

	void disposeAll();
	void evict();
	void remove(HashMap::const_iterator itr);
	void gc(unsigned long now, bool force=false);

};

#endif //LRUCACHE_H
