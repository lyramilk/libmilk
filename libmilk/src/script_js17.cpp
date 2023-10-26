#include "script_js.h"
#include "script_js17.h"
#include "datawrapper.h"
#include "dict.h"
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
	static JSBool jsctr(JSContext *cx, unsigned argc, Value *vp);
	static void jsdtr(js::FreeOp *fop, JSObject *obj);
	static JSBool jsinstanceof(JSContext *cx, JSHandleObject obj, const Value *v, JSBool *bp);
	static void j2v(JSContext* cx,const Value& jv,lyramilk::data::var* retv);
	static bool j2s(JSContext* cx,const Value&,lyramilk::data::string* retv);
	static bool v2j(JSContext* cx,const lyramilk::data::var& v,Value *ret);

	static JSBool jget(JSContext *cx, JSHandleObject obj, JSHandleId id, JSMutableHandleValue vp);
	static JSBool jset(JSContext *cx, JSHandleObject obj, JSHandleId id, JSBool strict, JSMutableHandleValue vp);

/*
	static JSBool jsnewenum(JSContext *cx, JSHandleObject obj, JSIterateOp enum_op,Value *statep, jsid *idp);
*/

	static js::Class globalClass = { "global", JSCLASS_GLOBAL_FLAGS,
			JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
            JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub};

	static js::Class classesClass = { "classes", JSCLASS_HAS_PRIVATE,
			JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
			JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,
			nullptr,nullptr,nullptr,jsinstanceof,jsctr};

	static js::Class nativeClass = { "native", JSCLASS_HAS_PRIVATE /* | JSCLASS_NEW_ENUMERATE*/,
			JS_PropertyStub, JS_PropertyStub, jget, jset,
			JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,
			jsdtr};

	static js::Class normalClass = { "normal", 0,
			JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
            JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub};

/*
	static JSClass enumClass = { "enum", JSCLASS_NEW_ENUMERATE,
			JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
            JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub};*/

	static void script_js_error_report(JSContext *cx, const char *message, JSErrorReport *report)
	{
		Value jve;
		if(JS_IsExceptionPending(cx) && JS_GetPendingException(cx,&jve)){
			JSObject *jo = jve.toObjectOrNull();
			if(jo){
				Value __prop;
				if(JS_GetProperty(cx,jo,"stack",&__prop)){
					lyramilk::data::var retv;
					j2v(cx,__prop,&retv);
					lyramilk::klog(lyramilk::log::warning,"lyramilk.script.js") << lyramilk::kdict("%s(%d:%d)%s",(report->filename ? report->filename : "<no name>"),report->lineno,report->column,message) << "\n" << retv << std::endl;
					return;
				}
			}
		}

		lyramilk::klog(lyramilk::log::warning,"lyramilk.script.js") << lyramilk::kdict("%s(%d:%d)%s",(report->filename ? report->filename : "<no name>"),report->lineno,report->column,message) << std::endl;
	}

	struct js_obj_pack
	{
		unsigned char magic;
		engine::class_destoryer dtr;
		sclass* pthis;
		lyramilk::script::js::script_js* eng;
		lyramilk::data::map info;
		int idx;

		js_obj_pack(engine::class_destoryer da,lyramilk::script::js::script_js* _env,sclass* dt):dtr(da),pthis(dt),eng(_env)
		{
			magic = 0x22;
			//info[engine::s_env_this()].assign(engine::s_user_nativeobject(),pthis);
			info[engine::s_env_engine()].assign(engine_datawrapper(eng));
		}
	};

	static bool js_set_func_native(JSContext *cx,JSObject* o,void* ptr)
	{
		Value jv;
		jv.setUnmarkedPtr(ptr);
		return !!JS_SetProperty(cx,o,".libmilk",&jv);
	}

	static void* js_get_func_native(JSContext *cx,JSObject* o)
	{
		Value __prop;
		__prop.setUndefined();
		if(JS_GetProperty(cx,o,".libmilk",&__prop)){
			if(__prop.isNullOrUndefined()) return nullptr;
			return __prop.toUnmarkedPtr();
		}
		return nullptr;
	}

	void inline jsstr2str(const jschar* cstr,size_t len,lyramilk::data::string* strg)
	{
		lyramilk::data::string& str = *strg;
		const jschar* streof = &cstr[len];
		str.reserve(len*3);
		for(;cstr < streof;){
			jschar jwc = *cstr++;
			wchar_t wc = jwc;
			if(jwc >= 0xd800 && jwc <= 0xdfff && cstr<streof){
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
			}else if(wc < 0x80000000l){
				str.push_back((unsigned char)((wc>>30)&0x1) | 0xfc);
				str.push_back((unsigned char)((wc>>24)&0x3f) | 0xf0);
				str.push_back((unsigned char)((wc>>18)&0x3f) | 0x80);
				str.push_back((unsigned char)((wc>>12)&0x3f) | 0x80);
				str.push_back((unsigned char)((wc>>6)&0x3f) | 0x80);
				str.push_back((unsigned char)((wc>>0)&0x3f) | 0x80);
			}
		}
	}

	class jsvar_datawrapper:public lyramilk::data::datawrapper
	{
		const ::js::Value& jv;
		JSContext* cx;
	  public:	
		jsvar_datawrapper(JSContext* cx,const ::js::Value& v):jv(v)
		{
			this->cx = cx;
		}

	  	virtual ~jsvar_datawrapper()
		{
		}

		virtual lyramilk::data::string name() const
		{
			return "a445";
		}

		virtual lyramilk::data::datawrapper* clone() const
		{
			return new jsvar_datawrapper(cx,jv);
		}

		virtual void destory()
		{
			delete this;
		}


		virtual bool type_like(lyramilk::data::var::vt nt) const
		{

			if(nt == lyramilk::data::var::t_str){
				if(jv.isString()) return true;
			}else if(nt == lyramilk::data::var::t_bool){
				if(jv.isBoolean()) return true;
			}else if(nt == lyramilk::data::var::t_int){
				if(jv.isNumber()) return true;
			}else if(nt == lyramilk::data::var::t_map){
				if(jv.isObject()){
					JSObject *jo = jv.toObjectOrNull();
					if(JS_IsArrayObject(cx,jo)){
						return false;
					}else if(JS_ObjectIsCallable(cx,jo)){
						return false;
					}else if(JS_ObjectIsDate(cx,jo)){
						return false;
					}else if(JS_IsArrayBufferObject(jo,cx)){
						return false;
					}else if(JS_GetClass(jo) == Jsvalify(&nativeClass)){
						return false;
					}else{
						return true;
					}
				}
			}else if(nt == lyramilk::data::var::t_array){
				if(jv.isObject()){
					JSObject *jo = jv.toObjectOrNull();
					if(JS_IsArrayObject(cx,jo)){
						return true;
					}
				}
			}

COUT << lyramilk::data::var::type_name(nt) << std::endl;
			TODO();
		}

		virtual lyramilk::data::string get_str()
		{
			JSString* jstr = jv.toString();
			if(jstr == nullptr){
				return "";
			}
			size_t len = 0;
			const jschar* cstr = JS_GetStringCharsZAndLength(cx,jstr,&len);
			lyramilk::data::string tstr;
			jsstr2str(cstr,len,&tstr);
			return tstr;
		}

		virtual lyramilk::data::datawrapper& at(lyramilk::data::uint64 index)
		{
			TODO();
		}

		virtual lyramilk::data::datawrapper& at(const lyramilk::data::string& index)
		{
			TODO();
		}

		virtual lyramilk::data::chunk get_bytes()
		{
			JSString* jstr = jv.toString();
			if(jstr == nullptr){
				return lyramilk::data::chunk();
			}
			size_t len = 0;
			const jschar* cstr = JS_GetStringCharsZAndLength(cx,jstr,&len);
			lyramilk::data::string tstr;
			jsstr2str(cstr,len,&tstr);
			return lyramilk::data::chunk((const unsigned char*)tstr.c_str(),tstr.size());
		}

		virtual lyramilk::data::wstring get_wstr()
		{
			JSString* jstr = jv.toString();
			if(jstr == nullptr){
				return lyramilk::data::wstring();
			}
			size_t len = 0;
			const jschar* cstr = JS_GetStringCharsZAndLength(cx,jstr,&len);
			lyramilk::data::string tstr;
			jsstr2str(cstr,len,&tstr);
			return lyramilk::data::var(tstr);
		}

		virtual bool get_bool()
		{
			return jv.toBoolean();
		}

		virtual lyramilk::data::int64 get_int()
		{
			return jv.toInt32();
		}

		virtual double get_double()
		{
			TODO();
		}

		virtual lyramilk::data::datawrapper& at(const lyramilk::data::wstring& index)
		{
			TODO();
		}

	};




	void j2v(JSContext* cx,const Value& jv,lyramilk::data::var* retv)
	{
#if 1

		if(jv.isNullOrUndefined()){
			*retv = lyramilk::data::var::nil;
			return;
		}else if(jv.isInt32()){
			*retv = jv.toInt32();
			return;
		}else if(jv.isDouble()){
			*retv = jv.toDouble();
			lyramilk::data::string str = retv->str();
			std::size_t pos = str.find_last_not_of(".0");
			if(pos != str.npos){
				str.erase(str.begin() + pos + 1,str.end());
				if(str.find('.') == str.npos){
					retv->type(lyramilk::data::var::t_int);
				}
			}else{
				retv->type(lyramilk::data::var::t_int);
			}
			return;
		}else if(jv.isNumber()){
			*retv = jv.toNumber();
			return;
		}else if(jv.isBoolean()){
			*retv = jv.toBoolean();
			return;
		}else if(jv.isString()){
			JSString* jstr = jv.toString();
			if(jstr == nullptr){
				*retv = lyramilk::data::var::nil;
				return;
			}
			size_t len = 0;
			const jschar* cstr = JS_GetStringCharsZAndLength(cx,jstr,&len);
			lyramilk::data::string tstr;
			jsstr2str(cstr,len,&tstr);
			retv->clear();
			retv->assign(tstr);
			return;
		}else if(jv.isObject()){
			JSObject *jo = jv.toObjectOrNull();
			if(JS_IsArrayObject(cx,jo)){
				retv->type(lyramilk::data::var::t_array);
				lyramilk::data::array& r = *retv;
				uint32_t len = 0;
				if(!JS_GetArrayLength(cx,jo,&len)){
					*retv = lyramilk::data::var::nil;
					return;
				}
				r.resize(len);
				for(uint32_t i=0;i<len;++i){
					Value jv;
					JS_LookupElement(cx,jo,i,&jv);
					j2v(cx,jv,&r[i]);
				}
			}else if(JS_ObjectIsCallable(cx,jo)){
				jsid jid = OBJECT_TO_JSID(jo);
				retv->assign(mozjsobject_datawrapper(nullptr,jid,nullptr));
				return;
			}else if(JS_ObjectIsDate(cx,jo)){
				Value retval;
				if(JS_TRUE == JS_CallFunctionName(cx,jo,"getTime",0,nullptr,&retval)){
					j2v(cx,retval,retv);
					retv->type(lyramilk::data::var::t_uint);
				}
				return;
			}else if(JS_IsArrayBufferObject(jo,cx)){
				uint8_t * p = JS_GetArrayBufferData(jo,cx);
				uint32_t l = JS_GetArrayBufferByteLength(jo,cx);
				lyramilk::data::chunk cb(p,l);
				retv->assign(cb);
				return;
			}else if(JS_GetClass(jo) == Jsvalify(&nativeClass)){
				js_obj_pack *ppack = (js_obj_pack *)JS_GetPrivate(jo);
				if(ppack && ppack->pthis){
					jsid joid;
					JS_GetObjectId(cx,jo,&joid);
					retv->assign(mozjsobject_datawrapper(ppack->eng,joid,ppack->pthis));
					return;
				}
			}else{
				JSObject *iter = JS_NewPropertyIterator(cx,jo);
				if(iter){
					retv->type(lyramilk::data::var::t_map);
					lyramilk::data::map& m = *retv;
					jsid jid;
					while(JS_NextProperty(cx,iter,&jid)){
						Value jk,jv;
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
							jsstr2str(cstr,len,&skey);
						}
						if(skey == "__script_var"){
							void* p = jv.toUnmarkedPtr();
							*retv = *(lyramilk::data::var*)p;
							return;
						}
						j2v(cx,jv,&m[skey]);
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
#else

		retv->assign(jsvar_datawrapper(cx,jv));
#endif
	}
	bool j2s(JSContext* cx,const Value& jv,lyramilk::data::string* retv)
	{
		if(jv.isString()){
			JSString* jstr = jv.toString();
			if(jstr == nullptr){
				*retv = "";
				return false;
			}
			size_t len = 0;
			const jschar* cstr = JS_GetStringCharsZAndLength(cx,jstr,&len);
			jsstr2str(cstr,len,retv);
			return true;
		}
		retv->clear();
		return false;
	}
	bool v2j(JSContext* cx,const lyramilk::data::var& v,Value *ret)
	{
		switch(v.type()){
		  case lyramilk::data::var::t_bool:
			ret->setBoolean(v);
			return true;
		  case lyramilk::data::var::t_int:
		  case lyramilk::data::var::t_uint:{
				double dv = v;
				if(floor(dv) == dv){
					if(dv > JSVAL_INT_MIN && dv < JSVAL_INT_MAX){
						ret->setNumber(dv);
						return true;
					}
				}
			}
		  case lyramilk::data::var::t_double:
			ret->setNumber((double)v);
			return true;
		  case lyramilk::data::var::t_bin:{
				const lyramilk::data::chunk& cb = v;
				JSObject* jo = JS_NewArrayBuffer(cx,cb.size());
				uint8_t * p = JS_GetArrayBufferData(jo,cx);
				cb.copy(p,cb.size());
				ret->setObject(*jo);
				return true;
			}
		  case lyramilk::data::var::t_wstr:
		  case lyramilk::data::var::t_str:{
				if(JS_IsExceptionPending(cx)) return false;
				lyramilk::data::string s = v;
				JSString* str = JS_NewStringCopyN(cx,s.c_str(),s.size());
				if(JS_IsExceptionPending(cx)){
					JS_ClearPendingException(cx);

					JSObject* jo = JS_NewArrayBuffer(cx,s.size());
					uint8_t * p = JS_GetArrayBufferData(jo,cx);
					s.copy((char*)p,s.size());
					ret->setObject(*jo);
					return true;
				}

				if(str == nullptr){
					 ret->setUndefined();
					 return true;
				}
				ret->setString(str);
				return true;
			}break;
		  case lyramilk::data::var::t_array:{
				std::vector<Value> jvs;
				const lyramilk::data::array& ar = v;
				lyramilk::data::array::const_iterator it = ar.begin();
				jvs.resize(ar.size());
				for(int i=0;it!=ar.end();++it,++i){
					v2j(cx,*it,&jvs[i]);
				}

				JSObject* jo = JS_NewArrayObject(cx,jvs.size(),jvs.data());
				ret->setObject(*jo);
				return true;
			}break;
		  case lyramilk::data::var::t_map:{
				JSObject* jo = JS_NewObject(cx,Jsvalify(&normalClass),nullptr,JS_GetGlobalObject(cx));
				const lyramilk::data::map& m = v;
				lyramilk::data::map::const_iterator it = m.begin();
				for(;it!=m.end();++it){
					lyramilk::data::string str = it->first;
					lyramilk::data::var v = it->second;
					Value jv;
					v2j(cx,v,&jv);
					JS_SetProperty(cx,jo,str.c_str(),&jv);
				}
				//
				ret->setObject(*jo);
				return true ;
			}break;
		  case lyramilk::data::var::t_user:{
				lyramilk::data::datawrapper* urd = v.userdata();
				if(urd && urd->name() == objadapter_datawrapper::class_name()){
					objadapter_datawrapper* urd2 = (objadapter_datawrapper*)urd;
					if(urd2->subclassname() == mozjsobject_datawrapper::subclass_name()){
						mozjsobject_datawrapper* urdp = (mozjsobject_datawrapper*)urd2;
						JSObject* jsobj = JSID_TO_OBJECT(urdp->_jsid);
						ret->setObject(*jsobj);
						return true;
					}
				}

				JSObject* jo = JS_NewObject(cx,Jsvalify(&normalClass),nullptr,JS_GetGlobalObject(cx));
				Value jv;
				jv.setUnmarkedPtr((void*)&v);
				JS_SetProperty(cx,jo,"__script_var",&jv);
				ret->setObject(*jo);
				return true;
			}
		  case lyramilk::data::var::t_invalid:
			ret->setUndefined();
			return true;
		}
		ret->setUndefined();
		return true;
	}

	JSBool jsnewenum(JSContext *cx, JSHandleObject obj, JSIterateOp enum_op,Value *statep, jsid *idp)
	{
		js_obj_pack *ppack = (js_obj_pack*)JS_GetPrivate(obj);
		if(ppack && ppack->pthis){
			if(JSENUMERATE_INIT_ALL == enum_op){
			}else if(JSENUMERATE_INIT == enum_op){
				if(ppack->pthis->iterator_begin()){
					statep->setInt32(0);
				}else{
					statep->setNull();
				}
				if(idp){
					*idp = INT_TO_JSID(0);
				}
			}else if(JSENUMERATE_NEXT == enum_op){
				int idx = statep->toInt32();

				lyramilk::data::var v;
				if(!ppack->pthis->iterator_next(idx,&v)){
					ppack->pthis->iterator_end();
					statep->setNull();
					return JS_TRUE;
				}

				if(v.type() == lyramilk::data::var::t_invalid){
					return JS_TRUE;
				}

				Value jk;
				jk.setInt32(idx);
				JS_ValueToId(cx,jk,idp);

				Value jv;
				v2j(cx,v,&jv);

				JS_SetPropertyById(cx,obj,*idp,&jv);

				statep->setInt32(idx + 1);
			}else if(JSENUMERATE_DESTROY == enum_op){
				statep->setNull();
			}else{
			}
		}
		return JS_TRUE;
	}

	static JSBool iterator_next(JSContext *cx, unsigned argc, Value *vp)
	{
		CallArgs args = CallArgsFromVp(argc, vp);
		js_obj_pack *ppack = (js_obj_pack*)js_get_func_native(cx,&args.thisv().toObject());

		if(ppack && ppack->pthis){
			lyramilk::data::var v;
			if(!ppack->pthis->iterator_next(ppack->idx,&v)){
				ppack->pthis->iterator_end();
				JS_ThrowStopIteration(cx);
				return JS_FALSE;
			}
			++ppack->idx;

			Value jvret;
			v2j(cx,v,&jvret);

			args.rval().set(jvret);
			return JS_TRUE;
		}
		JS_ThrowStopIteration(cx);
		return JS_FALSE;
	}

	// spidermonkey 17 对 for of 句式的实现不标准，并没有用Symbol.Iterator作为新迭代属性，而是用了iterator属性。
	// iterator 方法返回一个包含next函数的对象。迭代器调用next，next的返回值作为迭代器得到的值，然后抛出StopIteration异常结束迭代。
	static JSFunctionSpec iterator_methods[] = {
		JS_FN("next",      iterator_next,       0, 0),
		JS_FS_END
	};

	static JSBool iterator_iterator(JSContext *cx, unsigned argc, Value *vp)
	{
		CallArgs args = CallArgsFromVp(argc, vp);
		js_obj_pack *ppack = (js_obj_pack*)JS_GetPrivate(&args.thisv().toObject());


		JSObject* jo = JS_NewObject(cx,Jsvalify(&normalClass),nullptr,JS_GetGlobalObject(cx));
		JS_DefineFunctions(cx, jo, iterator_methods);
		vp->setObject(*jo);
		if(ppack->pthis->iterator_begin()){
			ppack->idx = 0;
			js_set_func_native(cx,jo,ppack);
		}
		return JS_TRUE;
	}

/*
	JSObject* jsitor(JSContext *cx, JSHandleObject obj, JSBool keysonly)
	{
		JSObject* jo = JS_NewObject(cx,Jsvalify(&normalClass),nullptr,JS_GetGlobalObject(cx));
		JS_DefineFunctions(cx, jo, iterator_methods);
		return jo;
	}
*/
	JSBool jget(JSContext *cx, JSHandleObject obj, JSHandleId id, JSMutableHandleValue vp)
	{
		js_obj_pack *ppack = (js_obj_pack*)JS_GetPrivate(obj);
		if(ppack && ppack->pthis){
			Value jk;
			JS_IdToValue(cx,id,&jk);
			if(!jk.isString()) return JS_TRUE;

			lyramilk::data::string k;
			if(!j2s(cx,jk,&k) || k == "__iterator__" || k == "iterator"){
				return JS_PropertyStub(cx,obj,id,vp);
			}
			lyramilk::data::var v;
			if(!ppack->pthis->get_property(k,&v)){
				return JS_TRUE;
			}
			v2j(cx,v,vp.address());

		}
		return JS_TRUE;
	}

	JSBool jset(JSContext *cx, JSHandleObject obj, JSHandleId id, JSBool strict, JSMutableHandleValue vp)
	{

		js_obj_pack *ppack = (js_obj_pack*)JS_GetPrivate(obj);
		if(ppack && ppack->pthis){
			Value jk;
			JS_IdToValue(cx,id,&jk);
			if(!jk.isString()) return JS_TRUE;

			lyramilk::data::string k;
			if(!j2s(cx,jk,&k) || k == "__iterator__" || k == "iterator"){
				return JS_TRUE;
			}

			lyramilk::data::var v;
			j2v(cx,vp.get(),&v);

			if(!ppack->pthis->set_property(k,v)){
				return JS_TRUE;
				//return JS_PropertyStub(cx,obj,id,vp);
			}
		}
		return JS_TRUE;
	}

	void js_throw(JSContext *cx,const char* s,std::size_t n)
	{
		Value errorarg;
		{
			JSString* str = JS_NewStringCopyN(cx,s,n);
			if(str == nullptr){
				 return;
			}
			errorarg.setString(str);
		}

		Value jo;
		JS_GetProperty(cx,JS_GetGlobalObject(cx),"Error",&jo);
		JSObject* errobj = JS_New(cx,&jo.toObject(),1,&errorarg);
		if(errobj){
			Value exc;
			exc.setObject(*errobj);
			JS_SetPendingException(cx, exc);
		}
	}

	JSBool jsctr(JSContext *cx, unsigned argc, Value *vp)
	{
		CallArgs args = CallArgsFromVp(argc, vp);
		script_js::class_handler* h = (script_js::class_handler*)JS_GetPrivate(&args.callee());
		sclass *pnewobj = nullptr;
		if(h && h->ctr){
			lyramilk::data::array params;
			if(argc>0){
				Value* jv = JS_ARGV(cx,vp);
				params.resize(argc);
				for(unsigned i=0;i<argc;++i){
					j2v(cx,jv[i],&params[i]);
				}
			}

			try{
				pnewobj = h->ctr(params);
				if(pnewobj == nullptr){
					lyramilk::data::string str = D("创建对象失败");
					js_throw(cx,str.c_str(),str.size());
					return JS_FALSE;
				}
			}catch(lyramilk::data::string& str){
				js_throw(cx,str.c_str(),str.size());
				return JS_FALSE;
			}catch(const char* cstr){
				js_throw(cx,cstr,strlen(cstr));
				return JS_FALSE;
			}catch(std::exception& e){
				const char* cstr = e.what();
				js_throw(cx,cstr,strlen(cstr));
				return JS_FALSE;
			}catch(...){
				lyramilk::data::string str = D("未知异常");
				js_throw(cx,str.c_str(),str.size());
				return JS_FALSE;
			}
		}

		JSObject* jsret = JS_NewObject(cx,Jsvalify(&nativeClass),&args.callee(),JS_GetGlobalObject(cx));

		js_obj_pack *ppack = new js_obj_pack(h->dtr,h->eng,pnewobj);

		JS_SetPrivate(jsret,ppack);
		args.rval().setObject(*jsret);
		return JS_TRUE;
	}

	void jsdtr(js::FreeOp *fop, JSObject *obj)
	{
		//JSRuntime* rt = fop->runtime();
		js_obj_pack *ppack = (js_obj_pack*)JS_GetPrivate(obj);
		if(ppack && ppack->pthis && ppack->dtr){
			ppack->dtr(ppack->pthis);
			delete ppack;
		}
	}

	static JSBool js_func_adapter(JSContext *cx, unsigned argc, Value *vp)
	{
		CallArgs args = CallArgsFromVp(argc, vp);
		
		engine::functional_type_inclass pfun = (engine::functional_type_inclass)js_get_func_native(cx,&args.callee());
		JSObject* thisv = &args.thisv().toObject();

		js_obj_pack *ppack = (js_obj_pack*)JS_GetPrivate(thisv);

		if(pfun && ppack){
			if(ppack->magic == 0x21){
				script_js::class_handler* pp = (script_js::class_handler*)reinterpret_cast<script_js::class_handler*>(ppack);
				lyramilk::data::map info;
				info[engine::s_env_engine()].assign(engine_datawrapper(pp->eng));

				lyramilk::data::array params;
				params.resize(argc);
				for(unsigned i=0;i<argc;++i){
					j2v(cx,args[i],&params[i]);
				}

				try{
					lyramilk::data::var ret = pfun(params,info,nullptr);
					Value jsret;
					if(v2j(cx,ret,&jsret)){
						args.rval().set(jsret);
						return JS_TRUE;
					}
					return JS_FALSE;
				}catch(const lyramilk::data::string& str){
					js_throw(cx,str.c_str(),str.size());
					return JS_FALSE;
				}catch(const char* cstr){
					js_throw(cx,cstr,strlen(cstr));
					return JS_FALSE;
				}catch(std::exception& e){
					const char* cstr = e.what();
					js_throw(cx,cstr,strlen(cstr));
					return JS_FALSE;
				}catch(...){
					lyramilk::data::string str = D("未知异常");
					js_throw(cx,str.c_str(),str.size());
					return JS_FALSE;
				}
			}else if(ppack->magic == 0x22 && ppack->pthis){
				lyramilk::data::array params;
				params.resize(argc);
				for(unsigned i=0;i<argc;++i){
					j2v(cx,args[i],&params[i]);
				}

				try{

					if(ppack->info.find(engine::s_script_object()) == ppack->info.end()){
						jsid joid;
						JS_GetObjectId(cx,thisv,&joid);
						ppack->info[engine::s_script_object()].assign(mozjsobject_datawrapper(ppack->eng,joid,ppack->pthis));
					}

					lyramilk::data::var ret = pfun(params,ppack->info,ppack->pthis);
					Value jsret;
					if(v2j(cx,ret,&jsret)){
						args.rval().set(jsret);
						return JS_TRUE;
					}
				}catch(const lyramilk::data::string& str){
					js_throw(cx,str.c_str(),str.size());
					return JS_FALSE;
				}catch(const char* cstr){
					js_throw(cx,cstr,strlen(cstr));
					return JS_FALSE;
				}catch(std::exception& e){
					const char* cstr = e.what();
					js_throw(cx,cstr,strlen(cstr));
					return JS_FALSE;
				}catch(...){
					lyramilk::data::string str = D("未知异常");
					js_throw(cx,str.c_str(),str.size());
					return JS_FALSE;
				}
			}
		}
		return JS_TRUE;
	}

	JSBool script_js::js_func_adapter_noclass(JSContext *cx, unsigned argc, Value *vp)
	{
		CallArgs args = CallArgsFromVp(argc, vp);
		func_handler* h = (func_handler*)js_get_func_native(cx,&args.callee());

		lyramilk::data::array params;
		params.resize(argc);
		for(unsigned i=0;i<argc;++i){
			j2v(cx,args[i],&params[i]);
		}
		try{
			lyramilk::data::var ret = h->func(params,h->eng->info);
			Value jsret;
			if(v2j(cx,ret,&jsret)){
				args.rval().set(jsret);
				return JS_TRUE;
			}
		}catch(const lyramilk::data::string& str){
			js_throw(cx,str.c_str(),str.size());
			return JS_FALSE;
		}catch(const char* cstr){
			js_throw(cx,cstr,strlen(cstr));
			return JS_FALSE;
		}catch(std::exception& e){
			const char* cstr = e.what();
			js_throw(cx,cstr,strlen(cstr));
			return JS_FALSE;
		}catch(...){
			lyramilk::data::string str = D("未知异常");
			js_throw(cx,str.c_str(),str.size());
			return JS_FALSE;
		}
		return JS_TRUE;	
	}

	JSBool jsinstanceof(JSContext *cx, JSHandleObject obj, const Value *v, JSBool *bp)
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
	script_js::script_js():rt(nullptr),cx_template(nullptr)
	{
		info[engine::s_env_engine()].assign(engine_datawrapper(this));

		rt = JS_NewRuntime(128*1024L*1024L);
		cx_template = JS_NewContext(rt, 8192);
		JS_SetCStringsAreUTF8();

		//JS_SetRuntimePrivate(rt,cx_template);
		JS_SetOptions(cx_template, JSOPTION_VAROBJFIX | JSOPTION_METHODJIT);
		JS_SetVersion(cx_template, JSVERSION_LATEST);
		JS_SetErrorReporter(cx_template, script_js_error_report);

		JSObject * glob = JS_NewGlobalObject(cx_template, Jsvalify(&globalClass), nullptr);
		JS::RootedObject global(cx_template, glob);
		JS_InitStandardClasses(cx_template,global);
		//js_InitExceptionClasses
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

	bool script_js::load_string(const lyramilk::data::string& scriptstring)
	{
		JS_SetRuntimeThread(rt);
		init();
		JSContext* selectedcx = (JSContext *)JS_GetRuntimePrivate(rt);
		JSObject* global = JS_GetGlobalObject(selectedcx);

		JSScript* script = JS_CompileScript(selectedcx,global,scriptstring.c_str(),scriptstring.size(),nullptr,1);
		if(script){
			return !!JS_ExecuteScript(selectedcx,global,script,nullptr);
		}
		return false;
	}

		struct bytecode
		{
			time_t tm;
			std::string code;
		};

	bool script_js::load_file(const lyramilk::data::string& scriptfile)
	{
		if(!scriptfilename.empty()){
			return false;
		}

		scriptfilename = scriptfile;

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
					script = JS_DecodeScript(selectedcx,(const void*)c.code.c_str(),c.code.size(),nullptr,nullptr);
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

	bool script_js::load_module(const lyramilk::data::string& modulefile)
	{
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
		if(0 !=::stat(modulefile.c_str(),&st)){
			return false;
		}
		{
			//尝试读取
			lyramilk::threading::mutex_sync _(bytecodelock.r());
			std::map<lyramilk::data::string,bytecode>::const_iterator it = bytecodemap.find(modulefile);
			if(it!=bytecodemap.end()){
				const bytecode& c = it->second;
				if(st.st_mtime == c.tm){
					script = JS_DecodeScript(selectedcx,(const void*)c.code.c_str(),c.code.size(),nullptr,nullptr);
				}
			}
		}
		if(script){
			return !!JS_ExecuteScript(selectedcx,global,script,nullptr);
		}else{
			JS::CompileOptions options(selectedcx);
			//options.setSourcePolicy(JS::CompileOptions::NO_SOURCE);
			JS::RootedObject g(selectedcx,global);
			script = JS::Compile(selectedcx,g,options,modulefile.c_str());
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
						bytecodemap[modulefile] = c;
					}
				}
				return true;
			}
			return false;
		}
	}






	bool script_js::call(const lyramilk::data::var& func,const lyramilk::data::array& args,lyramilk::data::var* ret)
	{
		JS_SetRuntimeThread(rt);
		init();
		JSContext* selectedcx = (JSContext *)JS_GetRuntimePrivate(rt);
		JSObject* global = JS_GetGlobalObject(selectedcx);

		if(func.type_like(lyramilk::data::var::t_str)){
			Value retval;
			std::vector<Value> jvs;
			jvs.resize(args.size());
			for(std::size_t i=0;i < args.size();++i){
				v2j(selectedcx,args[i],&jvs[i]);
			}

			if(JS_TRUE == JS_CallFunctionName(selectedcx,global,func.str().c_str(),jvs.size(),jvs.data(),&retval)){
				if(ret){
					j2v(selectedcx,retval,ret);
				}
				return true;
			}
		}else if(func.type() == lyramilk::data::var::t_user){
			//调用回调函数
			lyramilk::data::datawrapper *urd = func.userdata();
			if(urd && urd->name() == mozjsobject_datawrapper::class_name()){
				mozjsobject_datawrapper* urdp = (mozjsobject_datawrapper*)urd;


				jsid jid = urdp->_jsid;
				if(!jid) return false;
				Value retval;
				Value jv;
				JS_IdToValue(selectedcx,jid,&jv);
				JSFunction *fun = JS_ValueToFunction(selectedcx,jv);

				std::vector<Value> jvs;
				jvs.resize(args.size());
				for(std::size_t i=0;i < args.size();++i){
					v2j(selectedcx,args[i],&jvs[i]);
				}

				if(JS_TRUE == JS_CallFunction(selectedcx,global,fun,jvs.size(),jvs.data(),&retval)){
					if(ret){
						j2v(selectedcx,retval,ret);
					}
					return true;
				}
			}
		}
		return false;
	}

	bool script_js::call_method(objadapter_datawrapper* obj,const lyramilk::data::var& meth,const lyramilk::data::array& args,lyramilk::data::var* ret)
	{
		JS_SetRuntimeThread(rt);
		init();
		JSContext* selectedcx = (JSContext *)JS_GetRuntimePrivate(rt);

		if(obj->subclassname() == mozjsobject_datawrapper::subclass_name()){
			mozjsobject_datawrapper* urdp = (mozjsobject_datawrapper*)obj;
			JSObject* jsobj = JSID_TO_OBJECT(urdp->_jsid);

			if(meth.type_like(lyramilk::data::var::t_str)){
				Value retval;
				std::vector<Value> jvs;
				jvs.resize(args.size());
				for(std::size_t i=0;i < args.size();++i){
					v2j(selectedcx,args[i],&jvs[i]);
				}
				if(JS_TRUE == JS_CallFunctionName(selectedcx,jsobj,meth.str().c_str(),jvs.size(),jvs.data(),&retval)){
					if(ret){
						j2v(selectedcx,retval,ret);
					}
					return true;
				}
			}else if(meth.type() == lyramilk::data::var::t_user){
				//调用回调函数
				lyramilk::data::datawrapper *urd = meth.userdata();
				if(urd && urd->name() == mozjsobject_datawrapper::class_name()){
					mozjsobject_datawrapper* urdp = (mozjsobject_datawrapper*)urd;


					jsid jid = urdp->_jsid;
					if(!jid) return false;
					Value retval;
					Value jv;
					JS_IdToValue(selectedcx,jid,&jv);
					JSFunction *fun = JS_ValueToFunction(selectedcx,jv);

					std::vector<Value> jvs;
					jvs.resize(args.size());
					for(std::size_t i=0;i < args.size();++i){
						v2j(selectedcx,args[i],&jvs[i]);
					}

					if(JS_TRUE == JS_CallFunction(selectedcx,jsobj,fun,jvs.size(),jvs.data(),&retval)){
						if(ret){
							j2v(selectedcx,retval,ret);
						}
						return true;
					}
				}
			}
		}
		return false;
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
		engine::reset();
	}

	void script_js::define(const lyramilk::data::string& classname,functional_map m,class_builder builder,class_destoryer destoryer)
	{
		JS_SetRuntimeThread(rt);
		JSContext* selectedcx = cx_template;
		JSObject* global = JS_GetGlobalObject(selectedcx);

		//std::cout << "注册类：" << classname << ",构造:" << (void*)builder << ",释放" << (void*)destoryer << std::endl;
		JSObject* jo = JS_DefineObject(selectedcx,global,classname.c_str(),Jsvalify(&classesClass),nullptr,0);
		assert(jo);
		jsid joid;
		JS_GetObjectId(selectedcx,jo,&joid);

		class_handler h;
		h.magic = 0x21;
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

		// 为了实现 for of
		JS_DefineFunction(selectedcx,jo,"iterator",iterator_iterator,0,0);
	}

	void script_js::define_const(const lyramilk::data::string& key,const lyramilk::data::var& value)
	{
		JS_SetRuntimeThread(rt);
		JSContext* selectedcx = cx_template;
		JSObject* global = JS_GetGlobalObject(selectedcx);
		Value jv;
		v2j(selectedcx,value,&jv);
		JS_DefineProperty(selectedcx,global,key.c_str(),jv,nullptr,nullptr,JSPROP_READONLY);
	}

	void script_js::define(const lyramilk::data::string& funcname,functional_type func)
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

	lyramilk::data::var script_js::createobject(const lyramilk::data::string& classname,const lyramilk::data::array& args)
	{
		JS_SetRuntimeThread(rt);
		init();
		JSContext* selectedcx = (JSContext *)JS_GetRuntimePrivate(rt);
		JSObject* global = JS_GetGlobalObject(selectedcx);

		Value jo;
		JS_GetProperty(selectedcx,global,classname.c_str(),&jo);
		if(!jo.isObject()) return lyramilk::data::var::nil;
		std::vector<Value> jvs;
		jvs.resize(args.size());
		for(std::size_t i=0;i < args.size();++i){
			v2j(selectedcx,args[i],&jvs[i]);
		}

		JSObject* jsobj = JS_New(selectedcx,&jo.toObject(),jvs.size(),jvs.data());

		if(jsobj == nullptr){
			return lyramilk::data::var::nil;
		}

		Value jv;
		jv.setObject(*jsobj);

		lyramilk::data::var v;
		j2v(selectedcx,jv,&v);
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
			JSObject * glob = JS_NewGlobalObject(cx, Jsvalify(&globalClass), nullptr);
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
		//nativeClass.ops.enumerate = jsnewenum;

		//nativeClass.ext.iteratorObject = jsitor;

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
