#include "scriptengine.h"
#include <fstream>
#include <cassert>

namespace lyramilk{namespace script
{
	// sclass
	sclass::sclass()
	{
	}

	sclass::~sclass()
	{
	}

	bool sclass::iterator_begin()
	{
		return false;
	}

	bool sclass::iterator_next(std::size_t idx,lyramilk::data::var* v)
	{
		return false;
	}

	void sclass::iterator_end()
	{
	}

	bool sclass::set_property(const lyramilk::data::string& k,const lyramilk::data::var& v)
	{
		return false;
	}

	bool sclass::get_property(const lyramilk::data::string& k,lyramilk::data::var* v)
	{
		return false;
	}

	// engine
	engine::engine()
	{}

	engine::~engine()
	{}

	lyramilk::data::var& engine::get(const lyramilk::data::string& k)
	{
		/*
		lyramilk::data::map::iterator it = _mparams.find(k);
		if(it == _mparams.end()) return lyramilk::data::var::nil;
		return it->second;*/
		return mparams[k];
	}

	bool engine::set(const lyramilk::data::string& k,const lyramilk::data::var& v)
	{
		mparams[k] = v;
		return true;
	}


	bool engine::load_file(const lyramilk::data::string& scriptfile)
	{
		lyramilk::data::string str;
		std::ifstream ifs;
		ifs.open(scriptfile.c_str(),std::ifstream::binary | std::ifstream::in);

		while(ifs){
			char buff[4096];
			ifs.read(buff,4096);
			str.append(buff,(unsigned int)ifs.gcount());
		}
		ifs.close();
		return load_string(str);
	}

	lyramilk::data::var engine::call(lyramilk::data::var func)
	{
		lyramilk::data::array a;
		return call(func,a);
	}
	
	void engine::gc()
	{}

	struct engine_helper
	{
		lyramilk::script::engine* (*ctr)();
		void (*dtr)(lyramilk::script::engine*);
	};

	typedef std::map<lyramilk::data::string,engine_helper> engine_builder;

	static engine_builder& get_builder()
	{
		static engine_builder _mm;
		return _mm;
	};



	bool engine::define(const lyramilk::data::string& scriptname,lyramilk::script::engine* (*builder)(),void (*destoryer)(lyramilk::script::engine*))
	{
		engine_helper r;
		r.ctr = builder;
		r.dtr = destoryer;
		std::pair<engine_builder::iterator,bool> pr = get_builder().insert(std::make_pair(scriptname,r));
		return pr.second;
	}

	void engine::undef(const lyramilk::data::string& funcname)
	{
		get_builder().erase(funcname);
	}

	lyramilk::script::engine* engine::createinstance(const lyramilk::data::string& scriptname)
	{
		engine_builder::const_iterator it = get_builder().find(scriptname);
		if(it == get_builder().end()) return nullptr;
		return it->second.ctr();
	}

	void engine::destoryinstance(const lyramilk::data::string& scriptname,lyramilk::script::engine* eng)
	{
		engine_builder::const_iterator it = get_builder().find(scriptname);
		if(it == get_builder().end()) return;
		return it->second.dtr(eng);
	}

	// engines
	engines::engines()
	{}

	engines::~engines()
	{}

	void engines::onfire(engine* o)
	{
		assert(o);
		o->gc();
	}

	void engines::reset()
	{
		list_type::iterator it = es.begin();
		for(;it!=es.end();++it){
			lyramilk::threading::mutex_sync _(it->l);
			it->t->reset();
		}
	}
}}
