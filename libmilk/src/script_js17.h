#ifndef _lyramilk_script_js17_engine_h_
#define _lyramilk_script_js17_engine_h_
#include "config.h"

#include "scriptengine.h"
#include "datawrapper.h"
#include <jsapi.h>
/**
	@namespace lyramilk::script::js
	@brief 该命名空间用来封装脚本，但并不要求对每种脚本提供统一的定义形式。
*/

namespace lyramilk{namespace script{namespace js
{
	class script_js : public lyramilk::script::engine
	{
	  public:
		struct func_handler
		{
			functional_type func;
			script_js* eng;
		};

		struct class_handler
		{
			unsigned char magic;
			class_builder ctr;
			class_destoryer dtr;
			script_js* eng;
		};
	  private:
		std::list<func_handler> fcache;
		std::list<class_handler> scache;
	  protected:
		JSRuntime* rt;
		JSContext* cx_template;
		lyramilk::data::string scriptfilename;
		lyramilk::data::map info;
		JSBool static js_func_adapter_noclass(JSContext *cx, unsigned argc, jsval *vp);
	  public:
		script_js();
		virtual ~script_js();
		virtual bool load_string(const lyramilk::data::string& script);
		virtual bool load_file(const lyramilk::data::string& scriptfile);
		virtual bool load_module(const lyramilk::data::string& modulefile);

		virtual bool call(const lyramilk::data::var& func,const lyramilk::data::array& args,lyramilk::data::var* ret);
		virtual void define(const lyramilk::data::string& classname,functional_map m,class_builder builder,class_destoryer destoryer);
		virtual void define(const lyramilk::data::string& funcname,functional_type func);
		virtual void define_const(const lyramilk::data::string& key,const lyramilk::data::var& value);
		virtual lyramilk::data::var createobject(const lyramilk::data::string& classname,const lyramilk::data::array& args);
		virtual void reset();
		virtual void gc();
		virtual lyramilk::data::string name();
		virtual lyramilk::data::string filename();
		void init();
	};



	class mozjsobject_datawrapper:public lyramilk::script::objadapter_datawrapper
	{

	  public:
		script_js* _eng;
		jsid _jsid;
	  public:
		mozjsobject_datawrapper(script_js* __eng,jsid __jsid,sclass* __sclass):lyramilk::script::objadapter_datawrapper(__sclass),_eng(__eng),_jsid(__jsid)
		{}

	  	virtual ~mozjsobject_datawrapper()
		{}

		static lyramilk::data::string subclass_name()
		{
			return "lyramilk.mozjs.objadapter";
		}

		virtual lyramilk::data::string subclassname() const
		{
			return subclass_name();
		}

		virtual lyramilk::data::datawrapper* clone() const
		{
			return new mozjsobject_datawrapper(_eng,_jsid,_sclass);
		}
		virtual void destory()
		{
			delete this;
		}
	};


}}}

#endif
