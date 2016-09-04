#ifndef _lyramilk_script_lua_engine_h_
#define _lyramilk_script_lua_engine_h_

#include "scriptengine.h"
extern "C" {
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

#include <stack>

/**
	@namespace lyramilk::script::lua
	@brief 该命名空间用来封装脚本，但并不要求对每种脚本提供统一的定义形式。
*/

namespace lyramilk{namespace script{namespace lua
{
	class script_lua : public lyramilk::script::engine
	{
		lua_State *L;
		lyramilk::data::string scriptfilename;
	  public:
		std::stack<lyramilk::data::string> callstack;
		struct metainfo
		{
			class_builder ctr;
			class_destoryer dtr;
			std::map<lyramilk::data::string,functional_type> funcmap;
			void* self;
			lyramilk::script::engine *env;
		};
	  public:
		script_lua();
		virtual ~script_lua();
		virtual bool load_string(lyramilk::data::string script);
		virtual bool load_file(lyramilk::data::string scriptfile);
		virtual lyramilk::data::var call(lyramilk::data::var func,lyramilk::data::var::array args);
		virtual void reset();
		virtual void define(lyramilk::data::string classname,functional_map m,class_builder builder,class_destoryer destoryer);
		virtual lyramilk::data::var createobject(lyramilk::data::string classname,lyramilk::data::var::array args);
		virtual void gc();
		virtual lyramilk::data::string name();
		virtual lyramilk::data::string filename();
	  private:
		std::map<lyramilk::data::string,metainfo> minfo;
		void clear();
	};

}}}

#endif
