#ifndef _lyramilk_script_js_engine_h_
#define _lyramilk_script_js_engine_h_
#include "config.h"

#include "scriptengine.h"
#ifdef JS24_FOUND 
	#include <mozjs-24/jsapi.h>
#elif defined JS38_FOUND
	#include <jsapi.h>
#elif defined JS_FOUND
	#include <js/jsapi.h>
#elif defined JS17_FOUND
	#include <jsapi.h>
#endif

/**
	@namespace lyramilk::script::js
	@brief 该命名空间用来封装脚本，但并不要求对每种脚本提供统一的定义形式。
*/

namespace lyramilk{namespace script{namespace js
{
	class script_js : public lyramilk::script::engine
	{
		JSRuntime* rt;
		JSContext* cx;
#ifdef JS38_FOUND 
		JSScript* script;
		JSObject* global;
#elif defined JS24_FOUND 
		JSScript* script;
		JSObject* global;
#elif defined JS17_FOUND 
		JSScript* script;
#elif defined JS_FOUND
		JSObject* script;
		JSObject* global;
#endif

		std::map<lyramilk::data::string,jsid> m;
	  public:
		script_js();
		virtual ~script_js();
		virtual bool load_string(lyramilk::data::string script);
		virtual bool load_file(lyramilk::data::string scriptfile);
		virtual lyramilk::data::var pcall(lyramilk::data::var::array args);
		virtual lyramilk::data::var call(lyramilk::data::string func,lyramilk::data::var::array args);
		virtual void reset();
		virtual void define(lyramilk::data::string classname,functional_map m,class_builder builder,class_destoryer destoryer);
		virtual void define(lyramilk::data::string funcname,functional_type func);
		virtual lyramilk::data::var createobject(lyramilk::data::string classname,lyramilk::data::var::array args);
		virtual void gc();
	};

}}}

#endif
