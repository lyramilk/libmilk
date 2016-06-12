#include "scriptengine.h"
#include <fstream>

namespace lyramilk{namespace script
{
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


	enginefactory::enginefactory()
	{}

	enginefactory::~enginefactory()
	{}

	enginefactory* enginefactory::instance()
	{
		static enginefactory _mm;
		return &_mm;
	}
	
	void enginefactory::regist(lyramilk::data::string name,engine* eng,int engsize,int engcount)
	{
		std::vector<enginelessee::enginelease>& e = es[name];
		int msz = (int)e.size();
		e.resize(msz + engcount);
		for(int i = 0;i<engcount;++i){
			e[msz + i].eng = (engine*)((char*)eng + engsize);
		}
	}
	
	void enginefactory::unregist(lyramilk::data::string name)
	{
		es.erase(name);
	}
	
	enginelessee enginefactory::get(lyramilk::data::string name)
	{
		std::vector<enginelessee::enginelease>& e = es[name];
		std::vector<enginelessee::enginelease>::iterator it = e.begin();
		for(;it!=e.end();++it){
			if(it->lock.try_lock()){
				return enginelessee(*it);
			}
		}
		return enginelessee();
	}
	


	bool enginefactory::load_string(lyramilk::data::string script)
	{
		std::map<lyramilk::data::string,std::vector<enginelessee::enginelease> >::iterator it = es.begin();
		for(;it!=es.end();++it){
			std::vector<enginelessee::enginelease>::iterator vit = it->second.begin();
			for(;vit!=it->second.end();++vit){
				lyramilk::system::threading::mutex_sync _(vit->lock);
				vit->eng->load_string(script);
			}
		}
		return true;
	}

	lyramilk::data::var enginefactory::pcall(lyramilk::data::var::array args)
	{
		TODO();
	}

	lyramilk::data::var enginefactory::call(lyramilk::data::string func,lyramilk::data::var::array args)
	{
		TODO();
	}

	void enginefactory::reset()
	{
		std::map<lyramilk::data::string,std::vector<enginelessee::enginelease> >::iterator it = es.begin();
		for(;it!=es.end();++it){
			std::vector<enginelessee::enginelease>::iterator vit = it->second.begin();
			for(;vit!=it->second.end();++vit){
				lyramilk::system::threading::mutex_sync _(vit->lock);
				vit->eng->reset();
			}
		}
	}

	void enginefactory::define(lyramilk::data::string classname,engine::functional_map m,engine::class_builder builder,engine::class_destoryer destoryer)
	{
		std::map<lyramilk::data::string,std::vector<enginelessee::enginelease> >::iterator it = es.begin();
		for(;it!=es.end();++it){
			std::vector<enginelessee::enginelease>::iterator vit = it->second.begin();
			for(;vit!=it->second.end();++vit){
				lyramilk::system::threading::mutex_sync _(vit->lock);
				vit->eng->define(classname,m,builder,destoryer);
			}
		}
	}
}}
