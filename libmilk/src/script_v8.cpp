#include "script_v8.h"
#include "multilanguage.h"
#include "log.h"
#include <fstream>
#include <cassert>
#include <errno.h>
#include <string.h>
//script_v8
namespace lyramilk{namespace script{namespace js
{
	using namespace v8;


	struct frame_scope
	{
		script_v8::frame sf;
		script_v8::frame*& pf;
		script_v8::frame* old;

		frame_scope(script_v8::frame*& pframe):pf(pframe)
		{
			old = pf;
			pf = &sf;
		}
		~frame_scope()
		{
			pf = old;
		}
	};

	script_v8::pack::pack():ctr(NULL),dtr(NULL),pthis(NULL),env(NULL)
	{
	}

	script_v8::pack::pack(lyramilk::data::string name,engine::class_builder ca,engine::class_destoryer da,script_v8* _env,void* dt):cname(name),ctr(ca),dtr(da),pthis(dt),env(_env)
	{}

	script_v8::pack::~pack()
	{
		if(dtr && pthis){
			dtr(pthis);
		}
	}

	lyramilk::data::var script_v8::j2v(Handle<Value> d)
	{
		if(d->IsUndefined() || d->IsNull()){
COUT << "IsUndefined" << std::endl;
			return lyramilk::data::var::nil;
		}else if(d->IsString()){
			String::Utf8Value sstr(d->ToDetailString());
			//d->ToString()
			return lyramilk::data::string(*sstr,sstr.length());
		}else if(d->IsExternal()){
			Local<External> obj = External::Cast(*d);

			void* v = obj->Value();
			assert(v);
			lyramilk::data::var* pv = (lyramilk::data::var*)v;
			return *pv;
		}else if(d->IsFunction()){
COUT << "IsFunction" << std::endl;
		}else if(d->IsArray()){
COUT << "IsArray" << std::endl;
		}else if(d->IsObject()){
			Local<Object> obj = d->ToObject();
			pack* p = (pack*)obj->GetPointerFromInternalField(0);
			if(p){
				if(!p->pthis) return lyramilk::data::var::nil;
				lyramilk::data::var v("__script_native_object",p->pthis);

				Handle<Object> ho = obj->Clone();
				void* pobj = (void*)*ho;
				v.userdata("__script_object_ptr",pobj);
				stk[pobj] = v8::Persistent<v8::Object>::New(ho);
				return v;
			}
			Local<Array> jsar = obj->GetOwnPropertyNames();
			unsigned int count = jsar->Length();
			unsigned int i = 0;
			lyramilk::data::var::map m;
			for(;i<count;++i){
				Local<Value> k = jsar->Get(i);
				Local<Value> v = obj->Get(k);
				m[j2v(k)] = j2v(v);
			}
			return m;
		}else if(d->IsBoolean()){
			return d->ToBoolean()->Value();
		}else if(d->IsInt32()){
			return d->ToInt32()->Value();
		}else if(d->IsUint32()){
			return d->ToUint32()->Value();
		}else if(d->IsDate()){
COUT << "IsDate" << std::endl;
		}else if(d->IsBooleanObject()){
COUT << "IsBooleanObject" << std::endl;
		}else if(d->IsNumberObject()){
COUT << "IsNumberObject" << std::endl;
		}else if(d->IsStringObject()){
COUT << "IsStringObject" << std::endl;
		}else if(d->IsNativeError()){
COUT << "IsNativeError" << std::endl;
		}else if(d->IsRegExp()){
COUT << "IsRegExp" << std::endl;
		}else if(d->IsNumber()){
COUT << "IsNumber" << std::endl;
		}


		String::Utf8Value dstr(d->ToDetailString());
COUT << "j2v----------------------------------------->发现未知参数" << (*dstr) << std::endl;
		TODO();
		return lyramilk::data::var::nil;
	}

	Handle<Value> script_v8::v2j(lyramilk::data::var v)
	{
		switch(v.type()){
		  case lyramilk::data::var::t_bool:
				return Boolean::New((bool)v);
		  case lyramilk::data::var::t_int8:
		  case lyramilk::data::var::t_int16:
		  case lyramilk::data::var::t_int32:
		  case lyramilk::data::var::t_int64:{
				return Int32::New(v);
			}break;
		  case lyramilk::data::var::t_uint8:
		  case lyramilk::data::var::t_uint16:
		  case lyramilk::data::var::t_uint32:
		  case lyramilk::data::var::t_uint64:{
				return Uint32::New(v);
			}break;
		  case lyramilk::data::var::t_double:{
				return Number::New(v);
			}break;
		  case lyramilk::data::var::t_bin:{
				lyramilk::data::var* pv = build_var();
				*pv = v;
				return External::New(pv);
			}break;
		  case lyramilk::data::var::t_wstr:{
			}break;
		  case lyramilk::data::var::t_str:{
				lyramilk::data::string str = v.str();
				return String::New(str.c_str(),str.size());
			}break;
		  case lyramilk::data::var::t_array:{
			}break;
		  case lyramilk::data::var::t_map:{
				lyramilk::data::var::map m = v;
				lyramilk::data::var::map::iterator it = m.begin();
				Local<Object> obj = Object::New();
				for(;it!=m.end();++it){
					Handle<Value> k = v2j(it->first);
					Handle<Value> v = v2j(it->second);
					obj->Set(k,v);
				}
				return obj;
			}break;
		  case lyramilk::data::var::t_invalid:{
				return Null();
			}break;
		  case lyramilk::data::var::t_user:{
				void* pjsobj = (void*)v.userdata("__script_object_ptr");
				if(pjsobj){
					return stk[pjsobj];
				}
				lyramilk::data::var* pv = build_var();
				*pv = v;
				return External::New(pv);
			}break;
		  default:
			return Undefined();
		}

		TODO();
	}

	lyramilk::data::var* script_v8::build_var()
	{
		frame* p = (pframe == NULL ? (&gf) : pframe);
		assert(p);
		p->v.push_back(lyramilk::data::var());
		return &p->v.back();
	}

	script_v8::pack* script_v8::build_pack(lyramilk::data::string name,engine::class_builder ca,engine::class_destoryer da,script_v8* _env,void* dt)
	{
		frame* p = ((pframe == NULL || forceglobal) ? (&gf) : pframe);
		lyramilk::data::string cas = (p == pframe)?"局部":"全局";
		assert(p);
		p->a.push_back(script_v8::pack());
		script_v8::pack* k = &p->a.back();
		k->ctr = ca;
		k->dtr = da;
		k->pthis = dt;
		k->env = _env;
		k->cname = name;
		return k;
	}

	script_v8::script_v8():log(lyramilk::klog,"lyramilk.script.js.v8")
	{
		isolate = NULL;
		pframe = NULL;
		forceglobal = false;
	}

	script_v8::~script_v8()
	{
		{
			Isolate::Scope iscope(isolate);  
			Locker locker(isolate);
			ctx.Dispose();
		}
		isolate->Dispose();
	}

	bool script_v8::init(lyramilk::data::var::map m)
	{
		lyramilk::data::var::map::iterator it = m.find("v8.isolate");
		if(it!=m.end()){
			isolate = (Isolate*)it->second.userdata("v8.isolate");
		}
		if(isolate == NULL )isolate = Isolate::New();
		Isolate::Scope iscope(isolate);  
		Locker locker(isolate);
		HandleScope scope;
		globaltpl = Persistent<ObjectTemplate>::New(ObjectTemplate::New());

		ctx = v8::Context::New(NULL,globaltpl);
		return true;
	}

	bool script_v8::load_string(lyramilk::data::string scriptstring)
	{
		TODO();
	}

	bool script_v8::load_file(lyramilk::data::string scriptfile)
	{
		lyramilk::data::string str;
		str.reserve(16384);
		std::ifstream ifs;
		ifs.open(scriptfile.c_str(),std::ifstream::binary | std::ifstream::in);

		while(ifs){
			char buff[4096];
			ifs.read(buff,4096);
			str.append(buff,(unsigned int)ifs.gcount());
		}
		ifs.close();

		Isolate::Scope iscope(isolate);
		Locker locker(isolate);
		HandleScope scope;
		Context::Scope cscope(ctx);
		script = Persistent<Script>::New(Script::Compile(String::New(str.c_str(),str.size()),String::New(scriptfile.c_str(),scriptfile.size())));
		if(script.IsEmpty()){
			log(lyramilk::log::error,__FUNCTION__) << D("加载文件%s失败",scriptfile.c_str()) << std::endl;
			return false;
		}
		return true;
	}

	lyramilk::data::var script_v8::pcall(lyramilk::data::var::array args)
	{
		assert(isolate);
		Isolate::Scope iscope(isolate);
		Locker locker(isolate);
		HandleScope scope;
		Context::Scope cscope(ctx);

		TryCatch trycatch;
		forceglobal = true;
		/*Handle<Value> v = */script->Run();
		forceglobal = false;
		if(trycatch.HasCaught()){
			Handle<Value> exception = trycatch.Exception();
			Local<Message> msg = trycatch.Message();
			Handle<Value> str = msg->GetScriptResourceName();
			String::Utf8Value exception_str(exception);
			String::Utf8Value exception_file(str);
			log(lyramilk::log::error,__FUNCTION__) << D("%s(%d):%s",*exception_file,msg->GetLineNumber(),*exception_str) << std::endl;
			trycatch.Reset();
		}


		return true;
	}

	lyramilk::data::var script_v8::call(lyramilk::data::string func,lyramilk::data::var::array args)
	{
		assert(isolate);
		frame_scope fscope(pframe);
		Isolate::Scope iscope(isolate);
		Locker locker(isolate);
		HandleScope scope;
		Context::Scope cscope(ctx);
		Local<Value> jfunval = ctx->Global()->Get(String::New(func.c_str(),func.size()));
		assert(jfunval->IsFunction());
		Local<Function> jfun = Local<Function>::Cast(jfunval);

		std::vector<Handle<Value> > v;
		v.reserve(args.size());
		for(lyramilk::data::var::array::iterator it = args.begin();it!=args.end();++it){
			v.push_back(v2j(*it));
		}

		TryCatch trycatch;
		Local<Value> ret = jfun->Call(ctx->Global(),v.size(),v.data());
		if(trycatch.HasCaught()){
			Handle<Value> exception = trycatch.Exception();
			Local<Message> msg = trycatch.Message();
			Handle<Value> str = msg->GetScriptResourceName();
			String::Utf8Value exception_str(exception);
			String::Utf8Value exception_file(str);
			log(lyramilk::log::error,__FUNCTION__) << D("%s(%d):%s",*exception_file,msg->GetLineNumber(),*exception_str) << std::endl;
			trycatch.Reset();
			pframe = NULL;
			return lyramilk::data::var::nil;
		}

		pframe = fscope.old;


		gc();

		lyramilk::data::var vr = j2v(ret);
		return vr;
	}

	void script_v8::reset()
	{
		TODO();
	}

	static Handle<Value> v8_adapter(const Arguments& args)
	{
		Local<Object> objthis = args.This();
		//Local<Object> objee = args.Callee();
		Local<Value> jfun = args.Data();

		engine::functional_type pfun = (engine::functional_type)External::Unwrap(jfun);
		script_v8::pack* p = (script_v8::pack*)objthis->GetPointerFromInternalField(0);
		assert(pfun);
		assert(p);
		assert(Isolate::GetCurrent() == p->env->isolate && args.GetIsolate() == p->env->isolate);
		lyramilk::data::var::map env;
		env["this"].assign("this",p->pthis);
		env["env"].assign("env",p->env);
		lyramilk::data::var::array ar;
		for(int i =0;i<args.Length();++i){
			ar.push_back(p->env->j2v(args[i]));
		}

		lyramilk::data::var v = pfun(ar,env);
		return p->env->v2j(v);
	}
	static Handle<Value> v8_ctr(const Arguments& args)
	{
		Local<Object> objthis = args.This();

		Local<ObjectTemplate> tpl = ObjectTemplate::New();
		tpl->SetInternalFieldCount(1);
		Local<Object> objee = tpl->NewInstance();

		Local<Value> v = args.Data();
		script_v8::pack* p = (script_v8::pack*)External::Unwrap(v);
		assert(p);
		assert(Isolate::GetCurrent() == p->env->isolate && args.GetIsolate() == p->env->isolate);

		lyramilk::data::var::array ar;
		for(int i =0;i<args.Length();++i){
			ar.push_back(p->env->j2v(args[i]));
		}
		void* pobj = p->ctr(ar);

		script_v8::pack* p2 = p->env->build_pack(p->cname,p->ctr,p->dtr,p->env,pobj);

		objee->SetPrototype(objthis);
		objee->SetPointerInInternalField(0,p2);
		return objee;
	}

	void script_v8::define(lyramilk::data::string classname,functional_map m,class_builder builder,class_destoryer destoryer)
	{
		assert(isolate);
		Isolate::Scope iscope(isolate);
		Locker locker(isolate);
		HandleScope scope;
		Context::Scope cscope(ctx);

		functional_map::iterator it = m.begin();
//COUT << "注册类" << classname << ":ctr=" << (void*)builder << ",dtr=" << (void*)destoryer << std::endl;
		Handle<ObjectTemplate> obj = ObjectTemplate::New();
		obj->SetInternalFieldCount(1);

		pack* p = build_pack(classname,builder,destoryer,this,NULL);

		obj->SetCallAsFunctionHandler(v8_ctr,External::Wrap(p));
		for(;it!=m.end();++it){
//COUT << "\t注册函数" << classname << "." << it->first << ":" << (void*)it->second << std::endl;
			Handle<FunctionTemplate> fun = FunctionTemplate::New(v8_adapter,External::Wrap((void*)it->second));
			obj->Set(String::New(it->first.c_str(),it->first.size()), fun);
		}

		std::pair<lyramilk::data::string,v8::Persistent<v8::ObjectTemplate> > pr(classname,Persistent<ObjectTemplate>::New(obj));
		this->m.insert(pr);

		globaltpl->Set(String::New(classname.c_str(),classname.size()),obj);
		if(*ctx){
			Context::Scope cscope(ctx);
			Handle<Object> g = ctx->Global();
			g->Set(String::New(classname.c_str(),classname.size()),obj->NewInstance());
		}
	}

	lyramilk::data::var script_v8::createobject(lyramilk::data::string classname,lyramilk::data::var::array args)
	{
		assert(isolate);
		std::map<lyramilk::data::string,v8::Persistent<v8::ObjectTemplate> >::iterator it = m.find(classname);
		if(it==m.end()) return lyramilk::data::var::nil;

		Isolate::Scope iscope(isolate);
		Locker locker(isolate);
		HandleScope scope;
		Context::Scope cscope(ctx);


		frame* old = pframe;
		pframe = &tmp;

		/*
		Handle<Object> obj = it->second->NewInstance();
		if(obj.IsEmpty()){
			return lyramilk::data::var::nil;
		}*/
		Handle<Object> obj = it->second->NewInstance();

		std::vector<Handle<Value> > v;
		v.reserve(args.size());
		for(lyramilk::data::var::array::iterator it = args.begin();it!=args.end();++it){
			v.push_back(v2j(*it));
		}

		Local<Value> ret = obj->CallAsConstructor(v.size(),v.data());
		pframe = old;
		return j2v(ret);
	}

	void script_v8::gc()
	{
		stk.clear();
		tmp.a.clear();
		tmp.v.clear();

		COUT << "gf.a=" << gf.a.size() << ",gf.v=" << gf.v.size() << std::endl;

	}

}}}
