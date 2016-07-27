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
		return _mparams[k];
	}

	bool engine::set(lyramilk::data::string k,lyramilk::data::var v)
	{
		_mparams[k] = v;
		return true;
	}


	bool engine::load_file(lyramilk::data::string scriptfile)
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
		lyramilk::data::var::array a;
		return call(func,a);
	}
	
	void engine::gc()
	{}

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
