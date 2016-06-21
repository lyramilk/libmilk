#include "kv_db.h"

namespace lyramilk { namespace data{ namespace kv
{

	// obj
	obj::obj()
	{
	}

	obj::~obj()
	{
	}

	bool obj::from(map* m,const string& key)
	{
		this->m = m;
		this->key = key;
		return true;
	}

	// slice
	slice::slice()
	{
		TODO();
	}

	slice::~slice()
	{
		TODO();
	}

	string slice::get()
	{
		TODO();
		return m->get(key);
	}

	bool slice::set(const var& str)
	{
		TODO();
		return m->set(key,str);
	}

	bool slice::incr()
	{
		TODO();
		return true;
	}

	bool slice::decr()
	{
		TODO();
		return false;
	}


	// hashmap
	hashmap::hashmap()
	{
		TODO();
	}

	hashmap::~hashmap()
	{
		TODO();
	}

	string hashmap::encode_sname()
	{
		TODO();
		string str;
		str.reserve(key.size() + 1);
		str.push_back('H');
		str += key;
		return str;
	}

	string hashmap::encode(const string& field)
	{
		TODO();
		string str;
		str.reserve(key.size() + field.size() + 1);
		str.push_back('h');
		str += key;
		str += "=";
		str += field;
		return str;
	}
	
	string hashmap::get(const string& field)
	{
		TODO();
		return m->get(encode(field));
	}
	
	bool hashmap::set(const string& field,const var& value)
	{
		TODO();
		string rawkey = encode(field);
		if(m->exists(rawkey)){
			slice s;
			s.from(m,encode_sname());
			s.incr();
		}
		return m->set(rawkey,value);
	}

	uint64 hashmap::len()
	{
		TODO();
	}
}}}

