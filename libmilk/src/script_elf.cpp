#include "script_elf.h"
#include "multilanguage.h"
#include "log.h"
#include <fstream>
#include <cassert>
#include <string.h>
#include <dlfcn.h>
//script_elf
namespace lyramilk{namespace script{namespace elf
{
	script_elf::script_elf()
	{
		handle = NULL;
	}

	script_elf::~script_elf()
	{
		if(handle){
			dlclose(handle);
			handle = NULL;
		}
	}

	bool script_elf::load_string(lyramilk::data::string scriptstring)
	{
		return false;
	}

	bool script_elf::load_file(lyramilk::data::string scriptfile)
	{
		handle = dlopen(scriptfile.c_str(), RTLD_NOW);
		if(handle == NULL){
			lyramilk::klog(lyramilk::log::error,"lyramilk.script.elf") << D("加载%s失败：%s",scriptfile.c_str(),dlerror()) << std::endl;
			return false;
		}
		elffilename = scriptfile;
		return true;
	}

	lyramilk::data::var script_elf::pcall(lyramilk::data::var::array args)
	{
		return lyramilk::data::var::nil;
	}

	lyramilk::data::var script_elf::call(lyramilk::data::string func,lyramilk::data::var::array args)
	{
		int (*pfunc)(void*) = (int (*)(void*))dlsym(handle,func.c_str());
		if(!pfunc){
			lyramilk::klog(lyramilk::log::error,"lyramilk.script.elf") << D("%s中找不到符号：%s",elffilename.c_str(),func.c_str(),dlerror()) << std::endl;
		}

		return pfunc(&args);
	}

	void script_elf::reset()
	{
		if(handle){
			dlclose(handle);
			handle = NULL;
		}
	}

	void script_elf::define(lyramilk::data::string classname,functional_map m,class_builder builder,class_destoryer destoryer)
	{
		
	}
}}}
