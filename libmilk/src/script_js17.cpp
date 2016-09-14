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
namespace lyramilk{namespace script{namespace js
{
	JSBool jsctr(JSContext *cx, unsigned argc, jsval *vp);
	void jsdtr(JSFreeOp *fop, JSObject *obj);
	void jsdtrvar(JSFreeOp *fop, JSObject *obj);
	JSBool jsinstanceof(JSContext *cx, JSHandleObject obj, const jsval *v, JSBool *bp);

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

	lyramilk::data::var j2v(JSContext* cx,jsval jv)
	{
		lyramilk::data::var v;
		if(jv.isNullOrUndefined()){
		}else if(jv.isInt32()){
			v.assign(jv.toInt32());
		}else if(jv.isDouble()){
			v.assign(jv.toDouble());
		}else if(jv.isNumber()){
			v.assign(jv.toNumber());
		}else if(jv.isBoolean()){
			v.assign(jv.toBoolean());
		}else if(jv.isString()){
			JSString* jstr = jv.toString();
			size_t len = 0;
			const jschar* cstr = JS_GetStringCharsZAndLength(cx,jstr,&len);

			lyramilk::data::wstring str;
			for(size_t i =0;i<len;++i){
				wchar_t c = cstr[i];
				str.push_back(c);
			}
			v.assign(str);
		}else if(jv.isObject()){
			JSObject *jo = jv.toObjectOrNull();
			if(JS_IsArrayObject(cx,jo)){
				lyramilk::data::var::array r;
				uint32_t len = 0;
				if(!JS_GetArrayLength(cx,jo,&len)){
					return lyramilk::data::var::nil;
				}
				for(uint32_t i=0;i<len;++i){
					jsval jv;
					JS_LookupElement(cx,jo,i,&jv);
					lyramilk::data::var cval = j2v(cx,jv);
					r.push_back(cval);
				}
				v = r;
			}else{
				js_obj_pack *ppack = (js_obj_pack*)js_get_property_ptr(cx,jo,"__script_native");
				if(ppack && ppack->pthis){
					jsid joid;
					JS_GetObjectId(cx,jo,&joid);
					v.assign(engine::s_user_objectid(),(void*)joid);
					v.userdata(engine::s_user_nativeobject(),ppack->pthis);
					return v;
				}
				jsval jv;
				if(JS_TRUE == JS_GetProperty(cx,jo,"__script_var",&jv)){
					if(!jv.isNullOrUndefined()){
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

						lyramilk::data::var ckey = j2v(cx,jk);
						lyramilk::data::var cval = j2v(cx,jv);
						m.insert(std::pair<lyramilk::data::var,lyramilk::data::var>(ckey,cval));
					}
				}
				v = m;
			}

			if(JS_ObjectIsFunction(cx,jo)){
				jsid jid = OBJECT_TO_JSID(jo);
				v.assign(engine::s_user_functionid(),(void*)jid);
			}
		}else{
			JSType jt = JS_TypeOfValue(cx,jv);
			const char* tn = JS_GetTypeName(cx,jt);
			COUT << tn << std::endl;
			TODO();
		}
		return v;
	}

	jsval v2j(JSContext* cx,lyramilk::data::var v)
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
					jvs.push_back(v2j(cx,*it));
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
					lyramilk::data::var v = j2v(cx,jv[i]);
					params.push_back(v);
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

	JSBool js_func_adapter(JSContext *cx, unsigned argc, jsval *vp)
	{
		JS::RootedObject callee(cx, JSVAL_TO_OBJECT(JS_CALLEE(cx, vp)));
		JS::RootedObject jsobj(cx,JS_THIS_OBJECT(cx,vp));

		void* pfunc = js_get_property_ptr(cx,callee,"__function_pointer");
		js_obj_pack *ppack = (js_obj_pack*)js_get_property_ptr(cx,jsobj,"__script_native");
		jsval* jv = JS_ARGV(cx,vp);

		engine::functional_type pfun = (engine::functional_type)pfunc;
		if(pfun && ppack && ppack->pthis){
			lyramilk::data::var::array params;
			for(unsigned i=0;i<argc;++i){
				lyramilk::data::var v = j2v(cx,jv[i]);
				params.push_back(v);
			}
			try{
				lyramilk::data::var ret = pfun(params,ppack->info);
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

	JSBool js_func_adapter_noclass(JSContext *cx, unsigned argc, jsval *vp)
	{
		JS::RootedObject callee(cx, JSVAL_TO_OBJECT(JS_CALLEE(cx, vp)));
		//JSObject *jsobj = JS_THIS_OBJECT(cx,vp);

		void* pfunc = js_get_property_ptr(cx,callee,"__function_pointer");
		//js_obj_pack *ppack = (js_obj_pack*)JS_GetPrivate(cx,jsobj);
		script_js *penv = (script_js*)js_get_property_ptr(cx,callee,"__env_pointer");
		jsval* jv = JS_ARGV(cx,vp);

		engine::functional_type pfun = (engine::functional_type)pfunc;
		if(pfun){
			lyramilk::data::var::array params;
			for(unsigned i=0;i<argc;++i){
				lyramilk::data::var v = j2v(cx,jv[i]);
				params.push_back(v);
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

	script_js::script_js():rt(NULL),cx(NULL)
	{
		rt = JS_Init(128*1024L*1024L);
		cx = JS_NewContext(rt, 8192);

		JS_SetRuntimePrivate(rt,cx);

		JS_SetCStringsAreUTF8();
		//JS_SetOptions(cx, JSOPTION_VAROBJFIX | JSOPTION_JIT | JSOPTION_METHODJIT);
		JS_SetOptions(cx, JSOPTION_VAROBJFIX | JSOPTION_METHODJIT);
		JS_SetVersion(cx, JSVERSION_LATEST);

		JS_SetErrorReporter(cx, script_js_error_report);

		isinited = false;
		info[engine::s_env_engine()].assign(engine::s_user_engineptr(),this);
	}

	script_js::~script_js()
	{
		gc();
		JS_SetRuntimeThread(rt);
		JS_DestroyContext(cx);
		JS_DestroyRuntime(rt);
		//JS_ShutDown();
	}

	bool script_js::load_string(lyramilk::data::string scriptstring)
	{
		init();
		JS_SetRuntimeThread(rt);
		JS::RootedObject global(cx,JS_GetGlobalObject(cx));
		JSScript* script = JS_CompileScript(cx,global,scriptstring.c_str(),scriptstring.size(),NULL,1);
		if(script){
			bool ret = JS_ExecuteScript(cx,global,script,NULL) == JS_TRUE;
			gc();
			return ret;
		}
		return false;
	}

	//lyramilk::debug::nsecdiff td;
	/*
	lyramilk::debug::clocktester _d(td,log(lyramilk::log::debug),k);
	*/

	void fsync(lyramilk::data::string file1,lyramilk::data::string file2,bool* file1exist,bool* file2exist,bool* issync)
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


	bool script_js::load_file(lyramilk::data::string scriptfile)
	{
		init();
		JS_SetRuntimeThread(rt);
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
			gc();
			return ret;
		}
		return false;
	}

	lyramilk::data::var script_js::call(lyramilk::data::var func,lyramilk::data::var::array args)
	{
		JS_SetRuntimeThread(rt);
		JS::RootedObject global(cx,JS_GetGlobalObject(cx));

		if(func.type_compat(lyramilk::data::var::t_str)){
			jsval retval;
			std::vector<jsval> jvs;
			for(lyramilk::data::var::array::iterator it = args.begin();it!=args.end();++it){
				lyramilk::data::var& v = *it;
				jvs.push_back(v2j(cx,v));
			}
			if(JS_TRUE == JS_CallFunctionName(cx,global,func.str().c_str(),jvs.size(),jvs.data(),&retval)){
				lyramilk::data::var ret = j2v(cx,retval);
				gc();
				return ret;
			}
		}else if(func.type() == lyramilk::data::var::t_user){
			jsid jid = (jsid)func.userdata(engine::s_user_functionid());
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
				lyramilk::data::var ret = j2v(cx,retval);
				gc();
				return ret;
			}
		}
		gc();
		return lyramilk::data::var::nil;
	}

	void script_js::reset()
	{
		TODO();
	}

	void script_js::define(lyramilk::data::string classname,functional_map m,class_builder builder,class_destoryer destoryer)
	{
		init();
		JS_SetRuntimeThread(rt);
		JS::RootedObject global(cx,JS_GetGlobalObject(cx));

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
	
	void script_js::define(lyramilk::data::string funcname,functional_type func)
	{
		init();
		JS_SetRuntimeThread(rt);
		JS::RootedObject global(cx,JS_GetGlobalObject(cx));

		JSFunction *f = JS_DefineFunction(cx,global,funcname.c_str(),js_func_adapter_noclass,10,10);
		JSObject *fo = JS_GetFunctionObject(f);
		js_set_property_ptr(cx,fo,"__function_pointer",(void*)func);
		js_set_property_ptr(cx,fo,"__env_pointer",(void*)this);
	}

	lyramilk::data::var script_js::createobject(lyramilk::data::string classname,lyramilk::data::var::array args)
	{
		init();
		JS_SetRuntimeThread(rt);
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
		//JS_SetPrivate(jsret,ppack);
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
		if(isinited) return;
		isinited = true;
		JS_SetRuntimeThread(rt);
		JSObject * glob = JS_NewGlobalObject(cx, &globalClass, NULL);
		JS::RootedObject global(cx, glob);
		JS_InitStandardClasses(cx,global);
	}
}}}
