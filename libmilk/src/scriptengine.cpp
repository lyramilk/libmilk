#include "scriptengine.h"
#include <fstream>

namespace lyramilk{namespace script
{
	// engine
	engine::engine()
	{}

	engine::~engine()
	{}

	lyramilk::data::var& engine::get(lyramilk::data::string k)
	{
		/*
		lyramilk::data::var::map::iterator it = _mparams.find(k);
		if(it == _mparams.end()) return lyramilk::data::var::nil;
		return it->second;*/
		return mparams[k];
	}

	bool engine::set(lyramilk::data::string k,lyramilk::data::var v)
	{
		mparams[k] = v;
		return true;
	}


	bool engine::load_file(bool permanent,lyramilk::data::string scriptfile)
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
		return load_string(permanent,str);
	}

	lyramilk::data::var engine::call(bool permanent,lyramilk::data::var func)
	{
		lyramilk::data::var::array a;
		return call(permanent,func,a);
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



	bool engine::define(lyramilk::data::string scriptname,lyramilk::script::engine* (*builder)(),void (*destoryer)(lyramilk::script::engine*))
	{
		engine_helper r;
		r.ctr = builder;
		r.dtr = destoryer;
		std::pair<engine_builder::iterator,bool> pr = get_builder().insert(std::make_pair(scriptname,r));
		return pr.second;
	}

	lyramilk::script::engine* engine::createinstance(lyramilk::data::string scriptname)
	{
		engine_builder::const_iterator it = get_builder().find(scriptname);
		if(it == get_builder().end()) return nullptr;
		return it->second.ctr();
	}

	void engine::destoryinstance(lyramilk::data::string scriptname,lyramilk::script::engine* eng)
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
