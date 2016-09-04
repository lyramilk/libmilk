#ifndef _lyramilk_script_elf_engine_h_
#define _lyramilk_script_elf_engine_h_

#include "scriptengine.h"

/**
	@namespace lyramilk::script::elf
	@brief 该命名空间用来封装脚本，但并不要求对每种脚本提供统一的定义形式。
*/

namespace lyramilk{namespace script{namespace elf
{
	class script_elf : public lyramilk::script::engine
	{
	  public:
		script_elf();
		virtual ~script_elf();
		virtual bool load_string(lyramilk::data::string script);
		virtual bool load_file(lyramilk::data::string scriptfile);
		virtual lyramilk::data::var call(lyramilk::data::var func,lyramilk::data::var::array args);
		void reset();
		virtual void define(lyramilk::data::string classname,functional_map m,class_builder builder,class_destoryer destoryer);
		virtual void define(lyramilk::data::string funcname,functional_type func);
		virtual lyramilk::data::var createobject(lyramilk::data::string classname,lyramilk::data::var::array args);
		virtual void gc();
		virtual lyramilk::data::string name();
		virtual lyramilk::data::string filename();
	  private:
		lyramilk::data::string elffilename;
		void* handle;
	};

}}}

#endif
