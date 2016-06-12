#ifndef _lyramilk_script_js_engine_h_
#define _lyramilk_script_js_engine_h_

#include "scriptengine.h"
#include <js/jsapi.h>

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
		JSObject* script;
		JSObject* global;
	  public:
		script_js();
		virtual ~script_js();
		virtual bool load_string(lyramilk::data::string script);
		virtual bool load_file(lyramilk::data::string scriptfile);
		virtual lyramilk::data::var pcall(lyramilk::data::var::array args);
		virtual lyramilk::data::var call(lyramilk::data::string func,lyramilk::data::var::array args);
		void reset();
		void define(lyramilk::data::string classname,functional_map m,class_builder builder,class_destoryer destoryer);
	};

}}}

#endif
