﻿#ifndef _lyramilk_script_lua_engine_h_
#define _lyramilk_script_lua_engine_h_

#include "scriptengine.h"
#include <lua.hpp>

/**
	@namespace lyramilk::script::lua
	@brief 该命名空间用来封装脚本，但并不要求对每种脚本提供统一的定义形式。
*/

namespace lyramilk{namespace script{namespace lua
{
	class script_lua : public lyramilk::script::engine
	{
		lua_State *L;
		lua_State *L_template;
		lyramilk::data::string scriptfilename;
	  public:
		struct metainfo
		{
			class_builder ctr;
			class_destoryer dtr;
			void* self;
			lyramilk::script::engine *env;
			lyramilk::data::string name;
		};
	  public:
		script_lua();
		virtual ~script_lua();
		virtual bool load_string(bool permanent,lyramilk::data::string script);
		virtual bool load_file(bool permanent,lyramilk::data::string scriptfile);
		virtual lyramilk::data::var call(bool permanent,lyramilk::data::var func,lyramilk::data::var::array args);
		virtual void define(bool permanent,lyramilk::data::string classname,functional_map m,class_builder builder,class_destoryer destoryer);
		virtual void define(bool permanent,lyramilk::data::string funcname,functional_type func);
		virtual lyramilk::data::var createobject(lyramilk::data::string classname,lyramilk::data::var::array args);
		virtual void reset();
		virtual void gc();
		virtual lyramilk::data::string name();
		virtual lyramilk::data::string filename();
	  private:
		bool init();
		std::map<lyramilk::data::string,metainfo> minfo;
		void clear();
	};

}}}

#endif
