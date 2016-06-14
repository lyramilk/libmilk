#include "scriptengine.h"
#include <fstream>

namespace lyramilk{namespace script
{
	// engine
	engine::engine()
	{}

	engine::~engine()
	{}

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

	lyramilk::data::var engine::pcall()
	{
		lyramilk::data::var::array a;
		return pcall(a);
	}
	lyramilk::data::var engine::call(lyramilk::data::string func)
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
	
	bool engines::load_string(lyramilk::data::string script)
	{
		list_type::iterator it = es.begin();
		for(;it!=es.end();++it){
			lyramilk::threading::mutex_sync _(it->l);
			it->t->load_string(script);
		}
		return true;
	}

	void engines::reset()
	{
		list_type::iterator it = es.begin();
		for(;it!=es.end();++it){
			lyramilk::threading::mutex_sync _(it->l);
			it->t->reset();
		}
	}

	void engines::define(lyramilk::data::string classname,engine::functional_map m,engine::class_builder builder,engine::class_destoryer destoryer)
	{
		list_type::iterator it = es.begin();
		for(;it!=es.end();++it){
			lyramilk::threading::mutex_sync _(it->l);
			it->t->define(classname,m,builder,destoryer);
		}
	}

	lyramilk::data::var engines::pcall(lyramilk::data::var::array args)
	{
		TODO();
	}

	lyramilk::data::var engines::call(lyramilk::data::string func,lyramilk::data::var::array args)
	{
		TODO();
	}

	lyramilk::data::var engines::createobject(lyramilk::data::string classname,lyramilk::data::var::array args)
	{
		TODO();
	}

}}
