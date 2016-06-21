#ifndef _lyramilk_data_kv_db_h_
#define _lyramilk_data_kv_db_h_

#include "var.h"
#include "iterator.h"

namespace lyramilk { namespace data{ namespace kv
{
	class map
	{
	  public:
		template <typename C>
		class iterator:public bidirectional_iterator<C,std::pair<string,var> >
		{
		  public:
			iterator();
			virtual ~iterator();
			virtual std::pair<string,var>* get() = 0;
			virtual void tonext() = 0;
			virtual void toprev() = 0;
			virtual bool equal(const iterator& c) const = 0;
			virtual iterator& assign(const iterator &o) = 0;
			virtual iterator& eof() const = 0;
		};

		virtual bool set(const string& k,var v) = 0;
		virtual var get(const string& k) = 0;
		virtual bool exists(const string& k) = 0;
		virtual uint64 size() = 0;

		template <typename C>
		void scan(iterator<C>& it)
		{
			return iterator<C>(this);
		}
	};

	class obj
	{
	  protected:
		map* m;
		string key;
	  public:
		obj();
		virtual ~obj();
		virtual bool from(map* m,const string& key);
	};

	class slice:public obj
	{
	  public:
		slice();
		virtual ~slice();

		virtual string get();
		virtual bool set(const var& str);

		virtual bool incr();
		virtual bool decr();
	};

	class hashmap:public obj
	{
	  public:
		hashmap();
		virtual ~hashmap();

		virtual string encode_sname();
		virtual string encode(const string& field);

		virtual string get(const string& field);
		virtual bool set(const string& field,const var& value);

		virtual uint64 len();
	};
}}}

#endif