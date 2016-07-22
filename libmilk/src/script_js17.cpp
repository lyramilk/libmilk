#include "script_js.h"
#include "multilanguage.h"
#include "log.h"
#include <fstream>
#include <cassert>
#include <gc/Root.h>
//script_js
namespace lyramilk{namespace script{namespace js
{
	JSBool jsctr(JSContext *cx, unsigned argc, jsval *vp);
	void jsdtr(JSFreeOp *fop, JSObject *obj);
	void jsdtrvar(JSFreeOp *fop, JSObject *obj);
	
	static JSClass globalClass = { "global", JSCLASS_GLOBAL_FLAGS,
			JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
            JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub};

	static JSClass classesClass = { "classes", 0,
			JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
			JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,
			NULL,NULL,NULL,NULL,jsctr};

	static JSClass nativeClass = { "native", 0,
			JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
			JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,
			jsdtr};

	static JSClass normalClass = { "normal", 0,
			JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
            JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub};

	static JSClass varClass = { "lyramilk::data::var", 0,
			JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
			JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,
			jsdtrvar};

	void static script_js_error_report(JSContext *cx, const char *message, JSErrorReport *report)
	{
		lyramilk::klog(lyramilk::log::warning,"lyramilk.script.js") << lyramilk::kdict("%s(%d)%s",(report->filename ? report->filename : "<no name>"),report->lineno,message) << std::endl;
	}

	void js_set_property_ptr(JSContext *cx,JSObject* o,lyramilk::data::string name,void* ptr)
	{
		lyramilk::data::string name_flag = name + "__flag";
		int offset = ((long)ptr) & 1;
		if(offset){
			jsval jvflag = INT_TO_JSVAL(1);
			JS_SetProperty(cx,o,name_flag.c_str(),&jvflag);
		}else{
			jsval jvflag = INT_TO_JSVAL(0);
			JS_SetProperty(cx,o,name_flag.c_str(),&jvflag);
		}
		jsval jv = PRIVATE_TO_JSVAL(ptr);
		JS_SetProperty(cx,o,name.c_str(),&jv);
	}

	void* js_get_property_ptr(JSContext *cx,JSObject* o,lyramilk::data::string name)
	{
		lyramilk::data::string name_flag = name + "__flag";
		jsval __flag;
		long offset = 0;
		if(JS_GetProperty(cx,o,name_flag.c_str(),&__flag)){
			if(JSVAL_IS_INT(__flag)){
				offset = JSVAL_TO_INT(__flag);
			}
		}
		jsval __prop;
		if(JS_GetProperty(cx,o,name.c_str(),&__prop)){
			if(JSVAL_IS_DOUBLE(__prop)){
				const char* p = (const char*)JSVAL_TO_PRIVATE(__prop);
				return (void*)&p[offset];
			}
		}
		return NULL;
	}
	jsid js_object_to_jsid(JSContext *cx,JSObject* jo)
	{
		jsid joid;
		JS_GetObjectId(cx,jo,&joid);
		return joid;
	}

	struct pack
	{
		engine::class_destoryer dtr;
		void* pthis;
		engine* env;

		pack(engine::class_destoryer da,engine* _env,void* dt):dtr(da),pthis(dt),env(_env)
		{
		}
	};

	lyramilk::data::var jsval2var(JSContext* cx,jsval jv)
	{
		lyramilk::data::var v;
		if(JSVAL_IS_DOUBLE(jv)){
			v.assign(JSVAL_TO_DOUBLE(jv));
		}else if(JSVAL_IS_INT(jv)){
			v.assign(JSVAL_TO_INT(jv));
		}else if(JSVAL_IS_VOID(jv)){
			
		}else if(JSVAL_IS_BOOLEAN(jv)){
			v.assign(JSVAL_TO_BOOLEAN(jv));
		}else if(JSVAL_IS_STRING(jv)){
			JSString* jstr = JSVAL_TO_STRING(jv);
			size_t len = 0;
			const jschar* cstr = JS_GetStringCharsZAndLength(cx,jstr,&len);

			lyramilk::data::wstring str;
			for(size_t i =0;i<len;++i){
				wchar_t c = cstr[i];
				str.push_back(c);
			}
			v.assign(str);
		}else if(JSVAL_IS_NULL(jv)){
		}else if(jv.isObjectOrNull()){
			JSObject *jo = JSVAL_TO_OBJECT(jv);
			if(JS_IsArrayObject(cx,jo)){
				lyramilk::data::var::array r;
				JSObject *iter = JS_NewPropertyIterator(cx,jo);
				if(iter){
					jsid jid;
					while(JS_NextProperty(cx,iter,&jid) && !JSID_IS_VOID(jid)){
						jsval jv;
						JS_LookupPropertyById(cx,jo,jid,&jv);
						lyramilk::data::var cval = jsval2var(cx,jv);
						r.push_back(cval);
					}
				}
				v = r;
			}else{
				pack *ppack = (pack*)js_get_property_ptr(cx,jo,"__script_native");
				if(ppack && ppack->pthis){
					jsid joid;
					JS_GetObjectId(cx,jo,&joid);
					v.assign("__script_object_id",(void*)joid);
					v.userdata("__script_native_object",ppack->pthis);
					return v;
				}
				jsval jv;
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
						jsval jk;
						JS_IdToValue(cx,jid,&jk);

						jsval jv;
						JS_LookupPropertyById(cx,jo,jid,&jv);

						lyramilk::data::var ckey = jsval2var(cx,jk);
						lyramilk::data::var cval = jsval2var(cx,jv);
						m.insert(std::pair<lyramilk::data::var,lyramilk::data::var>(ckey,cval));
					}
				}
				v = m;
			}
		}else{
			JSType jt = JS_TypeOfValue(cx,jv);
			const char* tn = JS_GetTypeName(cx,jt);
			std::cout << "[jsval2var]未知类型:" << tn << std::endl;
		}
		return v;
	}

	jsval var2jsval(JSContext* cx,lyramilk::data::var v)
	{
		switch(v.type()){
		  case lyramilk::data::var::t_bool:
			return BOOLEAN_TO_JSVAL((bool)v);
		  case lyramilk::data::var::t_int8:
		  case lyramilk::data::var::t_int16:
		  case lyramilk::data::var::t_int32:
		  case lyramilk::data::var::t_uint8:
		  case lyramilk::data::var::t_uint16:
		  case lyramilk::data::var::t_uint32:
			return INT_TO_JSVAL(v);
		  case lyramilk::data::var::t_int64:
		  case lyramilk::data::var::t_uint64:
		  case lyramilk::data::var::t_double:
			return DOUBLE_TO_JSVAL(v);
		  case lyramilk::data::var::t_bin:{
				JSObject* jo = JS_NewObject(cx,&varClass,NULL,JS_GetGlobalObject(cx));
				lyramilk::data::var* pv = new lyramilk::data::var;
				*pv = v;
				jsval jv = PRIVATE_TO_JSVAL(pv);
				if(JS_TRUE != JS_SetProperty(cx,jo,"__script_var",&jv)){
					delete pv;
					return JSVAL_NULL;
				}
				return OBJECT_TO_JSVAL(jo);
			}
		  case lyramilk::data::var::t_wstr:
		  case lyramilk::data::var::t_str:{
				lyramilk::data::wstring s = v;

				std::vector<jschar> jcstr;
				lyramilk::data::wstring::iterator it = s.begin();
				for(;it!=s.end();++it){
					jcstr.push_back((jschar)*it);
				}

				JSString* str = JS_NewUCStringCopyN(cx,jcstr.data(),jcstr.size());
				return STRING_TO_JSVAL(str);
			}break;
		  case lyramilk::data::var::t_array:{
				std::vector<jsval> jvs;
				lyramilk::data::var::array ar = v;
				lyramilk::data::var::array::iterator it = ar.begin();
				for(;it!=ar.end();++it){
					jvs.push_back(var2jsval(cx,*it));
				}
				JSObject* jo = JS_NewArrayObject(cx,jvs.size(),jvs.data());
				return OBJECT_TO_JSVAL(jo);
			}break;
		  case lyramilk::data::var::t_map:{
				JSObject* jo = JS_NewObject(cx,&normalClass,NULL,JS_GetGlobalObject(cx));
				lyramilk::data::var::map m = v;
				lyramilk::data::var::map::iterator it = m.begin();
				for(;it!=m.end();++it){
					lyramilk::data::string str = it->first;
					lyramilk::data::var v = it->second;
					jsval jv = var2jsval(cx,v);
					JS_SetProperty(cx,jo,str.c_str(),&jv);
				}
				//
				return OBJECT_TO_JSVAL(jo);
			}break;
		  case lyramilk::data::var::t_invalid:
			return JSVAL_VOID;
		  case lyramilk::data::var::t_user:
			const void* up = v.userdata("__script_object_id");
			if(up){
				JSObject* jsobj = JSID_TO_OBJECT((jsid)up);
				return OBJECT_TO_JSVAL(jsobj);
			}

			assert((((long)(&v))&1) == 0);
			return PRIVATE_TO_JSVAL(&v);
//		  default:;
		}
		return JSVAL_VOID;
	}

	lyramilk::data::string typeofjsval(JSContext *cx,jsval* p)
	{
		JSType jt = JS_TypeOfValue(cx,*p);
		const char* tn = JS_GetTypeName(cx,jt);
		return tn;
	}

	JSBool jsctr(JSContext *cx, unsigned argc, jsval *vp)
	{
		JS::RootedObject callee(cx, JSVAL_TO_OBJECT(JS_CALLEE(cx, vp)));

		void *pfuncnew = js_get_property_ptr(cx,callee,"__new");
		void *pfuncdel = js_get_property_ptr(cx,callee,"__delete");
		void *penv = js_get_property_ptr(cx,callee,"__env");
		void *pnewobj = NULL;

		engine::class_builder pfun = (engine::class_builder)pfuncnew;
		if(pfun){
			lyramilk::data::var::array params;
			if(argc>0){
				jsval* jv = JS_ARGV(cx,vp);
				for(unsigned i=0;i<argc;++i){
					lyramilk::data::var v = jsval2var(cx,jv[i]);
					params.push_back(v);
				}
			}
			pnewobj = pfun(params);
		}

		JSObject* jsret = JS_NewObject(cx,&nativeClass,callee,JS_GetGlobalObject(cx));

		pack *ppack = new pack((engine::class_destoryer)pfuncdel,(engine*)penv,pnewobj);
		jsval jv = PRIVATE_TO_JSVAL(ppack);
		if(JS_TRUE != JS_SetProperty(cx,jsret,"__script_native",&jv)){
			delete ppack;
			return JS_FALSE;
		}
		JS_SET_RVAL(cx,vp,OBJECT_TO_JSVAL(jsret));
		return JS_TRUE;
	}

	void jsdtr(JSFreeOp *fop, JSObject *obj)
	{
		/*
		JSContext* cx = NULL;
		for(;JS_ContextIterator(rt,&cx);){

		}
		*/
		JSRuntime* rt = fop->runtime();
		JSContext *cx = (JSContext *)JS_GetRuntimePrivate(rt);

		jsval jv;
		if(JS_TRUE == JS_GetProperty(cx,obj,"__script_native",&jv)){
			if(!JSVAL_IS_VOID(jv)){
				void* p = JSVAL_TO_PRIVATE(jv);
				pack *ppack = (pack*)p;
				if(ppack && ppack->pthis && ppack->dtr){
					ppack->dtr(ppack->pthis);
					delete ppack;
				}
			}
		}
	}
	void jsdtrvar(JSFreeOp *fop, JSObject *obj)
	{
		JSRuntime* rt = fop->runtime();
		JSContext *cx = (JSContext *)JS_GetRuntimePrivate(rt);

		jsval jv;
		if(JS_TRUE == JS_GetProperty(cx,obj,"__script_var",&jv)){
			if(!JSVAL_IS_VOID(jv)){
				void* p = JSVAL_TO_PRIVATE(jv);
				delete (lyramilk::data::var*)p;
			}
		}
	}

	JSBool js_func_adapter(JSContext *cx, unsigned argc, jsval *vp)
	{
		JS::RootedObject callee(cx, JSVAL_TO_OBJECT(JS_CALLEE(cx, vp)));
		JS::RootedObject jsobj(cx,JS_THIS_OBJECT(cx,vp));

		void* pfunc = js_get_property_ptr(cx,callee,"__function_pointer");
		pack *ppack = (pack*)js_get_property_ptr(cx,jsobj,"__script_native");
		jsval* jv = JS_ARGV(cx,vp);

		engine::functional_type pfun = (engine::functional_type)pfunc;
		if(pfun && ppack && ppack->pthis){
			lyramilk::data::var::array params;
			for(unsigned i=0;i<argc;++i){
				lyramilk::data::var v = jsval2var(cx,jv[i]);
				params.push_back(v);
			}
			lyramilk::data::var::map env;
			env["this"].assign("this",ppack->pthis);
			env["env"].assign("env",ppack->env);

			try{
				lyramilk::data::var ret = pfun(params,env);
				jsval jsret = var2jsval(cx,ret);
				JS_SET_RVAL(cx,vp,jsret);
			}catch(lyramilk::data::string& str){
				jsval args = var2jsval(cx,str.c_str());
				jsval exc;
				//只用一个参数就可以显示js中的错误了。
				if (JS_CallFunctionName(cx, JS_GetGlobalObject(cx), "Error", 1, &args, &exc)){
					JS_SetPendingException(cx, exc);
				}
				return JS_FALSE;
			}catch(const char* cstr){
				jsval args = var2jsval(cx,cstr);
				jsval exc;
				if (JS_CallFunctionName(cx, JS_GetGlobalObject(cx), "Error", 1, &args, &exc)){
					JS_SetPendingException(cx, exc);
				}
				return JS_FALSE;
			}catch(std::exception& e){
				jsval args = var2jsval(cx,e.what());
				jsval exc;
				//只用一个参数就可以显示js中的错误了。
				if (JS_CallFunctionName(cx, JS_GetGlobalObject(cx), "Error", 1, &args, &exc)){
					JS_SetPendingException(cx, exc);
				}
				return JS_FALSE;
			}catch(...){
				jsval args = var2jsval(cx,"未知异常");
				jsval exc;
				//只用一个参数就可以显示js中的错误了。
				if (JS_CallFunctionName(cx, JS_GetGlobalObject(cx), "Error", 1, &args, &exc)){
					JS_SetPendingException(cx, exc);
				}
				return JS_FALSE;
			}
		}

		return JS_TRUE;
	}

	JSBool js_func_adapter_noclass(JSContext *cx, unsigned argc, jsval *vp)
	{
		JS::RootedObject callee(cx, JSVAL_TO_OBJECT(JS_CALLEE(cx, vp)));
		//JSObject *jsobj = JS_THIS_OBJECT(cx,vp);

		void* pfunc = js_get_property_ptr(cx,callee,"__function_pointer");
		//pack *ppack = (pack*)JS_GetPrivate(cx,jsobj);
		script_js *penv = (script_js*)js_get_property_ptr(cx,callee,"__env_pointer");
		jsval* jv = JS_ARGV(cx,vp);

		engine::functional_type pfun = (engine::functional_type)pfunc;
		if(pfun){
			lyramilk::data::var::array params;
			for(unsigned i=0;i<argc;++i){
				lyramilk::data::var v = jsval2var(cx,jv[i]);
				params.push_back(v);
			}
			lyramilk::data::var::map env;
			env["env"].assign("env",penv);

			try{
				lyramilk::data::var ret = pfun(params,env);
				jsval jsret = var2jsval(cx,ret);
				JS_SET_RVAL(cx,vp,jsret);
			}catch(lyramilk::data::string& str){
				jsval args = var2jsval(cx,str.c_str());
				jsval exc;
				//只用一个参数就可以显示js中的错误了。
				if (JS_CallFunctionName(cx, JS_GetGlobalObject(cx), "Error", 1, &args, &exc)){
					JS_SetPendingException(cx, exc);
				}
				return JS_FALSE;
			}catch(const char* cstr){
				jsval args = var2jsval(cx,cstr);
				jsval exc;
				if (JS_CallFunctionName(cx, JS_GetGlobalObject(cx), "Error", 1, &args, &exc)){
					JS_SetPendingException(cx, exc);
				}
				return JS_FALSE;
			}catch(std::exception& e){
				jsval args = var2jsval(cx,e.what());
				jsval exc;
				//只用一个参数就可以显示js中的错误了。
				if (JS_CallFunctionName(cx, JS_GetGlobalObject(cx), "Error", 1, &args, &exc)){
					JS_SetPendingException(cx, exc);
				}
				return JS_FALSE;
			}catch(...){
				jsval args = var2jsval(cx,"未知异常");
				jsval exc;
				//只用一个参数就可以显示js中的错误了。
				if (JS_CallFunctionName(cx, JS_GetGlobalObject(cx), "Error", 1, &args, &exc)){
					JS_SetPendingException(cx, exc);
				}
				return JS_FALSE;
			}
		}

		return JS_TRUE;
	}

	script_js::script_js():rt(NULL),cx(NULL),script(NULL)
	{
		rt = JS_Init(8L*1024L*1024L);
		cx = JS_NewContext(rt, 8192);

		JS_SetRuntimePrivate(rt,cx);

		JS_SetCStringsAreUTF8();
		//JS_SetOptions(cx, JSOPTION_VAROBJFIX | JSOPTION_JIT | JSOPTION_METHODJIT);
		JS_SetOptions(cx, JSOPTION_VAROBJFIX | JSOPTION_METHODJIT);
		JS_SetVersion(cx, JSVERSION_LATEST);

		JS_SetErrorReporter(cx, script_js_error_report);

		JSObject * glob = JS_NewGlobalObject(cx, &globalClass, NULL);

		JS::RootedObject global(cx, glob);
		JSAutoCompartment ac(cx, global);
		JS_InitStandardClasses(cx,global);
	}

	script_js::~script_js()
	{
		gc();
		JS_DestroyContext(cx);
		JS_DestroyRuntime(rt);
		JS_ShutDown();
	}

	bool script_js::load_string(lyramilk::data::string scriptstring)
	{
		JS::RootedObject global(cx,JS_GetGlobalObject(cx));
		JSAutoCompartment ac(cx, global);
		script = JS_CompileScript(cx,global,scriptstring.c_str(),scriptstring.size(),NULL,1);
		return script != NULL;
	}

	bool script_js::load_file(lyramilk::data::string scriptfile)
	{
		JS::RootedObject global(cx,JS_GetGlobalObject(cx));
		JSAutoCompartment ac(cx, global);

		lyramilk::data::string scriptstring;
		std::ifstream ifs;
		ifs.open(scriptfile.c_str(),std::ifstream::binary | std::ifstream::in);

		while(ifs){
			char buff[4096];
			ifs.read(buff,4096);
			scriptstring.append(buff,ifs.gcount());
		}
		ifs.close();

		script = JS_CompileScript(cx,global,scriptstring.c_str(),scriptstring.size(),scriptfile.c_str(),1);
		return script != NULL;
	}

	lyramilk::data::var script_js::pcall(lyramilk::data::var::array args)
	{
		JS::RootedObject global(cx,JS_GetGlobalObject(cx));
		JSAutoCompartment ac(cx, global);
		bool ret = JS_ExecuteScript(cx,global,script,NULL) != JS_TRUE;
		gc();
		return ret;
	}

	lyramilk::data::var script_js::call(lyramilk::data::string func,lyramilk::data::var::array args)
	{
		JS::RootedObject global(cx,JS_GetGlobalObject(cx));
		JSAutoCompartment ac(cx, global);

		jsval retval;
		std::vector<jsval> jvs;
		for(lyramilk::data::var::array::iterator it = args.begin();it!=args.end();++it){
			lyramilk::data::var& v = *it;
			jvs.push_back(var2jsval(cx,v));
		}
		if(JS_TRUE == JS_CallFunctionName(cx,global,func.c_str(),jvs.size(),jvs.data(),&retval)){
			lyramilk::data::var ret = jsval2var(cx,retval);
			JS_GC(rt);
			return ret;
		}
		return lyramilk::data::var::nil;
	}

	void script_js::reset()
	{
		TODO();
	}

	void script_js::define(lyramilk::data::string classname,functional_map m,class_builder builder,class_destoryer destoryer)
	{
		JS::RootedObject global(cx,JS_GetGlobalObject(cx));
		JSAutoCompartment ac(cx, global);

		std::cout << "注册类：" << classname << ",构造:" << (void*)builder << ",释放" << (void*)destoryer << std::endl;
		JSObject* jo = JS_DefineObject(cx,global,classname.c_str(),&classesClass,NULL,0);
		assert(jo);
		jsid joid;
		JS_GetObjectId(cx,jo,&joid);
		this->m[classname] = joid;

		js_set_property_ptr(cx,jo,"__new",(void*)builder);
		js_set_property_ptr(cx,jo,"__delete",(void*)destoryer);
		js_set_property_ptr(cx,jo,"__env",(void*)this);
		
		functional_map::iterator it = m.begin();
		for(;it!=m.end();++it){
			std::cout << "\t注册函数：" << it->first << "," << (void*)it->second << std::endl;
			JSFunction *f = JS_DefineFunction(cx,jo,it->first.c_str(),js_func_adapter,10,10);
			assert(f);
			JSObject *fo = JS_GetFunctionObject(f);
			assert(fo);
			js_set_property_ptr(cx,fo,"__function_pointer",(void*)it->second);
		}
	}
	
	void script_js::define(lyramilk::data::string funcname,functional_type func)
	{
		JS::RootedObject global(cx,JS_GetGlobalObject(cx));
		JSAutoCompartment ac(cx, global);

		JSFunction *f = JS_DefineFunction(cx,global,funcname.c_str(),js_func_adapter_noclass,10,10);
		JSObject *fo = JS_GetFunctionObject(f);
		js_set_property_ptr(cx,fo,"__function_pointer",(void*)func);
		js_set_property_ptr(cx,fo,"__env_pointer",(void*)this);
	}

	lyramilk::data::var script_js::createobject(lyramilk::data::string classname,lyramilk::data::var::array args)
	{
		JS::RootedObject global(cx,JS_GetGlobalObject(cx));
		JSAutoCompartment ac(cx, global);

		std::map<lyramilk::data::string,jsid>::iterator it = m.find(classname);
		if(it==m.end()) return lyramilk::data::var::nil;

		JSObject* jsobj = JSID_TO_OBJECT(it->second);

		void *pfuncnew = js_get_property_ptr(cx,jsobj,"__new");
		void *pfuncdel = js_get_property_ptr(cx,jsobj,"__delete");
		void *pnewobj = NULL;
		engine::class_builder pfun = (engine::class_builder)pfuncnew;
		if(!pfun) return lyramilk::data::var::nil;

		pnewobj = pfun(args);
		JSObject* jsret = JS_NewObject(cx,&nativeClass,jsobj,global);
		pack *ppack = new pack((engine::class_destoryer)pfuncdel,this,pnewobj);
		//JS_SetPrivate(jsret,ppack);
		jsval jv = PRIVATE_TO_JSVAL(ppack);
		if(JS_TRUE != JS_SetProperty(cx,jsret,"__script_native",&jv)){
			delete ppack;
		}

		jsid jsretid;
		JS_GetObjectId(cx,jsret,&jsretid);
		lyramilk::data::var v("__script_object_id",(const void*)jsretid);
		v.userdata("__script_native_object",pnewobj);
		return v;
	}

	void script_js::gc()
	{
		//JSAutoCompartment ac(cx, JS_GetGlobalObject(cx));
		JS_GC(rt);
	}

}}}
