#ifndef _lyramilk_script_js_engine_h_
#define _lyramilk_script_js_engine_h_
#include "config.h"

#include "scriptengine.h"
#include <jsapi.h>
/**
	@namespace lyramilk::script::js
	@brief 该命名空间用来封装脚本，但并不要求对每种脚本提供统一的定义形式。
*/

namespace lyramilk{namespace script{namespace js
{
	class script_js : public lyramilk::script::engine
	{
		JSRuntime* rt;
		JSContext* cx_template;

		std::map<lyramilk::data::string,jsid> mdefs;
		lyramilk::data::string scriptfilename;

		lyramilk::data::var::map info;

		JSBool static js_func_adapter_noclass(JSContext *cx, unsigned argc, jsval *vp);
	  public:
		script_js();
		virtual ~script_js();
		virtual bool load_string(lyramilk::data::string script);
		virtual bool load_file(lyramilk::data::string scriptfile);
		virtual lyramilk::data::var call(lyramilk::data::var func,lyramilk::data::var::array args);
		virtual void define(lyramilk::data::string classname,functional_map m,class_builder builder,class_destoryer destoryer);
		virtual void define(lyramilk::data::string funcname,functional_type func);
		virtual lyramilk::data::var createobject(lyramilk::data::string classname,lyramilk::data::var::array args);
		virtual void reset();
		virtual void gc();
		virtual lyramilk::data::string name();
		virtual lyramilk::data::string filename();
		void init();
	};

}}}

#endif
