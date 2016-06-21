﻿#include "script_js.h"
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


//typedef JSBool (* JSNative)(JSContext *cx, uintN argc, jsval *vp);
//extern JS_PUBLIC_API(JSBool) JS_NextProperty(JSContext *cx, JSObject *iterobj, jsid *idp);
	JSBool jsctr(JSContext *cx, unsigned argc, JS::Value *vp);



	void jsdtr(JSFreeOp *fop, JSObject *obj);
	void jsdtrvar(JSFreeOp *fop, JSObject *obj);
	JSBool jsrmprop(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

	static JSClass globalClass = { "global", JSCLASS_GLOBAL_FLAGS,
		JS_PropertyStub,JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
		JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub};

	static JSClass classesClass = { "classes", 0,
		JS_PropertyStub,JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
		JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,NULL,NULL,NULL,NULL,jsctr};

	static JSClass nativeClass = { "native", 0,
		JS_PropertyStub,JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
		JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,jsdtr};

	static JSClass normalClass = { "normal", 0,
		JS_PropertyStub,JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
		JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub};

	static JSClass varClass = { "lyramilk::data::var", 0,
		JS_PropertyStub,JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
		JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,jsdtrvar};

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

	lyramilk::data::var jsval2var(JSContext* cx,jsval jv)
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
				size_t len = 0;
				const jschar* cstr = JS_GetStringCharsZAndLength(cx,jstr,&len);

				lyramilk::data::wstring str;
				for(size_t i =0;i<len;++i){
					wchar_t c = cstr[i];
					str.push_back(c);
				}
				return str;
			}
		  case JSVAL_TYPE_NULL:
			return lyramilk::data::var::nil;
		  case JSVAL_TYPE_OBJECT:{
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
				JSObject* jo = JS_NewObject(cx,&varClass,NULL,JS_GetScriptedGlobal(cx));
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
				JSObject* jo = JS_NewObject(cx,&normalClass,NULL,JS_GetScriptedGlobal(cx));
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
			return JSVAL_NULL;
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

	void displayJSVal(JSContext* cx,jsval jv)
	{
		/*
		if(JSVAL_IS_DOUBLE(jv)){
			std::cout << "[displayJSObject]double=" << JSVAL_TO_DOUBLE(jv) << std::endl;
		}else if(JSVAL_IS_INT(jv)){
			std::cout << "[displayJSObject]int=" << JSVAL_TO_INT(jv) << std::endl;
		}else if(JSVAL_IS_VOID(jv)){
			std::cout << "[displayJSObject]undefine" << std::endl;
		}else if(JSVAL_IS_BOOLEAN(jv)){
			std::cout << "[displayJSObject]bool=" << JSVAL_TO_BOOLEAN(jv) << std::endl;
		}else if(JSVAL_IS_STRING(jv)){
			JSString* jstr = JSVAL_TO_STRING(jv);
			size_t len = 0;
			const jschar* cstr = JS_GetStringCharsZAndLength(cx,jstr,&len);
			lyramilk::data::wstring str;
			for(size_t i =0;i<len;++i){
				wchar_t c = cstr[i];
				str.push_back(c);
			}
			std::wcout << L"[displayJSObject]string=" << str << std::endl;
		}else if(JSVAL_IS_NULL(jv)){
			std::cout << "[displayJSObject]null" << std::endl;
		}else if(JSVAL_IS_OBJECT(jv)){
			JSObject *jo = JSVAL_TO_OBJECT(jv);
			if(JS_IsArrayObject(cx,jo)){
				std::cout << "[displayJSObject]object(array) " << std::endl;
			}else if(JS_ObjectIsFunction(cx,jo)){
				std::cout << "[displayJSObject]object(function) " << std::endl;
			}else if(JS_ObjectIsDate(cx,jo)){
				std::cout << "[displayJSObject]object(date) " << std::endl;
			}else if(JS_ObjectIsCallable(cx,jo)){
				std::cout << "[displayJSObject]object(callbale) " << std::endl;
			}else{
				std::cout << "[displayJSObject]object(other) " << std::endl;
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
						std::cout << "key=" << ckey << "(" << ckey.type_name() << ")" << ",value=" << cval << "(" << cval.type_name() << ")" << std::endl;
					}
				}
			}
		}else{
			JSType jt = JS_TypeOfValue(cx,jv);
			const char* tn = JS_GetTypeName(cx,jt);
			std::cout << "[displayJSObject]unknown(" << tn << ") " << std::endl;
		}*/
	}
	void displayJSObject(JSContext* cx,JSObject* jo)
	{
		jsval jv = OBJECT_TO_JSVAL(jo);
		displayJSVal(cx,jv);
	}

	lyramilk::data::string typeofjsval(JSContext *cx,jsval* p)
	{
		JSType jt = JS_TypeOfValue(cx,*p);
		const char* tn = JS_GetTypeName(cx,jt);
		return tn;
	}

	void js_set_property_ptr(JSContext *cx,JSObject* o,lyramilk::data::string name,void* ptr)
	{
		lyramilk::data::string name_flag = name + "__flag";
		int offset = ((long)ptr) & 1;
		if(offset){
//std::cout << "注册偏移" << name_flag << "," << offset << std::endl;
			jsval jvflag = PRIVATE_TO_JSVAL((void*)2);
			JS_SetProperty(cx,o,name_flag.c_str(),&jvflag);
		}else{
			jsval jvflag = PRIVATE_TO_JSVAL((void*)0);
			JS_SetProperty(cx,o,name_flag.c_str(),&jvflag);
		}
//std::cout << "注册指针" << name << "," << ptr << std::endl;
		jsval jv = PRIVATE_TO_JSVAL(ptr);
		JS_SetProperty(cx,o,name.c_str(),&jv);
	}

	void* js_get_property_ptr(JSContext *cx,JSObject* o,lyramilk::data::string name)
	{
		lyramilk::data::string name_flag = name + "__flag";
		jsval __flag;
		long offset = 0;
		if(JS_GetProperty(cx,o,name_flag.c_str(),&__flag)){
			void* flag = JSVAL_TO_PRIVATE(__flag);
			offset = ((long)(flag)>>1);
//std::cout << "获得偏移" << name_flag << ",offset=" << offset << std::endl;
		}
		jsval __prop;
		if(JS_GetProperty(cx,o,name.c_str(),&__prop)){
			const char* p = (const char*)JSVAL_TO_PRIVATE(__prop);
//std::cout << "获得指针" << name << "," << (void*)&p[offset] << std::endl;
			return (void*)&p[offset];
		}
		return NULL;
	}




	JSBool jsctr(JSContext *cx, unsigned argc, JS::Value *vp)
	{
		JSObject* jsobj = JSVAL_TO_OBJECT(JS_CALLEE(cx,vp));

		void *pfuncnew = js_get_property_ptr(cx,jsobj,"__new");
		void *pfuncdel = js_get_property_ptr(cx,jsobj,"__delete");
		void *penv = js_get_property_ptr(cx,jsobj,"__env");
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

		JSObject* jsret = JS_NewObject(cx,&nativeClass,jsobj,JS_GetScriptedGlobal(cx));
		JS_SetPrivate(jsret,new pack((engine::class_destoryer)pfuncdel,(engine*)penv,pnewobj));
		JS_SET_RVAL(cx,vp,OBJECT_TO_JSVAL(jsret));
		return JS_TRUE;
	}

	void jsdtr(JSFreeOp *fop, JSObject *obj)
	{
		pack *ppack = (pack*)JS_GetPrivate(obj);
		if(ppack && ppack->pthis && ppack->dtr){
			ppack->dtr(ppack->pthis);
			delete ppack;
		}
	}
	void jsdtrvar(JSFreeOp *fop, JSObject *obj)
	{
		JSRuntime *rt = fop->runtime();
		JSContext *acx;
		JSContext *iterp = NULL;
		while ((acx = JS_ContextIterator(rt, &iterp)) != NULL){
			jsval jv;
			if(JS_TRUE == JS_GetProperty(acx,obj,"__script_var",&jv)){
				if(!JSVAL_IS_VOID(jv)){
					void* p = JSVAL_TO_PRIVATE(jv);
					delete (lyramilk::data::var*)p;
					break;
				}
			}
		}
	}

	JSBool js_func_adapter(JSContext *cx, unsigned argc, JS::Value *vp)
	{
		JSObject *root = JSVAL_TO_OBJECT(JS_CALLEE(cx,vp));
		JSObject *jsobj = JS_THIS_OBJECT(cx,vp);

		void* pfunc = js_get_property_ptr(cx,root,"__function_pointer");
		pack *ppack = (pack*)JS_GetPrivate(jsobj);
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
			lyramilk::data::var ret = pfun(params,env);
			jsval jsret = var2jsval(cx,ret);
			JS_SET_RVAL(cx,vp,jsret);
		}

		return JS_TRUE;
	}

	script_js::script_js():rt(NULL),cx(NULL),script(NULL)
	{
		rt = JS_NewRuntime(8L*1024L*1024L,JS_NO_HELPER_THREADS);

		cx = JS_NewContext(rt, 8192);

		//JS_SetCStringsAreUTF8();

		//JS_SetOptions(cx, JSOPTION_VAROBJFIX | JSOPTION_JIT | JSOPTION_METHODJIT);
		JS_SetOptions(cx, JSOPTION_VAROBJFIX);
		//JS_SetVersion(cx, JSVERSION_LATEST);

		JS_SetErrorReporter(cx, script_js_error_report);

		//global = JS_NewCompartmentAndGlobalObject(cx,&globalClass,0);

//		JSAutoRequest __ar(cx);
		//JS::RootedObject global(cx);
		//JS_NewObject(cx,&nativeClass,jsobj,JS_GetScriptedGlobal(cx));
COUT << "JS_NewGlobalObject.0 " << cx << std::endl;
		global = JS_NewGlobalObject(cx,&globalClass,NULL);
COUT << "JS_NewGlobalObject.1 " << global << std::endl;
		JSObject* jo = JS_NewObject(cx,&nativeClass,global,global);
COUT << "JS_NewObject.1 " << jo << std::endl;
		JS_InitStandardClasses(cx,global);
COUT << "JS_InitStandardClasses.2" << std::endl;
	}

	script_js::~script_js()
	{
		JS_DestroyContext(cx);
		JS_DestroyRuntime(rt);
		JS_ShutDown();
	}

	bool script_js::load_string(lyramilk::data::string scriptstring)
	{
		script = JS_CompileScript(cx,global,scriptstring.c_str(),scriptstring.size(),NULL,1);
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

		script = JS_CompileScript(cx,global,scriptstring.c_str(),scriptstring.size(),scriptfile.c_str(),1);
		return script != NULL;
	}

	lyramilk::data::var script_js::pcall(lyramilk::data::var::array args)
	{
		bool ret = JS_ExecuteScript(cx,global,script,NULL) != JS_TRUE;
		gc();
		return ret;
	}

	lyramilk::data::var script_js::call(lyramilk::data::string func,lyramilk::data::var::array args)
	{
		jsval retval;
		std::vector<jsval> jvs;
		for(lyramilk::data::var::array::iterator it = args.begin();it!=args.end();++it){
			lyramilk::data::var& v = *it;
			jvs.push_back(var2jsval(cx,v));
		}
		if(JS_TRUE == JS_CallFunctionName(cx,global,func.c_str(),jvs.size(),jvs.data(),&retval)){
			lyramilk::data::var ret = jsval2var(cx,retval);
			//JS_GC(cx);
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
		//std::cout << "注册类：" << classname << ",构造:" << (void*)builder << ",释放" << (void*)destoryer << std::endl;
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
		std::map<lyramilk::data::string,jsid>::iterator it = m.find(classname);
		if(it==m.end()) return lyramilk::data::var::nil;

		JSObject* jsobj = JSID_TO_OBJECT(it->second);

		void *pfuncnew = js_get_property_ptr(cx,jsobj,"__new");
		void *pfuncdel = js_get_property_ptr(cx,jsobj,"__delete");
		void *pnewobj = NULL;
		engine::class_builder pfun = (engine::class_builder)pfuncnew;
		if(!pfun) return lyramilk::data::var::nil;

		pnewobj = pfun(args);
		JSObject* jsret = JS_NewObject(cx,&nativeClass,jsobj,JS_GetScriptedGlobal(cx));
		JS_SetPrivate(jsret,new pack((engine::class_destoryer)pfuncdel,this,pnewobj));

		jsid jsretid;
		JS_GetObjectId(cx,jsret,&jsretid);
		lyramilk::data::var v("__script_object_id",(const void*)jsretid);
		v.userdata("__script_native_object",pnewobj);
		return v;
	}

	void script_js::gc()
	{
		JS_GC(rt);
	}

}}}