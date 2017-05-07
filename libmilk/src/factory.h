#ifndef _lyramilk_util_factory_h_
#define _lyramilk_util_factory_h_
#include "var.h"

namespace lyramilk{ namespace util
{
	template <typename T>
	class factory
	{
	  public:
		typedef T* (*builder)(void* args);
		typedef void (*destoryer)(T*);

		virtual ~factory()
		{}

		virtual void define(lyramilk::data::string name,builder ctr,destoryer dtr)
		{
			ctrdtr_pair s;
			s.ctr = ctr;
			s.dtr = dtr;
			m[name] = s;
		}

		virtual void undef(lyramilk::data::string name)
		{
			m.erase(name);
		}

		virtual lyramilk::data::var::array keys() const
		{
			lyramilk::data::var::array ar;
			typename map_type::const_iterator it = m.begin();
			for(;it!=m.end();++it){
				ar.push_back(it->first);
			}
			return ar;
		}

		virtual T* create(lyramilk::data::string name,void* args)
		{
			typename map_type::iterator it = m.find(name);
			if(it == m.end()){
				return nullptr;
			}
			return it->second.ctr(args);
		}

		virtual T* create(lyramilk::data::string name)
		{
			typename map_type::iterator it = m.find(name);
			if(it == m.end()){
				return nullptr;
			}
			return it->second.ctr(nullptr);
		}

		virtual bool destory(lyramilk::data::string name,T* p)
		{
			typename map_type::iterator it = m.find(name);
			if(it == m.end()){
				return false;
			}
			it->second.dtr(p);
			return true;
		}
	  protected:
		struct ctrdtr_pair
		{
			builder ctr;
			destoryer dtr;
		};
		typedef std::map<lyramilk::data::string,ctrdtr_pair> map_type;
		map_type m;
	};


	template <typename T>
	class multiton_factory
	{
	  public:
		virtual ~multiton_factory()
		{}

		virtual void define(lyramilk::data::string name,T* ptr)
		{
			m[name] = ptr;
		}

		virtual void undef(lyramilk::data::string name)
		{
			m.erase(name);
		}

		virtual lyramilk::data::var::array keys() const
		{
			lyramilk::data::var::array ar;
			typename map_type::const_iterator it = m.begin();
			for(;it!=m.end();++it){
				ar.push_back(it->first);
			}
			return ar;
		}

		virtual T* get(lyramilk::data::string name)
		{
			typename map_type::iterator it = m.find(name);
			if(it == m.end()){
				return nullptr;
			}
			return it->second;
		}
	  protected:
		typedef std::map<lyramilk::data::string,T*> map_type;
		map_type m;
	};


}}

#endif