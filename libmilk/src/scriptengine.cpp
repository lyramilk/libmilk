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

	//enginelessee
	enginelessee::enginelease::enginelease():eng(NULL)
	{
	}
	enginelessee::enginelessee():autounlock(false),e(NULL)
	{
	}
	enginelessee::enginelessee(enginelease& o):autounlock(true),e(&o)
	{
	}
	

	enginelessee::enginelessee(const enginelessee& o):autounlock(true),e(o.e)
	{
		((enginelessee&)o).autounlock = false;
	}

	enginelessee& enginelessee::operator =(const enginelessee& o)
	{
		autounlock = true;
		e = ((enginelessee&)o).e;
		((enginelessee&)o).autounlock = false;
		return *this;
	}

	enginelessee::~enginelessee()
	{
		if(e && autounlock){
			e->lock.unlock();
			e->eng->gc();
		}
	}

	engine* enginelessee::operator->()
	{
		return (e?e->eng:NULL);
	}

	engine* enginelessee::operator*()
	{
		return (e?e->eng:NULL);
	}

	enginelessee::operator bool()
	{
		return e && e->eng;
	}


	engines::engines()
	{}

	engines::~engines()
	{}
	
	void engines::push_back(engine* eng)
	{
		es.push_back(enginelessee::enginelease());
		es.back().eng = eng;
	}
	
	void engines::clear()
	{
		es.clear();
	}
	
	enginelessee engines::get()
	{
		std::list<enginelessee::enginelease>::iterator it = es.begin();
		for(;it!=es.end();++it){
			if(it->lock.try_lock()){
				return enginelessee(*it);
			}
		}
		return enginelessee();
	}
	


	bool engines::load_string(lyramilk::data::string script)
	{
		std::list<enginelessee::enginelease>::iterator it = es.begin();
		for(;it!=es.end();++it){
			lyramilk::system::threading::mutex_sync _(it->lock);
			it->eng->load_string(script);
		}
		return true;
	}

	lyramilk::data::var engines::pcall(lyramilk::data::var::array args)
	{
		TODO();
	}

	lyramilk::data::var engines::call(lyramilk::data::string func,lyramilk::data::var::array args)
	{
		TODO();
	}

	void engines::reset()
	{
		std::list<enginelessee::enginelease>::iterator it = es.begin();
		for(;it!=es.end();++it){
			lyramilk::system::threading::mutex_sync _(it->lock);
			it->eng->reset();
		}
	}

	void engines::define(lyramilk::data::string classname,engine::functional_map m,engine::class_builder builder,engine::class_destoryer destoryer)
	{
		std::list<enginelessee::enginelease>::iterator it = es.begin();
		for(;it!=es.end();++it){
			lyramilk::system::threading::mutex_sync _(it->lock);
			it->eng->define(classname,m,builder,destoryer);
		}
	}

	lyramilk::data::var engines::createobject(lyramilk::data::string classname,lyramilk::data::var::array args)
	{
		TODO();
	}

}}
