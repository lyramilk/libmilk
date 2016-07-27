#ifndef _lyramilk_script_js_engine_h_
#define _lyramilk_script_js_engine_h_
#include "config.h"

#include "scriptengine.h"
#if defined JS_FOUND
	#include <js/jsapi.h>
#elif defined JS170_FOUND
	#include <jsapi.h>
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
#if defined JS_FOUND
		JSObject* global;
		JSObject* script;
#elif defined JS170_FOUND
		JSObject* global;
		JSScript* script;
#elif defined JS17_FOUND
		JSScript* script;
#endif

		std::map<lyramilk::data::string,jsid> m;
	  public:
		script_js();
		virtual ~script_js();
		virtual bool load_string(lyramilk::data::string script);
		virtual bool load_file(lyramilk::data::string scriptfile);
		virtual lyramilk::data::var call(lyramilk::data::var func,lyramilk::data::var::array args);
		virtual void reset();
		virtual void define(lyramilk::data::string classname,functional_map m,class_builder builder,class_destoryer destoryer);
		virtual void define(lyramilk::data::string funcname,functional_type func);
		virtual lyramilk::data::var createobject(lyramilk::data::string classname,lyramilk::data::var::array args);
		virtual void gc();
	};

}}}

#endif
