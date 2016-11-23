#include "script_js.h"
#include "multilanguage.h"
#include "log.h"
//#include "testing.h"
#include <gc/Root.h>
#include <fstream>
#include <cassert>
#include <sys/stat.h>
#include <utime.h>
#include <math.h>
//script_js
#include <jsfriendapi.h>

namespace lyramilk{namespace script{namespace js
{
	JSBool static jsctr(JSContext *cx, unsigned argc, jsval *vp);
	void static jsdtr(JSFreeOp *fop, JSObject *obj);
	void static jsdtrvar(JSFreeOp *fop, JSObject *obj);
	JSBool static jsinstanceof(JSContext *cx, JSHandleObject obj, const jsval *v, JSBool *bp);

	static JSClass globalClass = { "global", JSCLASS_GLOBAL_FLAGS,
			JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
            JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub};

	static JSClass classesClass = { "classes", 0,
			JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
			JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,
			NULL,NULL,NULL,jsinstanceof,jsctr};

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

	static void script_js_error_report(JSContext *cx, const char *message, JSErrorReport *report)
	{
		lyramilk::klog(lyramilk::log::warning,"lyramilk.script.js") << lyramilk::kdict("%s(%d)%s",(report->filename ? report->filename : "<no name>"),report->lineno,message) << std::endl;
	}

	static bool js_set_property_ptr(JSContext *cx,JSObject* o,lyramilk::data::string name,void* ptr)
	{
		/*
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
		/*/

		jsval jv;
		jv.setUnmarkedPtr(ptr);
		return !!JS_SetProperty(cx,o,name.c_str(),&jv);
		/**/
	}

	static void* js_get_property_ptr(JSContext *cx,JSObject* o,lyramilk::data::string name)
	{
		/*
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

		return nullptr;
		
		/*/

		jsval __prop;
		__prop.setUndefined();
		if(JS_GetProperty(cx,o,name.c_str(),&__prop)){
			if(__prop.isNullOrUndefined()) return nullptr;
			return __prop.toUnmarkedPtr();
		}
		return nullptr;
		/**/
	}

	/*
	static jsid js_object_to_jsid(JSContext *cx,JSObject* jo)
	{
		jsid joid;
		JS_GetObjectId(cx,jo,&joid);
		return joid;
	}*/

	struct js_obj_pack
	{
		engine::class_destoryer dtr;
		void* pthis;
		engine* env;
		lyramilk::data::var::map info;

		js_obj_pack(engine::class_destoryer da,engine* _env,void* dt):dtr(da),pthis(dt),env(_env)
		{
			info[engine::s_env_this()].assign(engine::s_user_nativeobject(),pthis);
			info[engine::s_env_engine()].assign(engine::s_user_engineptr(),env);
		}
	};

	static void j2v(JSContext* cx,jsval jv,lyramilk::data::var& retv)
	{
		if(jv.isNullOrUndefined()){
			retv = lyramilk::data::var::nil;
			return;
		}else if(jv.isInt32()){
			retv = jv.toInt32();
			return;
		}else if(jv.isDouble()){
			retv = jv.toDouble();
			return;
		}else if(jv.isNumber()){
			retv = jv.toNumber();
			return;
		}else if(jv.isBoolean()){
			retv = jv.toBoolean();
			return;
		}else if(jv.isString()){
			JSString* jstr = jv.toString();
			size_t len = 0;
			const jschar* cstr = JS_GetStringCharsZAndLength(cx,jstr,&len);
/*
			int slen = len*(sizeof(jschar)/sizeof(char));
			lyramilk::data::string str;
			iconv((const char*)cstr,slen,str,"utf16le","utf8");
/*/
			lyramilk::data::string str;
			str.reserve(len*3);
			for(size_t i =0;i<len;++i){
				jschar wc = cstr[i];
				if(wc < 0x80){
					str.push_back((unsigned char)wc);
				}else if(wc < 0x800){
					str.push_back((unsigned char)((wc>>6)&0x1f) | 0xc0);
					str.push_back((unsigned char)((wc>>0)&0x3f) | 0x80);
				}else if(wc < 0x10000){
					str.push_back((unsigned char)((wc>>12)&0xf) | 0xe0);
					str.push_back((unsigned char)((wc>>6)&0x3f) | 0x80);
					str.push_back((unsigned char)((wc>>0)&0x3f) | 0x80);
				}
			}
/**/
			retv = str;
			return;
		}else if(jv.isObject()){
			JSObject *jo = jv.toObjectOrNull();
			if(JS_IsArrayObject(cx,jo)){
				retv.type(lyramilk::data::var::t_array);
				lyramilk::data::var::array& r = retv;
				uint32_t len = 0;
				if(!JS_GetArrayLength(cx,jo,&len)){
					retv = lyramilk::data::var::nil;
					return;
				}
				r.resize(len);
				for(uint32_t i=0;i<len;++i){
					jsval jv;
					JS_LookupElement(cx,jo,i,&jv);
					j2v(cx,jv,r[i]);
				}
			}else if(JS_ObjectIsFunction(cx,jo)){
				jsid jid = OBJECT_TO_JSID(jo);
				retv.assign(engine::s_user_functionid(),(void*)jid);
				return;
			}else{
				JSObject *iter = JS_NewPropertyIterator(cx,jo);
				if(iter){
					retv.type(lyramilk::data::var::t_map);
					lyramilk::data::var::map& m = retv;
					jsid jid;
					while(JS_NextProperty(cx,iter,&jid)){
						jsval jk,jv;
						JS_IdToValue(cx,jid,&jk);
						if(!jk.isString()) break;
						JS_LookupPropertyById(cx,jo,jid,&jv);

						lyramilk::data::string skey;
						{
							JSString* jstr = jk.toString();
							size_t len = 0;
							const jschar* cstr = JS_GetStringCharsZAndLength(cx,jstr,&len);
							skey.reserve(len*3);
							for(size_t i =0;i<len;++i){
								jschar wc = cstr[i];
								if(wc < 0x80){
									skey.push_back((unsigned char)wc);
								}else if(wc < 0x800){
									skey.push_back((unsigned char)((wc>>6)&0x1f) | 0xc0);
									skey.push_back((unsigned char)((wc>>0)&0x3f) | 0x80);
								}else if(wc < 0x10000){
									skey.push_back((unsigned char)((wc>>12)&0xf) | 0xe0);
									skey.push_back((unsigned char)((wc>>6)&0x3f) | 0x80);
									skey.push_back((unsigned char)((wc>>0)&0x3f) | 0x80);
								}
							}
						}

						if(skey == "__script_native"){
							js_obj_pack *ppack = nullptr;
							if(!JSVAL_IS_VOID(jv)){
								void* p = JSVAL_TO_PRIVATE(jv);
								ppack = (js_obj_pack*)p;
							}
							if(ppack && ppack->pthis){
								jsid joid;
								JS_GetObjectId(cx,jo,&joid);
								retv.assign(engine::s_user_objectid(),(void*)joid);
								retv.userdata(engine::s_user_nativeobject(),ppack->pthis);
								return;
							}
						}else if(skey == "__script_var"){
							if(!jv.isNullOrUndefined()){
								void* p = JSVAL_TO_PRIVATE(jv);
								if(p){
									retv = *(lyramilk::data::var*)p;
									return;
								}
							}
						}else{
							j2v(cx,jv,m[skey]);
						}
					}
				}
				return ;
			}
		}else{
			JSType jt = JS_TypeOfValue(cx,jv);
			const char* tn = JS_GetTypeName(cx,jt);
			COUT << tn << std::endl;
			TODO();
		}
	}

	jsval static v2j(JSContext* cx,lyramilk::data::var v)
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
		  case lyramilk::data::var::t_uint64:{
				double dv = v;
				if(floor(v) == dv){
					if(dv > JSVAL_INT_MIN && dv < JSVAL_INT_MAX){
						return INT_TO_JSVAL(v);
					}
				}
			}
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
			  /*
				lyramilk::data::wstring s = v;

				std::vector<jschar> jcstr;
				lyramilk::data::wstring::iterator it = s.begin();
				for(;it!=s.end();++it){
					jcstr.push_back((jschar)*it);
				}

				JSString* str = JS_NewUCStringCopyN(cx,jcstr.data(),jcstr.size());
				return STRING_TO_JSVAL(str);

				*/
				lyramilk::data::string s = v;
				JSString* str = JS_NewStringCopyN(cx,s.c_str(),s.size());
				return STRING_TO_JSVAL(str);
			}break;
		  case lyramilk::data::var::t_array:{
				std::vector<jsval> jvs;
				lyramilk::data::var::array& ar = v;
				lyramilk::data::var::array::iterator it = ar.begin();
				jvs.reserve(ar.size());
				for(;it!=ar.end();++it){
					jvs.push_back(v2j(cx,*it));
				}
				JSObject* jo = JS_NewArrayObject(cx,jvs.size(),jvs.data());
				return OBJECT_TO_JSVAL(jo);
			}break;
		  case lyramilk::data::var::t_map:{
				JSObject* jo = JS_NewObject(cx,&normalClass,NULL,JS_GetGlobalObject(cx));
				lyramilk::data::var::map& m = v;
				lyramilk::data::var::map::iterator it = m.begin();
				for(;it!=m.end();++it){
					lyramilk::data::string str = it->first;
					lyramilk::data::var v = it->second;
					jsval jv = v2j(cx,v);
					JS_SetProperty(cx,jo,str.c_str(),&jv);
				}
				//
				return OBJECT_TO_JSVAL(jo);
			}break;
		  case lyramilk::data::var::t_invalid:
			return JSVAL_VOID;
		  case lyramilk::data::var::t_user:
			const void* up = v.userdata(engine::s_user_objectid());
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
/*
	lyramilk::data::string static typeofjsval(JSContext *cx,jsval* p)
	{
		JSType jt = JS_TypeOfValue(cx,*p);
		const char* tn = JS_GetTypeName(cx,jt);
		return tn;
	}*/

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
				params.resize(argc);
				for(unsigned i=0;i<argc;++i){
					j2v(cx,jv[i],params[i]);
				}
			}

			try{
				pnewobj = pfun(params);
				if(pnewobj == nullptr){
					jsval args = v2j(cx,D("创建对象失败"));
					jsval exc;
					if (JS_CallFunctionName(cx, JS_GetGlobalObject(cx), "Error", 1, &args, &exc)){
						JS_SetPendingException(cx, exc);
					}
					return JS_FALSE;
				}
			}catch(lyramilk::data::string& str){
				jsval args = v2j(cx,str.c_str());
				jsval exc;
				//只用一个参数就可以显示js中的错误了。
				if (JS_CallFunctionName(cx, JS_GetGlobalObject(cx), "Error", 1, &args, &exc)){
					JS_SetPendingException(cx, exc);
				}
				return JS_FALSE;
			}catch(const char* cstr){
				jsval args = v2j(cx,cstr);
				jsval exc;
				if (JS_CallFunctionName(cx, JS_GetGlobalObject(cx), "Error", 1, &args, &exc)){
					JS_SetPendingException(cx, exc);
				}
				return JS_FALSE;
			}catch(std::exception& e){
				jsval args = v2j(cx,e.what());
				jsval exc;
				//只用一个参数就可以显示js中的错误了。
				if (JS_CallFunctionName(cx, JS_GetGlobalObject(cx), "Error", 1, &args, &exc)){
					JS_SetPendingException(cx, exc);
				}
				return JS_FALSE;
			}catch(...){
				jsval args = v2j(cx,"未知异常");
				jsval exc;
				//只用一个参数就可以显示js中的错误了。
				if (JS_CallFunctionName(cx, JS_GetGlobalObject(cx), "Error", 1, &args, &exc)){
					JS_SetPendingException(cx, exc);
				}
				return JS_FALSE;
			}
		}

		JSObject* jsret = JS_NewObject(cx,&nativeClass,callee,JS_GetGlobalObject(cx));

		js_obj_pack *ppack = new js_obj_pack((engine::class_destoryer)pfuncdel,(engine*)penv,pnewobj);
		/*
			虽然 PRIVATE_TO_JSVAL 会舍弃一个bit（相当于x&~1）但是因为结构体导致 ppack 不会落到奇数地址上，故不需要补位。
		*/
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
				js_obj_pack *ppack = (js_obj_pack*)p;
				if(ppack && ppack->pthis && ppack->dtr){
					ppack->dtr(ppack->pthis);
					delete ppack;
				}
			}
		}
	}

	JSBool jsinstanceof(JSContext *cx, JSHandleObject obj, const jsval *v, JSBool *bp)
	{
		//支持 instanceof操作符
		JSObject *joproto = JS_GetPrototype(JSVAL_TO_OBJECT(*v)); 
		jsid joid;
		JS_GetObjectId(cx,joproto,&joid);

		jsid joid2;
		JS_GetObjectId(cx,obj,&joid2);

		if(joid == joid2){
			*bp = JS_TRUE;
		}else{
			*bp = JS_FALSE;
		}
		return JS_TRUE;
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

	JSBool static js_func_adapter(JSContext *cx, unsigned argc, jsval *vp)
	{
		JS::RootedObject callee(cx, JSVAL_TO_OBJECT(JS_CALLEE(cx, vp)));
		JS::RootedObject jsobj(cx,JS_THIS_OBJECT(cx,vp));

		void* pfunc = js_get_property_ptr(cx,callee,"__function_pointer");


		js_obj_pack *ppack = nullptr;
		{
			jsval jv;
			if(JS_TRUE == JS_GetProperty(cx,jsobj,"__script_native",&jv)){
				if(!JSVAL_IS_VOID(jv)){
					void* p = JSVAL_TO_PRIVATE(jv);
					ppack = (js_obj_pack*)p;
				}
			}
		}
		jsval* jv = JS_ARGV(cx,vp);

		engine::functional_type pfun = (engine::functional_type)pfunc;
		if(pfun && ppack && ppack->pthis){
			lyramilk::data::var::array params;
			params.resize(argc);
			for(unsigned i=0;i<argc;++i){
				j2v(cx,jv[i],params[i]);
			}
			try{
				lyramilk::data::var ret = pfun(params,ppack->info);
				jsval jsret = v2j(cx,ret);
				JS_SET_RVAL(cx,vp,jsret);
				return JS_TRUE;
			}catch(lyramilk::data::string& str){
				jsval args = v2j(cx,str.c_str());
				jsval exc;
				//只用一个参数就可以显示js中的错误了。
				if (JS_CallFunctionName(cx, JS_GetGlobalObject(cx), "Error", 1, &args, &exc)){
					JS_SetPendingException(cx, exc);
				}
				return JS_FALSE;
			}catch(const char* cstr){
				jsval args = v2j(cx,cstr);
				jsval exc;
				if (JS_CallFunctionName(cx, JS_GetGlobalObject(cx), "Error", 1, &args, &exc)){
					JS_SetPendingException(cx, exc);
				}
				return JS_FALSE;
			}catch(std::exception& e){
				jsval args = v2j(cx,e.what());
				jsval exc;
				//只用一个参数就可以显示js中的错误了。
				if (JS_CallFunctionName(cx, JS_GetGlobalObject(cx), "Error", 1, &args, &exc)){
					JS_SetPendingException(cx, exc);
				}
				return JS_FALSE;
			}catch(...){
				jsval args = v2j(cx,"未知异常");
				jsval exc;
				//只用一个参数就可以显示js中的错误了。
				if (JS_CallFunctionName(cx, JS_GetGlobalObject(cx), "Error", 1, &args, &exc)){
					JS_SetPendingException(cx, exc);
				}
				return JS_FALSE;
			}
		}

		return JS_FALSE;
	}

	JSBool static js_func_adapter_noclass(JSContext *cx, unsigned argc, jsval *vp)
	{
		JS::RootedObject callee(cx, JSVAL_TO_OBJECT(JS_CALLEE(cx, vp)));

		void* pfunc = js_get_property_ptr(cx,callee,"__function_pointer");
		script_js *penv = (script_js*)js_get_property_ptr(cx,callee,"__env_pointer");
		jsval* jv = JS_ARGV(cx,vp);

		engine::functional_type pfun = (engine::functional_type)pfunc;
		if(pfun){
			lyramilk::data::var::array params;
			params.resize(argc);
			for(unsigned i=0;i<argc;++i){
				j2v(cx,jv[i],params[i]);
			}
			try{
				lyramilk::data::var ret = pfun(params,penv->info);
				jsval jsret = v2j(cx,ret);
				JS_SET_RVAL(cx,vp,jsret);
			}catch(lyramilk::data::string& str){
				jsval args = v2j(cx,str.c_str());
				jsval exc;
				//只用一个参数就可以显示js中的错误了。
				if (JS_CallFunctionName(cx, JS_GetGlobalObject(cx), "Error", 1, &args, &exc)){
					JS_SetPendingException(cx, exc);
				}
				return JS_FALSE;
			}catch(const char* cstr){
				jsval args = v2j(cx,cstr);
				jsval exc;
				if (JS_CallFunctionName(cx, JS_GetGlobalObject(cx), "Error", 1, &args, &exc)){
					JS_SetPendingException(cx, exc);
				}
				return JS_FALSE;
			}catch(std::exception& e){
				jsval args = v2j(cx,e.what());
				jsval exc;
				//只用一个参数就可以显示js中的错误了。
				if (JS_CallFunctionName(cx, JS_GetGlobalObject(cx), "Error", 1, &args, &exc)){
					JS_SetPendingException(cx, exc);
				}
				return JS_FALSE;
			}catch(...){
				jsval args = v2j(cx,"未知异常");
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

	script_js::script_js():rt(NULL),cx_template(NULL)
	{
		info[engine::s_env_engine()].assign(engine::s_user_engineptr(),this);

		rt = JS_NewRuntime(128*1024L*1024L);
		cx_template = JS_NewContext(rt, 8192);
		JS_SetCStringsAreUTF8();

		//JS_SetRuntimePrivate(rt,cx_template);
		JS_SetOptions(cx_template, JSOPTION_VAROBJFIX | JSOPTION_METHODJIT);
		JS_SetVersion(cx_template, JSVERSION_LATEST);
		JS_SetErrorReporter(cx_template, script_js_error_report);

		JSObject * glob = JS_NewGlobalObject(cx_template, &globalClass, NULL);
		JS::RootedObject global(cx_template, glob);
		JS_InitStandardClasses(cx_template,global);
	}

	script_js::~script_js()
	{
		gc();
		JS_SetRuntimeThread(rt);
		JSContext *cx = (JSContext *)JS_GetRuntimePrivate(rt);
		if(cx){
			JS_DestroyContext(cx);
			JS_SetRuntimePrivate(rt,nullptr);
			scriptfilename.clear();
		}
		JS_DestroyContext(cx_template);
		JS_DestroyRuntime(rt);
		//JS_ShutDown();
	}

	bool script_js::load_string(lyramilk::data::string scriptstring)
	{
		init();
		JS_SetRuntimeThread(rt);
		JSContext *cx = (JSContext *)JS_GetRuntimePrivate(rt);
		JS::RootedObject global(cx,JS_GetGlobalObject(cx));
		JSScript* script = JS_CompileScript(cx,global,scriptstring.c_str(),scriptstring.size(),NULL,1);
		if(script){
			return !!JS_ExecuteScript(cx,global,script,NULL);
		}
		return false;
	}

	//lyramilk::debug::nsecdiff td;
	/*
	lyramilk::debug::clocktester _d(td,log(lyramilk::log::debug),k);
	*/

	void inline fsync(lyramilk::data::string file1,lyramilk::data::string file2,bool* file1exist,bool* file2exist,bool* issync)
	{
		struct stat statbuf[2];
		if (stat (file1.c_str(), &statbuf[0]) == -1){
			*file1exist = false;
		}else{
			*file1exist = true;
		}
		if (stat (file2.c_str(), &statbuf[1]) == -1){
			*file2exist = false;
		}else{
			*file2exist = true;
		}
		if(*file1exist && *file2exist){
			*issync = statbuf[0].st_mtime == statbuf[1].st_mtime;
		}else{
			*issync = false;
		}
	}

#if 0
	bool script_js::load_file(lyramilk::data::string scriptfile)
	{
		init();
		JS_SetRuntimeThread(rt);
		JSContext *cx = (JSContext *)JS_GetRuntimePrivate(rt);
		JS::RootedObject global(cx,JS_GetGlobalObject(cx));

		lyramilk::data::string precompliefile = scriptfile;
		if(precompliefile.size() > 3){
			if(precompliefile.compare(precompliefile.size() - 3,3,".js",3) == 0){
				precompliefile.push_back('c');
			}else{
				precompliefile.append(".jsc");
			}
		}

		bool isprecomplie = false;
		JSScript* script = nullptr;

		bool f1,f2,fs;
		fsync(scriptfile,precompliefile,&f1,&f2,&fs);
		if(f1 && f2 && fs){
			/*
lyramilk::data::string str = "从字节码编译" + scriptfile;
lyramilk::debug::clocktester _d(td,std::cout,str);*/
			lyramilk::data::string jsbuff;
			std::ifstream ifs;
			ifs.open(precompliefile.c_str(),std::ifstream::binary|std::ifstream::in);
			if(ifs.is_open()){
				char buff[65536];
				while(ifs){
					ifs.read(buff,65536);
					jsbuff.append(buff,ifs.gcount());
				}
				ifs.close();
				if(!jsbuff.empty()){
					script = JS_DecodeScript(cx,jsbuff.c_str(),jsbuff.size(),nullptr,nullptr);
					if(script){
						isprecomplie = true;
					}
				}
			}
		}
		if(!isprecomplie){
			/*
lyramilk::data::string str = "完整编译" + scriptfile;
lyramilk::debug::clocktester _d(td,std::cout,str);*/
			script = JS_CompileUTF8File(cx,global,scriptfile.c_str());
		}

		if(script){
			bool ret = false;
			if(scriptfilename.empty()){
				scriptfilename = scriptfile;
				ret = JS_ExecuteScript(cx,global,script,nullptr) == JS_TRUE;
				if(!ret){
					scriptfilename.clear();
				}
			}else{
				ret = JS_ExecuteScript(cx,global,script,nullptr) == JS_TRUE;
			}
			if(ret && !isprecomplie){
				uint32_t l = 0;
				void* p = JS_EncodeScript(cx,script,&l);
				std::ofstream ofs;
				ofs.open(precompliefile.c_str(),std::ofstream::binary|std::ofstream::out);
				if(ofs.is_open()){
					ofs.write((const char*)p,l);
					ofs.close();
					struct stat statbuf;
					if (stat (scriptfile.c_str(), &statbuf) != -1){
						struct utimbuf tv;
						tv.actime = statbuf.st_ctime;
						tv.modtime = statbuf.st_mtime;
						utime(precompliefile.c_str(),&tv);
					}
				}
			}
			return ret;
		}
		return false;
	}
#else
	bool script_js::load_file(lyramilk::data::string scriptfile)
	{
		if(scriptfilename.empty()){
			scriptfilename = scriptfile;
		}
		/**
		JS_SetRuntimeThread(rt);
		JSContext *cx = (JSContext *)JS_GetRuntimePrivate(rt);
		if(!cx){
			struct stat statbuf;
			stat (scriptfile.c_str(), &statbuf);
			void* cx_timestamp = (void*)statbuf.st_mtime;

			std::map<lyramilk::data::string,JSContext*>::iterator it = fm.find(scriptfile);
			if(it!=fm.end()){
				void* timestamp = JS_GetContextPrivate(it->second);
				if(cx_timestamp == timestamp){
					JS_SetRuntimePrivate(rt,it->second);
					cx = it->second;
					return true;
				}
				fm.erase(it);
				JS_DestroyContext(it->second);
			}
			init();
			cx = (JSContext *)JS_GetRuntimePrivate(rt);
			if(!cx) return false;
			JS_SetContextPrivate(cx,cx_timestamp);
			fm[scriptfile] = cx;
		}
		/*/
		//非重用
		init();
		JS_SetRuntimeThread(rt);
		JSContext *cx = (JSContext *)JS_GetRuntimePrivate(rt);
		/**/
		JS::RootedObject global(cx,JS_GetGlobalObject(cx));

		JSScript* script = nullptr;
		script = JS_CompileUTF8File(cx,global,scriptfile.c_str());
		if(!script)return false;
		return !!JS_ExecuteScript(cx,global,script,nullptr);
	}
#endif
	lyramilk::data::var script_js::call(lyramilk::data::var func,lyramilk::data::var::array args)
	{
		init();
		JS_SetRuntimeThread(rt);
		JSContext *cx = (JSContext *)JS_GetRuntimePrivate(rt);
		JS::RootedObject global(cx,JS_GetGlobalObject(cx));

		if(func.type_compat(lyramilk::data::var::t_str)){
			jsval retval;
			std::vector<jsval> jvs;
			for(lyramilk::data::var::array::iterator it = args.begin();it!=args.end();++it){
				lyramilk::data::var& v = *it;
				jvs.push_back(v2j(cx,v));
			}

			if(JS_TRUE == JS_CallFunctionName(cx,global,func.str().c_str(),jvs.size(),jvs.data(),&retval)){
				lyramilk::data::var ret;
				j2v(cx,retval,ret);
				return ret;
			}
		}else if(func.type() == lyramilk::data::var::t_user){
			//调用回调函数
			jsid jid = (jsid)func.userdata(engine::s_user_functionid());
			if(!jid) return lyramilk::data::var::nil;
			jsval retval;
			jsval jv;
			JS_IdToValue(cx,jid,&jv);
			JSFunction *fun = JS_ValueToFunction(cx,jv);

			std::vector<jsval> jvs;
			for(lyramilk::data::var::array::iterator it = args.begin();it!=args.end();++it){
				lyramilk::data::var& v = *it;
				jvs.push_back(v2j(cx,v));
			}

			if(JS_TRUE == JS_CallFunction(cx,global,fun,jvs.size(),jvs.data(),&retval)){
				lyramilk::data::var ret;
				j2v(cx,retval,ret);
				return ret;
			}
		}
		return lyramilk::data::var::nil;
	}

	void script_js::reset()
	{
		/**
		//重用
		JS_SetRuntimeThread(rt);
		JSContext *cx = (JSContext *)JS_GetRuntimePrivate(rt);
		if(cx){
			JS_GC(rt);
			JS_SetRuntimePrivate(rt,nullptr);
		}
		/*/
		//非重用
		JS_SetRuntimeThread(rt);
		JSContext *cx = (JSContext *)JS_GetRuntimePrivate(rt);
		if(cx){
			JS_DestroyContext(cx);
			JS_SetRuntimePrivate(rt,nullptr);
			scriptfilename.clear();
		}
		/**/
	}

	void script_js::define(lyramilk::data::string classname,functional_map m,class_builder builder,class_destoryer destoryer)
	{
		JS_SetRuntimeThread(rt);
		JS::RootedObject global(cx_template,JS_GetGlobalObject(cx_template));

		//std::cout << "注册类：" << classname << ",构造:" << (void*)builder << ",释放" << (void*)destoryer << std::endl;
		JSObject* jo = JS_DefineObject(cx_template,global,classname.c_str(),&classesClass,NULL,0);
		assert(jo);
		jsid joid;
		JS_GetObjectId(cx_template,jo,&joid);
		this->m[classname] = joid;

		js_set_property_ptr(cx_template,jo,"__new",(void*)builder);
		js_set_property_ptr(cx_template,jo,"__delete",(void*)destoryer);
		js_set_property_ptr(cx_template,jo,"__env",(void*)this);
		
		functional_map::iterator it = m.begin();
		for(;it!=m.end();++it){
			//std::cout << "\t注册函数：" << it->first << "," << (void*)it->second << std::endl;
			JSFunction *f = JS_DefineFunction(cx_template,jo,it->first.c_str(),js_func_adapter,10,10);
			assert(f);
			JSObject *fo = JS_GetFunctionObject(f);
			assert(fo);
			js_set_property_ptr(cx_template,fo,"__function_pointer",(void*)it->second);
		}
	}
	
	void script_js::define(lyramilk::data::string funcname,functional_type func)
	{
		//std::cout << "注册全局函数：" << funcname << "," << (void*)func << std::endl;
		JS_SetRuntimeThread(rt);
		JS::RootedObject global(cx_template,JS_GetGlobalObject(cx_template));
		JSFunction *f = JS_DefineFunction(cx_template,global,funcname.c_str(),js_func_adapter_noclass,10,10);
		JSObject *fo = JS_GetFunctionObject(f);
		js_set_property_ptr(cx_template,fo,"__function_pointer",(void*)func);
		js_set_property_ptr(cx_template,fo,"__env_pointer",(void*)this);
	}

	lyramilk::data::var script_js::createobject(lyramilk::data::string classname,lyramilk::data::var::array args)
	{
		init();
		JS_SetRuntimeThread(rt);
		JSContext *cx = (JSContext *)JS_GetRuntimePrivate(rt);
		JS::RootedObject global(cx,JS_GetGlobalObject(cx));

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

		js_obj_pack *ppack = new js_obj_pack((engine::class_destoryer)pfuncdel,this,pnewobj);
		jsval jv = PRIVATE_TO_JSVAL(ppack);

		if(JS_TRUE != JS_SetProperty(cx,jsret,"__script_native",&jv)){
			delete ppack;
			return lyramilk::data::var::nil;
		}

		jsid jsretid;
		JS_GetObjectId(cx,jsret,&jsretid);
		lyramilk::data::var v(engine::s_user_objectid(),(const void*)jsretid);
		v.userdata(engine::s_user_nativeobject(),pnewobj);
		return v;
	}

	void script_js::gc()
	{
		JS_SetRuntimeThread(rt);
		JS_GC(rt);
	}

	lyramilk::data::string script_js::name()
	{
		return "js";
	}

	lyramilk::data::string script_js::filename()
	{
		return scriptfilename;
	}

	void script_js::init()
	{
		JS_SetRuntimeThread(rt);
		JSContext *cx = (JSContext *)JS_GetRuntimePrivate(rt);
		if(cx == nullptr){
			cx = JS_NewContext(rt, 8192);
			assert(cx);
			JS_SetOptions(cx, JSOPTION_VAROBJFIX | JSOPTION_METHODJIT);
			JS_SetVersion(cx, JSVERSION_LATEST);
			JS_SetErrorReporter(cx, script_js_error_report);

			JSObject * template_glob = JS_GetGlobalObject(cx_template);

			JSObject * glob = JS_NewGlobalObject(cx, &globalClass, NULL);
			JS_InitStandardClasses(cx,glob);
			JS_SetPrototype(cx,glob,template_glob);

			JS_SetRuntimePrivate(rt,cx);
		}
	}

	static lyramilk::script::engine* __ctr()
	{
		return new script_js();
	}

	static void __dtr(lyramilk::script::engine* eng)
	{
		delete (script_js*)eng;
	}

	static bool ___init()
	{
		lyramilk::script::engine::define("js",__ctr,__dtr);
		return true;
	}

#ifdef __GNUC__

	static __attribute__ ((constructor)) void __init()
	{
		___init();
	}
#else
	bool r = __init();
#endif

}}}
