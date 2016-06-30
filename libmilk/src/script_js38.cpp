#include "script_js.h"
#include "multilanguage.h"
#include "log.h"
#include <fstream>
#include <cassert>
//script_js
namespace lyramilk{namespace script{namespace js
{

	#define MAKE_FLAG(b,d)			((void*)(((((long)b)&1) << 2) | ((((long)d)&1) << 1)))
	#define GET_BUILDER_PTR(b,f)		((void*)((char*)b + ((((long)f) >> 2)&1)))
	#define GET_DESTORY_PTR(d,f)		((void*)((char*)d + ((((long)f) >> 1)&1)))

	bool jsctr(JSContext *cx, unsigned argc, JS::Value *vp);

	void jsdtr(JSFreeOp *fop, JSObject *obj);
	void jsdtrvar(JSFreeOp *fop, JSObject *obj);
	bool jsrmprop(JSContext *cx, JSObject *obj, jsid id, JS::Handle<JS::Value> *vp);

	static JSClass globalClass = { "global", JSCLASS_GLOBAL_FLAGS};

	static JSClass classesClass = { "classes", 0,
		NULL,NULL,NULL,NULL,
		NULL,NULL,NULL,NULL,
		NULL,NULL,jsctr};

	static JSClass nativeClass = { "native", 0,
		NULL,NULL,NULL,NULL,
		NULL,NULL,NULL,jsdtr};

	static JSClass normalClass = { "normal", 0};

	static JSClass varClass = { "lyramilk::data::var", 0,
		NULL,NULL,NULL,NULL,
		NULL,NULL,NULL,jsdtrvar};

	void static script_js_error_report(JSContext *cx, const char *message, JSErrorReport *report)
	{
		lyramilk::klog("lyramilk.script.js") << lyramilk::kdict("%s(%d)%s",(report->filename ? report->filename : "<no name>"),report->lineno,message) << std::endl;
	}

	struct pack
	{
		engine::class_destoryer dtr;
		void* pthis;
		engine* env;

		pack(engine::class_destoryer da,engine* _env,void* dt):dtr(da),pthis(dt),env(_env)
		{}
	};

	lyramilk::data::var j2v(JSContext* cx,JS::Value jv)
	{
		if(jv.isDouble()){
			return jv.toDouble();
		}

		switch(jv.extractNonDoubleType()){
		  case JSVAL_TYPE_DOUBLE:
			return jv.toDouble();
		  case JSVAL_TYPE_INT32:
			return jv.toInt32();
		  case JSVAL_TYPE_UNDEFINED:
			return lyramilk::data::var::nil;
		  case JSVAL_TYPE_BOOLEAN:
			return jv.toInt32();
		  case JSVAL_TYPE_MAGIC:
			return lyramilk::data::var::nil;
		  case JSVAL_TYPE_STRING:{
				JSString* jstr = jv.toString();
				JSAutoByteString mozstr(cx,jstr);
				return lyramilk::data::string(mozstr.ptr(),mozstr.length());
			}
		  case JSVAL_TYPE_NULL:
			return lyramilk::data::var::nil;
		  case JSVAL_TYPE_OBJECT:{
				JSObject *jo = jv.toObjectOrNull();
				if(JS_IsArrayObject(cx,jo)){
					lyramilk::data::var::array r;
					JSObject *iter = JS_NewPropertyIterator(cx,jo);
					if(iter){
						jsid jid;
						while(JS_NextProperty(cx,iter,&jid) && !JSID_IS_VOID(jid)){
							JS::Handle<JS::Value> jv;
							JS_LookupPropertyById(cx,jo,jid,&jv);
							lyramilk::data::var cval = JS::Handle<JS::Value>2var(cx,jv);
							r.push_back(cval);
						}
					}
					return r;
				}else{
					pack *ppack = (pack*)JS_GetPrivate(jo);
					if(ppack && ppack->pthis){
						jsid joid;
						JS_GetObjectId(cx,jo,&joid);
						lyramilk::data::var v("__script_object_id",(void*)joid);
						v.userdata("__script_native_object",ppack->pthis);
						return v;
					}
					JS::Handle<JS::Value> jv;
					if(JS_TRUE == JS_GetProperty(cx,jo,"__script_var",&jv)){
						if(!JSVAL_IS_VOID(jv)){
							void* p = JSVAL_TO_PRIVATE(jv);
							if(p){
								return *(lyramilk::data::var*)p;
							}
						}
					}

					lyramilk::data::var::map m;
					JSObject *iter = JS_NewPropertyIterator(cx,jo);
					if(iter){
						jsid jid;
						while(JS_NextProperty(cx,iter,&jid) && !JSID_IS_VOID(jid)){
							JS::Handle<JS::Value> jk;
							JS_IdToValue(cx,jid,&jk);

							JS::Handle<JS::Value> jv;
							JS_LookupPropertyById(cx,jo,jid,&jv);

							lyramilk::data::var ckey = JS::Handle<JS::Value>2var(cx,jk);
							lyramilk::data::var cval = JS::Handle<JS::Value>2var(cx,jv);
							m.insert(std::pair<lyramilk::data::var,lyramilk::data::var>(ckey,cval));
						}
					}
					return m;
				}
			}
			break;
		  case JSVAL_TYPE_UNKNOWN:
			return lyramilk::data::var::nil;
		  case JSVAL_TYPE_MISSING:
			return lyramilk::data::var::nil;
		}
		return lyramilk::data::var::nil;
	}

	JS::Value v2j(JSContext* cx,lyramilk::data::var v)
	{
		TODO();
	}

	void js_set_property_ptr(JSContext *cx,JSObject* o,lyramilk::data::string name,void* ptr)
	{
		lyramilk::data::string name_flag = name + "__flag";
		int offset = ((long)ptr) & 1;
		if(offset){
//std::cout << "注册偏移" << name_flag << "," << offset << std::endl;
			JS::Value jvflag;
			jvflag.setPrivate((void*)2);
			JS::Handle<JSObject*> ho = JS::MutableHandle<JSObject*>::fromMarkedLocation(&o);
			JS::Handle<JS::Value> hv = JS::MutableHandle<JS::Value>::fromMarkedLocation(&jvflag);
			JS_SetProperty(cx,ho,name_flag.c_str(),hv);
		}else{
			JS::Value jvflag;
			jvflag.setPrivate((void*)0);
			JS::Handle<JSObject*> ho = JS::MutableHandle<JSObject*>::fromMarkedLocation(&o);
			JS::Handle<JS::Value> hv = JS::MutableHandle<JS::Value>::fromMarkedLocation(&jvflag);
			JS_SetProperty(cx,ho,name_flag.c_str(),hv);
		}
//std::cout << "注册指针" << name << "," << ptr << std::endl;
			JS::Value jvflag;
			jvflag.setPrivate(ptr);
			JS::Handle<JSObject*> ho = JS::MutableHandle<JSObject*>::fromMarkedLocation(&o);
			JS::Handle<JS::Value> hv = JS::MutableHandle<JS::Value>::fromMarkedLocation(&jvflag);
			JS_SetProperty(cx,ho,name_flag.c_str(),hv);
	}

	void* js_get_property_ptr(JSContext *cx,JSObject* o,lyramilk::data::string name)
	{
		lyramilk::data::string name_flag = name + "__flag";
		JS::Handle<JSObject*> ho = JS::MutableHandle<JSObject*>::fromMarkedLocation(&o);
		JS::Value jvflag;
		JS::MutableHandle<JS::Value> __flag = JS::MutableHandle<JS::Value>::fromMarkedLocation(&jvflag);
		long offset = 0;
		if(JS_GetProperty(cx,ho,name_flag.c_str(),__flag)){
			void* flag = __flag.get().toPrivate();
			offset = ((long)(flag)>>1);
//std::cout << "获得偏移" << name_flag << ",offset=" << offset << std::endl;
		}
		JS::Value prop;
		JS::MutableHandle<JS::Value> __prop = JS::MutableHandle<JS::Value>::fromMarkedLocation(&jvflag);
		if(JS_GetProperty(cx,ho,name.c_str(),__prop)){
			const char* prop = (const char*)__prop.get().toPrivate();
//std::cout << "获得指针" << name << "," << (void*)&p[offset] << std::endl;
			return (void*)&prop[offset];
		}
		return NULL;
	}




	bool jsctr(JSContext *cx, unsigned argc, JS::Value *vp)
	{
		JS::CallReceiver rec = JS::CallReceiverFromVp(vp);
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

		JSObject& callee = rec.callee();
		rec.rval().set(JS::ObjectValue(callee));

		void *pfuncnew = js_get_property_ptr(cx,&callee,"__new");
		void *pfuncdel = js_get_property_ptr(cx,&callee,"__delete");
		void *penv = js_get_property_ptr(cx,&callee,"__env");
		void *pnewobj = NULL;

		engine::class_builder pfun = (engine::class_builder)pfuncnew;
		if(pfun){
			lyramilk::data::var::array params;
			if(argc>0){
				JS::Value* jv = vp;
				for(unsigned i=0;i<argc;++i){
					lyramilk::data::var v = j2v(cx,jv[i]);
					params.push_back(v);
				}
			}
			pnewobj = pfun(params);
		}


		JSObject* global = JS::CurrentGlobalOrNull(cx);
		JS::Handle<JSObject*> hg = JS::MutableHandle<JSObject*>::fromMarkedLocation(&global);
		JSObject* jsret = JS_NewObject(cx,&nativeClass,hg);
		JS_SetPrivate(jsret,new pack((engine::class_destoryer)pfuncdel,(engine*)penv,pnewobj));











		TODO();
		return true;
	}

	void jsdtr(JSFreeOp *fop, JSObject *obj)
	{
		TODO();
	}
	void jsdtrvar(JSFreeOp *fop, JSObject *obj)
	{
		TODO();
	}

	bool js_func_adapter(JSContext *cx, unsigned argc, JS::Value *vp)
	{
		JS::CallReceiver rec = JS::CallReceiverFromVp(vp);
		TODO();
		return true;
	}

	script_js::script_js():rt(NULL),cx(NULL)
	{
		JS_Init();
		rt = JS_NewRuntime(8L*1024L*1024L);
		cx = JS_NewContext(rt, 8192);

		//JSAutoRequest ar(cx);
		JS::CompartmentOptions options;
		options.setVersion(JSVERSION_LATEST);
		JS::RootedObject glob(cx, JS_NewGlobalObject(cx, &globalClass, NULL,JS::DontFireOnNewGlobalHook,options));

		JSAutoCompartment ac(cx, glob);
		JS_InitStandardClasses(cx,glob);




		global = glob;
COUT << global << std::endl;
		COUT << "ct" << std::endl;
		//TODO();
	}

	script_js::~script_js()
	{
		JS_DestroyContext(cx);
		JS_DestroyRuntime(rt);
		JS_ShutDown();
	}

	bool script_js::load_string(lyramilk::data::string scriptstring)
	{
		TODO();
		return script != NULL;
	}

	bool script_js::load_file(lyramilk::data::string scriptfile)
	{
		lyramilk::data::string scriptstring;
		std::ifstream ifs;
		ifs.open(scriptfile.c_str(),std::ifstream::binary | std::ifstream::in);

		while(ifs){
			char buff[4096];
			ifs.read(buff,4096);
			scriptstring.append(buff,ifs.gcount());
		}
		ifs.close();

		JS::RootedObject glob(cx,global);
		JSAutoCompartment ac(cx, glob);


		JS::CompileOptions options(cx);
		options.setIntroductionType("js shell file")
				.setUTF8(true)
				.setFileAndLine(scriptfile.c_str(), 1)
				.setCompileAndGo(true)
				.setNoScriptRval(true);




		JS_CompileScript(cx,glob,scriptstring.c_str(),scriptstring.size(),options,JS::MutableHandleScript::fromMarkedLocation(&script));
		return script != NULL;
	}

	lyramilk::data::var script_js::pcall(lyramilk::data::var::array args)
	{
		JS::RootedObject glob(cx,global);
		JSAutoCompartment ac(cx, glob);
		JS::RootedScript scri(cx,script);

		JS::Value jv;
		bool ret = JS_ExecuteScript(cx,glob,scri,JS::MutableHandleValue::fromMarkedLocation(&jv));
		gc();
		return ret;

	}

	lyramilk::data::var script_js::call(lyramilk::data::string func,lyramilk::data::var::array args)
	{
		TODO();
	}

	void script_js::reset()
	{
		TODO();
	}

	void script_js::define(lyramilk::data::string classname,functional_map m,class_builder builder,class_destoryer destoryer)
	{
		//std::cout << "注册类：" << classname << ",构造:" << (void*)builder << ",释放" << (void*)destoryer << std::endl;
		JS::RootedObject glob(cx,global);
		JSAutoCompartment ac(cx, glob);
		JS::RootedObject jo(cx,JS_DefineObject(cx,glob,classname.c_str(),&classesClass,0));
		assert(jo);
		/*
		jsid joid;
		JS_GetObjectId(cx,jo,&joid);
		this->m[classname] = joid;
		*/

		js_set_property_ptr(cx,jo,"__new",(void*)builder);
		js_set_property_ptr(cx,jo,"__delete",(void*)destoryer);
		js_set_property_ptr(cx,jo,"__env",(void*)this);
		
		functional_map::iterator it = m.begin();
		for(;it!=m.end();++it){
			//std::cout << "\t注册函数：" << it->first << "," << (void*)it->second << std::endl;
			JSFunction *f = JS_DefineFunction(cx,jo,it->first.c_str(),js_func_adapter,10,10);
			assert(f);
			JSObject *fo = JS_GetFunctionObject(f);
			assert(fo);
			js_set_property_ptr(cx,fo,"__function_pointer",(void*)it->second);
		}
	}

	lyramilk::data::var script_js::createobject(lyramilk::data::string classname,lyramilk::data::var::array args)
	{
//		JS::RootedId id(cx, ::js::NameToId(classname));
		TODO();
	}

	void script_js::gc()
	{
		JS_GC(rt);
	}

}}}
