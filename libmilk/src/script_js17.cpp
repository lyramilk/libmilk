#include "script_js.h"
#include "multilanguage.h"
#include "log.h"
//#include "testing.h"
#include <jsfriendapi.h>
#include <fstream>
#include <cassert>
#include <sys/stat.h>
#include <utime.h>
#include <math.h>
//script_js

namespace lyramilk{namespace script{namespace js
{
	using namespace ::js;
	JSBool static jsctr(JSContext *cx, unsigned argc, jsval *vp);
	void static jsdtr(JSFreeOp *fop, JSObject *obj);
	JSBool static jsinstanceof(JSContext *cx, JSHandleObject obj, const jsval *v, JSBool *bp);
	static void j2v(JSContext* cx,jsval jv,lyramilk::data::var& retv);

	static JSClass globalClass = { "global", JSCLASS_GLOBAL_FLAGS,
			JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
            JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub};

	static JSClass classesClass = { "classes", JSCLASS_HAS_PRIVATE,
			JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
			JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,
			NULL,NULL,NULL,jsinstanceof,jsctr};

	static JSClass nativeClass = { "native", JSCLASS_HAS_PRIVATE,
			JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
			JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,
			jsdtr};

	static JSClass normalClass = { "normal", 0,
			JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
            JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub};

	static void script_js_error_report(JSContext *cx, const char *message, JSErrorReport *report)
	{
		jsval jve;
		if(JS_IsExceptionPending(cx) && JS_GetPendingException(cx,&jve)){
			JSObject *jo = jve.toObjectOrNull();
			if(jo){
				jsval __prop;
				if(JS_GetProperty(cx,jo,"stack",&__prop)){
					lyramilk::data::var retv;
					j2v(cx,__prop,retv);
					lyramilk::klog(lyramilk::log::warning,"lyramilk.script.js") << lyramilk::kdict("%s(%d:%d)%s",(report->filename ? report->filename : "<no name>"),report->lineno,report->column,message) << "\n" << retv << std::endl;
					return;
				}
			}
		}

		lyramilk::klog(lyramilk::log::warning,"lyramilk.script.js") << lyramilk::kdict("%s(%d:%d)%s",(report->filename ? report->filename : "<no name>"),report->lineno,report->column,message) << std::endl;
	}

	struct js_obj_pack
	{
		engine::class_destoryer dtr;
		void* pthis;
		lyramilk::script::js::script_js* eng;
		lyramilk::data::var::map info;

		js_obj_pack(engine::class_destoryer da,lyramilk::script::js::script_js* _env,void* dt):dtr(da),pthis(dt),eng(_env)
		{
			//info[engine::s_env_this()].assign(engine::s_user_nativeobject(),pthis);
			info[engine::s_env_engine()].assign(engine::s_env_engine(),eng);
		}
	};

	static bool js_set_func_native(JSContext *cx,JSObject* o,void* ptr)
	{
		jsval jv;
		jv.setUnmarkedPtr(ptr);
		return !!JS_SetProperty(cx,o,".libmilk",&jv);
	}

	static void* js_get_func_native(JSContext *cx,JSObject* o)
	{
		jsval __prop;
		__prop.setUndefined();
		if(JS_GetProperty(cx,o,".libmilk",&__prop)){
			if(__prop.isNullOrUndefined()) return nullptr;
			return __prop.toUnmarkedPtr();
		}
		return nullptr;
	}

	void inline jsstr2str(const jschar* cstr,size_t len,lyramilk::data::string& str)
	{
		const jschar* streof = &cstr[len+1];
		str.reserve(len*3);

		for(;cstr < streof;){
			jschar jwc = *cstr++;
			unsigned wchar_t wc = jwc;
			if(jwc >= 0xd800 && jwc <= 0xdfff && cstr + 1<streof){
				jschar jwc2 = *cstr++;
				wc = (jwc2&0x03ff) + (((jwc&0x03ff) + 0x40) << 10);
			}
			if(wc < 0x80){
				str.push_back((unsigned char)wc);
			}else if(wc < 0x800){
				str.push_back((unsigned char)((wc>>6)&0x1f) | 0xc0);
				str.push_back((unsigned char)((wc>>0)&0x3f) | 0x80);
			}else if(wc < 0x10000){
				str.push_back((unsigned char)((wc>>12)&0xf) | 0xe0);
				str.push_back((unsigned char)((wc>>6)&0x3f) | 0x80);
				str.push_back((unsigned char)((wc>>0)&0x3f) | 0x80);
			}else if(wc < 0x200000){
				str.push_back((unsigned char)((wc>>18)&0x7) | 0xf0);
				str.push_back((unsigned char)((wc>>12)&0x3f) | 0x80);
				str.push_back((unsigned char)((wc>>6)&0x3f) | 0x80);
				str.push_back((unsigned char)((wc>>0)&0x3f) | 0x80);
			}else if(wc < 0x4000000){
				str.push_back((unsigned char)((wc>>24)&0x3) | 0xf8);
				str.push_back((unsigned char)((wc>>18)&0x3f) | 0x80);
				str.push_back((unsigned char)((wc>>12)&0x3f) | 0x80);
				str.push_back((unsigned char)((wc>>6)&0x3f) | 0x80);
				str.push_back((unsigned char)((wc>>0)&0x3f) | 0x80);
			}else if(wc < 0x80000000){
				str.push_back((unsigned char)((wc>>30)&0x1) | 0xfc);
				str.push_back((unsigned char)((wc>>24)&0x3f) | 0xf0);
				str.push_back((unsigned char)((wc>>18)&0x3f) | 0x80);
				str.push_back((unsigned char)((wc>>12)&0x3f) | 0x80);
				str.push_back((unsigned char)((wc>>6)&0x3f) | 0x80);
				str.push_back((unsigned char)((wc>>0)&0x3f) | 0x80);
			}
		}
	}

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
			lyramilk::data::string str = retv.str();
			std::size_t pos = str.find_last_not_of(".0");
			if(pos != str.npos){
				str.erase(str.begin() + pos + 1,str.end());
				if(str.find('.') == str.npos){
					retv.type(lyramilk::data::var::t_int);
				}
			}else{
				retv.type(lyramilk::data::var::t_int);
			}
			return;
		}else if(jv.isNumber()){
			retv = jv.toNumber();
			return;
		}else if(jv.isBoolean()){
			retv = jv.toBoolean();
			return;
		}else if(jv.isString()){
			JSString* jstr = jv.toString();
			if(jstr == nullptr){
				retv = lyramilk::data::var::nil;
				return;
			}
			size_t len = 0;
			const jschar* cstr = JS_GetStringCharsZAndLength(cx,jstr,&len);
			lyramilk::data::string str;
			jsstr2str(cstr,len,str);
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
			}else if(JS_ObjectIsCallable(cx,jo)){
				jsid jid = OBJECT_TO_JSID(jo);
				retv.assign(engine::s_user_functionid(),(void*)jid);
				return;
			}else if(JS_ObjectIsDate(cx,jo)){
				jsval retval;
				if(JS_TRUE == JS_CallFunctionName(cx,jo,"getTime",0,nullptr,&retval)){
					j2v(cx,retval,retv);
					retv.type(lyramilk::data::var::t_uint);
				}
				return;
			}else if(JS_IsArrayBufferObject(jo,cx)){
				uint8_t * p = JS_GetArrayBufferData(jo,cx);
				uint32_t l = JS_GetArrayBufferByteLength(jo,cx);
				lyramilk::data::chunk cb(p,l);
				retv = cb;
				return;
			}else if(JS_GetClass(jo) == &nativeClass){
				js_obj_pack *ppack = (js_obj_pack *)JS_GetPrivate(jo);
				if(ppack && ppack->pthis){
					jsid joid;
					JS_GetObjectId(cx,jo,&joid);
					retv.assign(engine::s_user_objectid(),(void*)joid);
					retv.userdata(engine::s_user_nativeobject(),ppack->pthis);
					return;
				}
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
							if(jstr == nullptr){
								continue;
							}
							size_t len = 0;
							const jschar* cstr = JS_GetStringCharsZAndLength(cx,jstr,&len);
							jsstr2str(cstr,len,skey);
						}
						j2v(cx,jv,m[skey]);
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

	void static v2j(JSContext* cx,const lyramilk::data::var& v,jsval &ret)
	{
		switch(v.type()){
		  case lyramilk::data::var::t_bool:
			ret.setBoolean(v);
			return;
		  case lyramilk::data::var::t_int:
		  case lyramilk::data::var::t_uint:{
				double dv = v;
				if(floor(v) == dv){
					if(dv > JSVAL_INT_MIN && dv < JSVAL_INT_MAX){
						ret.setNumber(dv);
						return;
					}
				}
			}
		  case lyramilk::data::var::t_double:
			ret.setNumber((double)v);
			return;
		  case lyramilk::data::var::t_bin:{
				const lyramilk::data::chunk& cb = v;
				JSObject* jo = JS_NewArrayBuffer(cx,cb.size());
				uint8_t * p = JS_GetArrayBufferData(jo,cx);
				cb.copy(p,cb.size());
				ret.setObject(*jo);
				return;
			}
		  case lyramilk::data::var::t_wstr:
		  case lyramilk::data::var::t_str:{
				lyramilk::data::string s = v;
				JSString* str = JS_NewStringCopyN(cx,s.c_str(),s.size());
				if(str == nullptr){
					 ret.setUndefined();
					 return;
				}
				ret.setString(str);
				return;
			}break;
		  case lyramilk::data::var::t_array:{
				std::vector<jsval> jvs;
				const lyramilk::data::var::array& ar = v;
				lyramilk::data::var::array::const_iterator it = ar.begin();
				jvs.resize(ar.size());
				for(int i=0;it!=ar.end();++it,++i){
					v2j(cx,*it,jvs[i]);
				}

				JSObject* jo = JS_NewArrayObject(cx,jvs.size(),jvs.data());
				ret.setObject(*jo);
				return;
			}break;
		  case lyramilk::data::var::t_map:{
				JSObject* jo = JS_NewObject(cx,&normalClass,NULL,JS_GetGlobalObject(cx));
				const lyramilk::data::var::map& m = v;
				lyramilk::data::var::map::const_iterator it = m.begin();
				for(;it!=m.end();++it){
					lyramilk::data::string str = it->first;
					lyramilk::data::var v = it->second;
					jsval jv;
					v2j(cx,v,jv);
					JS_SetProperty(cx,jo,str.c_str(),&jv);
				}
				//
				ret.setObject(*jo);
				return ;
			}break;
		  case lyramilk::data::var::t_user:{
				const void* up = v.userdata(engine::s_user_objectid());
				if(up){
					JSObject* jsobj = JSID_TO_OBJECT((jsid)up);
					ret.setObject(*jsobj);
					return;
				}
			}
		  case lyramilk::data::var::t_invalid:
			ret.setUndefined();
			return;
		}
		ret.setUndefined();
	}

	void js_thow(JSContext *cx,const char* s,std::size_t n)
	{
		jsval errorarg;
		{
			JSString* str = JS_NewStringCopyN(cx,s,n);
			if(str == nullptr){
				 return;
			}
			errorarg.setString(str);
		}

		jsval jo;
		JS_GetProperty(cx,JS_GetGlobalObject(cx),"Error",&jo);
		JSObject* errobj = JS_New(cx,&jo.toObject(),1,&errorarg);
		if(errobj){
			jsval exc;
			exc.setObject(*errobj);
			JS_SetPendingException(cx, exc);
		}
	}

	JSBool jsctr(JSContext *cx, unsigned argc, jsval *vp)
	{
		CallArgs args = CallArgsFromVp(argc, vp);
		script_js::class_handler* h = (script_js::class_handler*)JS_GetPrivate(&args.callee());
		void *pnewobj = NULL;
		if(h && h->ctr){
			lyramilk::data::var::array params;
			if(argc>0){
				jsval* jv = JS_ARGV(cx,vp);
				params.resize(argc);
				for(unsigned i=0;i<argc;++i){
					j2v(cx,jv[i],params[i]);
				}
			}

			try{
				pnewobj = h->ctr(params);
				if(pnewobj == nullptr){
					lyramilk::data::string str = D("创建对象失败");
					js_thow(cx,str.c_str(),str.size());
					return JS_FALSE;
				}
			}catch(lyramilk::data::string& str){
				js_thow(cx,str.c_str(),str.size());
				return JS_FALSE;
			}catch(const char* cstr){
				js_thow(cx,cstr,strlen(cstr));
				return JS_FALSE;
			}catch(std::exception& e){
				const char* cstr = e.what();
				js_thow(cx,cstr,strlen(cstr));
				return JS_FALSE;
			}catch(...){
				lyramilk::data::string str = D("未知异常");
				js_thow(cx,str.c_str(),str.size());
				return JS_FALSE;
			}
		}

		JSObject* jsret = JS_NewObject(cx,&nativeClass,&args.callee(),JS_GetGlobalObject(cx));

		js_obj_pack *ppack = new js_obj_pack(h->dtr,h->eng,pnewobj);

		JS_SetPrivate(jsret,ppack);
		args.rval().setObject(*jsret);
		return JS_TRUE;
	}

	void jsdtr(JSFreeOp *fop, JSObject *obj)
	{
		//JSRuntime* rt = fop->runtime();
		js_obj_pack *ppack = (js_obj_pack*)JS_GetPrivate(obj);
		if(ppack && ppack->pthis && ppack->dtr){
			ppack->dtr(ppack->pthis);
			delete ppack;
		}
	}

	JSBool static js_func_adapter(JSContext *cx, unsigned argc, Value *vp)
	{
		CallArgs args = CallArgsFromVp(argc, vp);
		engine::functional_type pfun = (engine::functional_type)js_get_func_native(cx,&args.callee());
		js_obj_pack *ppack = (js_obj_pack*)JS_GetPrivate(&args.thisv().toObject());

		if(pfun && ppack && ppack->pthis){
			lyramilk::data::var::array params;
			params.resize(argc);
			for(unsigned i=0;i<argc;++i){
				j2v(cx,args[i],params[i]);
			}
			try{
				lyramilk::data::var ret = pfun(params,ppack->info,ppack->eng);
				jsval jsret;
				v2j(cx,ret,jsret);
				args.rval().set(jsret);
			}catch(lyramilk::data::string& str){
				js_thow(cx,str.c_str(),str.size());
				return JS_FALSE;
			}catch(const char* cstr){
				js_thow(cx,cstr,strlen(cstr));
				return JS_FALSE;
			}catch(std::exception& e){
				const char* cstr = e.what();
				js_thow(cx,cstr,strlen(cstr));
				return JS_FALSE;
			}catch(...){
				lyramilk::data::string str = D("未知异常");
				js_thow(cx,str.c_str(),str.size());
				return JS_FALSE;
			}
		}
		return JS_TRUE;
	}

	JSBool script_js::js_func_adapter_noclass(JSContext *cx, unsigned argc, jsval *vp)
	{
		CallArgs args = CallArgsFromVp(argc, vp);
		func_handler* h = (func_handler*)js_get_func_native(cx,&args.callee());

		lyramilk::data::var::array params;
		params.resize(argc);
		for(unsigned i=0;i<argc;++i){
			j2v(cx,args[i],params[i]);
		}
		try{
			lyramilk::data::var ret = h->func(params,h->eng->info,nullptr);
			jsval jsret;
			v2j(cx,ret,jsret);
			args.rval().set(jsret);
		}catch(lyramilk::data::string& str){
			js_thow(cx,str.c_str(),str.size());
			return JS_FALSE;
		}catch(const char* cstr){
			js_thow(cx,cstr,strlen(cstr));
			return JS_FALSE;
		}catch(std::exception& e){
			const char* cstr = e.what();
			js_thow(cx,cstr,strlen(cstr));
			return JS_FALSE;
		}catch(...){
			lyramilk::data::string str = D("未知异常");
			js_thow(cx,str.c_str(),str.size());
			return JS_FALSE;
		}
		return JS_TRUE;	
	}

	JSBool jsinstanceof(JSContext *cx, JSHandleObject obj, const jsval *v, JSBool *bp)
	{
		//支持 instanceof操作符
		JSObject *joproto = JS_GetPrototype(&v->toObject()); 
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
	script_js::script_js():rt(NULL),cx_template(NULL)
	{
		info[engine::s_env_engine()].assign(engine::s_env_engine(),this);

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
		JS_SetRuntimeThread(rt);
		init();
		JSContext* selectedcx = (JSContext *)JS_GetRuntimePrivate(rt);
		JSObject* global = JS_GetGlobalObject(selectedcx);

		JSScript* script = JS_CompileScript(selectedcx,global,scriptstring.c_str(),scriptstring.size(),NULL,1);
		if(script){
			return !!JS_ExecuteScript(selectedcx,global,script,NULL);
		}
		return false;
	}

		struct bytecode
		{
			time_t tm;
			std::string code;
		};

	bool script_js::load_file(lyramilk::data::string scriptfile)
	{
		if(scriptfilename.empty()){
			scriptfilename = scriptfile;
		}

		JS_SetRuntimeThread(rt);
		init();
		JSContext* selectedcx = (JSContext *)JS_GetRuntimePrivate(rt);
		JSObject* global = JS_GetGlobalObject(selectedcx);

		/*
		JSScript* script = nullptr;
		script = JS_CompileUTF8File(selectedcx,global,scriptfile.c_str());
		if(!script)return false;
		return !!JS_ExecuteScript(selectedcx,global,script,nullptr);
		*/
		static std::map<lyramilk::data::string,bytecode> bytecodemap;
		static lyramilk::threading::mutex_rw bytecodelock;
		JSScript* script = nullptr;

		struct stat st = {0};
		if(0 !=::stat(scriptfile.c_str(),&st)){
			return false;
		}
		{
			//尝试读取
			lyramilk::threading::mutex_sync _(bytecodelock.r());
			std::map<lyramilk::data::string,bytecode>::const_iterator it = bytecodemap.find(scriptfile);
			if(it!=bytecodemap.end()){
				const bytecode& c = it->second;
				if(st.st_mtime == c.tm){
					script = JS_DecodeScript(selectedcx,(const void*)c.code.c_str(),c.code.size(),NULL,NULL);
				}
			}
		}
		if(script){
			return !!JS_ExecuteScript(selectedcx,global,script,nullptr);
		}else{
			JS::CompileOptions options(selectedcx);
			//options.setSourcePolicy(JS::CompileOptions::NO_SOURCE);
			JS::RootedObject g(selectedcx,global);
			script = JS::Compile(selectedcx,g,options,scriptfile.c_str());
			if(!script)return false;
			if(JS_ExecuteScript(selectedcx,global,script,nullptr)){
				uint32_t len = 0;
				void* p = JS_EncodeScript(selectedcx,script,&len);
				if(p && len){
					bytecode c;
					c.tm = st.st_mtime;
					c.code.assign((const char*)p,len);
					{
						lyramilk::threading::mutex_sync _(bytecodelock.w());
						bytecodemap[scriptfile] = c;
					}
				}
				return true;
			}
			return false;
		}
	}

	lyramilk::data::var script_js::call(lyramilk::data::var func,lyramilk::data::var::array args)
	{
		JS_SetRuntimeThread(rt);
		init();
		JSContext* selectedcx = (JSContext *)JS_GetRuntimePrivate(rt);
		JSObject* global = JS_GetGlobalObject(selectedcx);

		if(func.type_like(lyramilk::data::var::t_str)){
			jsval retval;
			std::vector<jsval> jvs;
			jvs.resize(args.size());
			for(std::size_t i=0;i < args.size();++i){
				v2j(selectedcx,args[i],jvs[i]);
			}

			if(JS_TRUE == JS_CallFunctionName(selectedcx,global,func.str().c_str(),jvs.size(),jvs.data(),&retval)){
				lyramilk::data::var ret;
				j2v(selectedcx,retval,ret);
				return ret;
			}
		}else if(func.type() == lyramilk::data::var::t_user){
			//调用回调函数
			jsid jid = (jsid)func.userdata(engine::s_user_functionid());
			if(!jid) return lyramilk::data::var::nil;
			jsval retval;
			jsval jv;
			JS_IdToValue(selectedcx,jid,&jv);
			JSFunction *fun = JS_ValueToFunction(selectedcx,jv);

			std::vector<jsval> jvs;
			jvs.resize(args.size());
			for(std::size_t i=0;i < args.size();++i){
				v2j(selectedcx,args[i],jvs[i]);
			}

			if(JS_TRUE == JS_CallFunction(selectedcx,global,fun,jvs.size(),jvs.data(),&retval)){
				lyramilk::data::var ret;
				j2v(selectedcx,retval,ret);
				return ret;
			}
		}
		return lyramilk::data::var::nil;
	}

	void script_js::reset()
	{
		JS_SetRuntimeThread(rt);
		JSContext *selectedcx = (JSContext *)JS_GetRuntimePrivate(rt);
		if(selectedcx){
			JS_DestroyContext(selectedcx);
			JS_SetRuntimePrivate(rt,nullptr);
			scriptfilename.clear();
		}
	}

	void script_js::define(lyramilk::data::string classname,functional_map m,class_builder builder,class_destoryer destoryer)
	{
		JS_SetRuntimeThread(rt);
		JSContext* selectedcx = cx_template;
		JSObject* global = JS_GetGlobalObject(selectedcx);

		//std::cout << "注册类：" << classname << ",构造:" << (void*)builder << ",释放" << (void*)destoryer << std::endl;
		JSObject* jo = JS_DefineObject(selectedcx,global,classname.c_str(),&classesClass,NULL,0);
		assert(jo);
		jsid joid;
		JS_GetObjectId(selectedcx,jo,&joid);

		class_handler h;
		h.ctr = builder;
		h.dtr = destoryer;
		h.eng = this;
		scache.push_back(h);
		JS_SetPrivate(jo,&scache.back());
		
		functional_map::iterator it = m.begin();
		for(;it!=m.end();++it){
			//std::cout << "\t注册函数：" << it->first << "," << (void*)it->second << std::endl;
			JSFunction *f = JS_DefineFunction(selectedcx,jo,it->first.c_str(),js_func_adapter,0,0);
			assert(f);
			JSObject *fo = JS_GetFunctionObject(f);
			assert(fo);
			js_set_func_native(selectedcx,fo,(void*)it->second);
		}
	}
	
	void script_js::define(lyramilk::data::string funcname,functional_type func)
	{
		JS_SetRuntimeThread(rt);
		JSContext* selectedcx = cx_template;
		JSObject* global = JS_GetGlobalObject(selectedcx);
		//std::cout << "注册全局函数：" << funcname << "," << (void*)func << std::endl;
		JSFunction *f = JS_DefineFunction(selectedcx,global,funcname.c_str(),js_func_adapter_noclass,0,0);
		JSObject *fo = JS_GetFunctionObject(f);

		func_handler h;
		h.func = func;
		h.eng = this;
		fcache.push_back(h);
		js_set_func_native(selectedcx,fo,&fcache.back());
	}

	lyramilk::data::var script_js::createobject(lyramilk::data::string classname,lyramilk::data::var::array args)
	{
		JS_SetRuntimeThread(rt);
		init();
		JSContext* selectedcx = (JSContext *)JS_GetRuntimePrivate(rt);
		JSObject* global = JS_GetGlobalObject(selectedcx);

		jsval jo;
		JS_GetProperty(selectedcx,global,classname.c_str(),&jo);

		std::vector<jsval> jvs;
		jvs.resize(args.size());
		for(std::size_t i=0;i < args.size();++i){
			v2j(selectedcx,args[i],jvs[i]);
		}

		JSObject* jsobj = JS_New(selectedcx,&jo.toObject(),jvs.size(),jvs.data());

		jsval jv;
		jv.setObject(*jsobj);

		lyramilk::data::var v;
		j2v(selectedcx,jv,v);
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
			JSObject * glob = JS_NewGlobalObject(cx, &globalClass, NULL);
			JS_InitStandardClasses(cx,glob);

			JS_SetPrototype(cx,glob,JS_GetGlobalObject(cx_template));

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
