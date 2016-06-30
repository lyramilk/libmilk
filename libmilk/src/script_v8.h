#ifndef _lyramilk_script_v8js_engine_h_
#define _lyramilk_script_v8js_engine_h_

#include "scriptengine.h"
#include <v8.h>
#include "log.h"

/**
	@namespace lyramilk::script::js
	@brief 该命名空间用来封装脚本，但并不要求对每种脚本提供统一的定义形式。
*/

namespace lyramilk{namespace script{namespace js
{
	class script_v8 : public lyramilk::script::engine
	{
	  protected:
		lyramilk::log::logss log;
		std::map<lyramilk::data::string,v8::Persistent<v8::ObjectTemplate> > m;
	  public:
		struct pack
		{
			lyramilk::data::string cname;

			engine::class_builder ctr;
			engine::class_destoryer dtr;
			void* pthis;
			script_v8* env;

			pack();
			pack(lyramilk::data::string name,engine::class_builder ca,engine::class_destoryer da,script_v8* _env,void* dt);

			~pack();
		};

		bool forceglobal;

		v8::Isolate* isolate;
		v8::Persistent<v8::ObjectTemplate> globaltpl;
		v8::Persistent<v8::Context> ctx;
		v8::Persistent<v8::Script> script;

		std::map<void*,v8::Persistent<v8::Object> > stk;

		struct frame
		{
			std::list<pack> a;
			std::list<lyramilk::data::var> v;
		} *pframe;

		frame gf;
		frame tmp;

		lyramilk::data::var* build_var();
		pack* build_pack(lyramilk::data::string name,engine::class_builder ca,engine::class_destoryer da,script_v8* _env,void* dt);

		script_v8();
		virtual ~script_v8();
		virtual bool init(lyramilk::data::var::map m);
		virtual bool load_string(lyramilk::data::string script);
		virtual bool load_file(lyramilk::data::string scriptfile);
		virtual lyramilk::data::var pcall(lyramilk::data::var::array args);
		virtual lyramilk::data::var call(lyramilk::data::string func,lyramilk::data::var::array args);
		void reset();
		void define(lyramilk::data::string classname,functional_map m,class_builder builder,class_destoryer destoryer);
		virtual lyramilk::data::var createobject(lyramilk::data::string classname,lyramilk::data::var::array args);
		virtual void gc();

	
		lyramilk::data::var j2v(v8::Handle<v8::Value> d);
		v8::Handle<v8::Value> v2j(lyramilk::data::var v);
};

}}}

#endif
