#include "script_lua.h"
#include "dict.h"
#include "log.h"
#include <fstream>
#include <cassert>
#include <string.h>
//script_lua

namespace lyramilk{namespace script{namespace lua
{
	static lyramilk::data::var luaget(lua_State *L,int index);

	void lua_lookup_stack(lua_State *L,int index,const char* flag = NULL)
	{
		int luatype = lua_type(L,index);
		std::cout << "[" << flag << "]查看Lua堆栈[" << index << "]=" << lua_typename(L,luatype) << "," << luatype << std::endl;
	}

	void lua_lookup_fullstack(lua_State *L,const char* flag = NULL)
	{
		if(!flag) flag = "";
		std::cout << "[" << flag << "]Lookup Lua stack begin (size=" << lua_gettop(L) << ")" << std::endl;
		for(int i=1;i<=lua_gettop(L);++i){
			lua_lookup_stack(L,i,flag);
		}
		std::cout << "[" << flag << "]Lookup Lua stack end" << std::endl;
	}
//__lua_native_object
	static int __var_gc(lua_State *L)
	{
		int argc = lua_gettop(L);
		if(argc < 1 || lua_type(L,1) != LUA_TUSERDATA){
			throw lyramilk::exception("参数错误");
		}

		void** p = (void**)lua_touserdata(L,1);
		delete reinterpret_cast<lyramilk::data::var*>(*p);
		return 0;
	}

	static void lua_pushvar(lua_State *L,const lyramilk::data::var* pv)
	{
		lyramilk::data::var** p = (lyramilk::data::var**)lua_newuserdata(L,sizeof(void*));

		lua_getglobal(L,"__lyramilk__data__var");
		if(lua_type(L,-1) == LUA_TNIL){
			lua_pop(L,1);
			luaL_newmetatable(L,"__lyramilk__data__var");
			lua_pushstring(L, "__gc");
			lua_pushcfunction(L, __var_gc);
			lua_settable(L, -3);
			lua_setglobal(L,"__lyramilk__data__var");
			lua_getglobal(L,"__lyramilk__data__var");
		}

		if(lua_type(L,-1) != LUA_TTABLE){
			lua_pushnil(L);
			return;
		}

		*p = new lyramilk::data::var;
		**p = *pv;

		lua_setmetatable(L,-2);
	}

	static lyramilk::data::var* lua_tovar(lua_State *L,int index)
	{
		lyramilk::data::var** p = (lyramilk::data::var**)lua_touserdata(L,index);
		if(p && *p){
			return *p;
		}
		return nullptr;
	}

	static lyramilk::data::var luaget(lua_State *L,int index)
	{
		lyramilk::data::var v;
		if(index < 0){
			int c = lua_gettop(L);
			index = c + index + 1;
		}
		int tlua = lua_type(L,index);
		switch(tlua)
		{
		  case LUA_TBOOLEAN:{
				return lua_toboolean(L,index) != 0;
			}break;
		  case LUA_TNUMBER:{
				return lua_tointeger(L,index);
			}break;
		  case LUA_TSTRING:{
				return lua_tostring(L,index);
			}break;
		  case LUA_TLIGHTUSERDATA:
		  case LUA_TUSERDATA:{
				lyramilk::data::var* p = lua_tovar(L,index);
				if(p){
					return *p;
				}
				return lyramilk::data::var::nil;
			}break;
		  case LUA_TTABLE:{
				lyramilk::data::map m;
				lua_pushnil(L);

				while (lua_next(L, index)) {
					lyramilk::data::var key = luaget(L,-2);
					lyramilk::data::var value = luaget(L,-1);
					lua_pop(L,1);
					m[key] = value;
				}
				v = m;
				return v;
			}break;
		  case LUA_TFUNCTION:{
				//未测试
				lyramilk::data::var v;
				v.assign(engine::s_user_functionid(),(void*)lua_tocfunction(L,index));
				return v;
				return lyramilk::data::var::nil;
			}break;
		  default:
			lyramilk::klog(lyramilk::log::warning,"lyramilk.script.lua.getluavalue") << lyramilk::kdict("不可识别的类型：%s",lua_typename(L,tlua)) << std::endl;
			return lyramilk::data::var::nil;
		}
		return lyramilk::data::var::nil;
	}

	static bool luaset(lua_State *L,const lyramilk::data::var& v)
	{
		switch(v.type()){
		  case lyramilk::data::var::t_bool:
			lua_pushboolean(L,v);
			break;
		  case lyramilk::data::var::t_int:
		  case lyramilk::data::var::t_uint:
			lua_pushinteger(L,v);
			break;
		  case lyramilk::data::var::t_double:
			lua_pushnumber(L,v);
			break;
		  case lyramilk::data::var::t_bin:
			lua_pushvar(L,&v);
			break;
		  case lyramilk::data::var::t_str:
		  case lyramilk::data::var::t_wstr:{
				lyramilk::data::string str = v;
				lua_pushstring(L,str.c_str());
			}
			break;
		  case lyramilk::data::var::t_array:{
				const lyramilk::data::array &a = v;
				lyramilk::data::array::const_iterator it = a.begin();
				lua_newtable(L);
				int itab = lua_gettop(L);
				for(int i=0;it!=a.end();++it,++i){
					luaset(L,i);
					luaset(L,*it);
					lua_settable(L,itab);
				}
			}
			break;
		  case lyramilk::data::var::t_map:{
				const lyramilk::data::map &m = v;
				lyramilk::data::map::const_iterator it = m.begin();
				lua_newtable(L);
				int itab = lua_gettop(L);
				for(;it!=m.end();++it){
					luaset(L,it->first);
					luaset(L,it->second);
					lua_settable(L,itab);
				}
			}
			break;
		  case lyramilk::data::var::t_user:
			if(v.userdata(engine::s_user_objectid())){
				const void* pid = v.userdata(engine::s_user_objectid());
				//const void* pobj = v.userdata(engine::s_user_nativeobject());
				lyramilk::data::string* pstr = (lyramilk::data::string*)pid;
				
				lyramilk::data::var** p = (lyramilk::data::var**)lua_newuserdata(L,sizeof(void*));
				*p = new lyramilk::data::var;
				(**p) = v;
				//lua_pushvar(L,&v);

				//lua_pushlightuserdata(L,(void*)pobj);
				lua_getglobal(L,pstr->c_str());
				lua_setmetatable(L,-2);
				break;
			}
			lua_pushvar(L,&v);
			break;
		  default:
			lyramilk::klog(lyramilk::log::warning,"lyramilk.script.lua.setluavalue") << lyramilk::kdict("不可识别的类型：%s",lyramilk::data::var::type_name(v.type()).c_str()) << std::endl;
			lua_pushnil(L);
			return false;
		}
		return true;
	}

	static lyramilk::data::array stov(lua_State *L)
	{
		lyramilk::data::array r;
		int m=lua_gettop(L);
		for(int index=1;index<=m;++index){
			lyramilk::data::var v = luaget(L,index);
			r.push_back(v);
		}
		return r;
	}

	static int vtos(lua_State *L,const lyramilk::data::array& v)
	{
		lyramilk::data::array::const_iterator it = v.begin();
		for(;it!=v.end();++it){
			const lyramilk::data::var &v = *it;
			luaset(L,v);
		}
		return v.size();
	}


	static int lua_func_adapter_single(lua_State *L)
	{
		void* pfunc = lua_touserdata(L,lua_upvalueindex(1));
		if(pfunc){
			script_lua::functional_type func = (script_lua::functional_type)pfunc;

			lyramilk::data::array params = stov(L);
			lua_pop(L,lua_gettop(L));

			lua_getglobal(L,"__global_engine_pointer");
			void* penv = lua_touserdata(L,-1);
			lua_pop(L,1);

			lyramilk::data::map env;
			env[engine::s_env_engine()].assign(engine::s_env_engine(),penv);
			lyramilk::data::var ret = func(params,env);
			luaset(L,ret);
			return 1;
		}
		return 0;
	}

	static int lua_func_adapter(lua_State *L)
	{
		lyramilk::data::var** pthis = (lyramilk::data::var**)lua_touserdata(L,1);
		lua_remove(L,1);
		void* pfunc = lua_touserdata(L,lua_upvalueindex(1));
		script_lua::metainfo* mi = (script_lua::metainfo*)lua_touserdata(L,lua_upvalueindex(2));

		if(pfunc && pthis && mi){
			script_lua::functional_type_inclass func = (script_lua::functional_type_inclass)pfunc;

			lyramilk::data::array params = stov(L);
			lua_pop(L,lua_gettop(L));

			lyramilk::data::map env;
			//env[engine::s_env_this()].assign(engine::s_user_nativeobject(),(*pthis)->userdata(engine::s_user_nativeobject()));
			env[engine::s_env_engine()].assign(engine::s_env_engine(),mi->env);
			lyramilk::data::var ret = func(params,env,(void*)(*pthis)->userdata(engine::s_user_nativeobject()));
			luaset(L,ret);
			return 1;

		}
		return 0;
	}

	struct lua_metainfo
	{
		script_lua::class_builder ctr;
		script_lua::class_destoryer dtr;
		script_lua::functional_map funcmap;
		void* self;
	};

	static int lua_func_meta_index(lua_State *L)
	{
		if(lua_gettop(L) < 2 || lua_type(L,2) != LUA_TSTRING){
			lyramilk::klog(lyramilk::log::warning,"lyramilk.script.lua.engine.meta._index") << lyramilk::kdict("参数错误") << std::endl;
			return 0;
		}
		const char* name = lua_tostring(L,2);
		if(strncmp(name,"__lua_metainfo",14) == 0){
			lua_getmetatable(L,1);
			lua_getfield(L,-1,name);
			return 1;
		}
		lua_getmetatable(L,1);
		lua_getfield(L,-1,name);
		lua_getfield(L,-2,"__lua_metainfo");
		lua_pushcclosure(L,lua_func_adapter,2);
		if(LUA_TFUNCTION == lua_type(L,-1)){
			return 1;
		}
		return 0;
	}

	static int lua_func_meta_ctr(lua_State *L)
	{
		int argc = lua_gettop(L);
		if(argc < 1) return 0;
		lua_getfield(L,1,"__lua_metainfo");
		void* cls = lua_touserdata(L,-1);
		if(cls == NULL) return 0;
		lua_pop(L,1);
		script_lua::metainfo* mi = (script_lua::metainfo*)cls;
		lyramilk::data::array r;
		int m=lua_gettop(L);
		for(int index=2;index<=m;++index){
			lyramilk::data::var v = luaget(L,index);
			r.push_back(v);
		}
		lua_pop(L,m-1);

		lyramilk::data::var** p = (lyramilk::data::var**)lua_newuserdata(L,sizeof(void*));
		*p = new lyramilk::data::var;
		(*p)->assign(engine::s_user_objectid(),&mi->name);
		(*p)->assign(engine::s_user_nativeobject(),mi->ctr(r));
		lua_insert(L,1);
		lua_setmetatable(L,-2);

		return 1;
	}

	static int lua_func_meta_dtr(lua_State *L)
	{
		int argc = lua_gettop(L);
		if(argc != 1) return 0;
		lua_getfield(L,1,"__lua_metainfo");
		void* cls = lua_touserdata(L,-1);
		if(cls == NULL) return 0;
		lua_pop(L,1);
		script_lua::metainfo* mi = (script_lua::metainfo*)cls;
		lyramilk::data::var** p = (lyramilk::data::var**)lua_touserdata(L,1);
		lyramilk::script::sclass* pdata = (lyramilk::script::sclass*)(*p)->userdata(engine::s_user_nativeobject());
		mi->dtr(pdata);
		delete *p;
		return 0;
	}

	script_lua::script_lua():L(nullptr),L_template(luaL_newstate())
	{
		luaL_openlibs(L_template);

		lua_pushlightuserdata(L_template,this);
		lua_setglobal(L_template,"__global_engine_pointer");
	}

	script_lua::~script_lua()
	{
		lua_close(L_template);
	}

	bool script_lua::load_string(const lyramilk::data::string& scriptstring)
	{
		init();
		if(luaL_loadstring(L, scriptstring.c_str()) == 0){
			if(lua_pcall(L,0,LUA_MULTRET,0) == 0){
				lyramilk::data::var sret = lyramilk::data::var::nil;
				if(lua_gettop(L) > 0){
					sret = luaget(L,-1);
				}
				clear();
				//lua_gc(L,LUA_GCCOLLECT,0);
				return sret;
			}
			lyramilk::data::string err = lua_tostring(L, -1);
			clear();
			//lua_gc(L,LUA_GCCOLLECT,0);
			lyramilk::klog(lyramilk::log::error,"lyramilk.script.lua.engine.load_string") << err << std::endl;
			return lyramilk::data::var::nil;
			return true;
		}
		lyramilk::data::string err = lua_tostring(L, -1);
		lyramilk::klog(lyramilk::log::error,"lyramilk.script.lua.engine.load_string") << lyramilk::kdict(err.c_str()) << std::endl;
		return false;
	}

	bool script_lua::load_file(const lyramilk::data::string& scriptfile)
	{
		init();
		if(!scriptfilename.empty()){
			return false;
		}

		scriptfilename = scriptfile;

		if(luaL_loadfile(L, scriptfile.c_str()) == 0){
			if(lua_pcall(L,0,LUA_MULTRET,0) == 0){
				lyramilk::data::var sret = lyramilk::data::var::nil;
				if(lua_gettop(L) > 0){
					sret = luaget(L,-1);
				}
				clear();
				//lua_gc(L,LUA_GCCOLLECT,0);
				return true;
			}
			const char* perr = lua_tostring(L, -1);
			lyramilk::data::string err = perr?perr:"";
			clear();
			//lua_gc(L,LUA_GCCOLLECT,0);
			lyramilk::klog(lyramilk::log::error,"lyramilk.script.lua.engine.load_file") << err << std::endl;
			scriptfilename.clear();
			return false;
		}
		const char* perr = lua_tostring(L, -1);
		lyramilk::data::string err = perr?perr:"";
		lyramilk::klog(lyramilk::log::error,"lyramilk.script.lua.engine.load_file") << err << std::endl;
		scriptfilename.clear();
		return false;
	}

	bool script_lua::load_module(const lyramilk::data::string& modulefile)
	{
		init();
		if(luaL_loadfile(L, modulefile.c_str()) == 0){
			if(lua_pcall(L,0,LUA_MULTRET,0) == 0){
				lyramilk::data::var sret = lyramilk::data::var::nil;
				if(lua_gettop(L) > 0){
					sret = luaget(L,-1);
				}
				clear();
				//lua_gc(L,LUA_GCCOLLECT,0);
				return true;
			}
			const char* perr = lua_tostring(L, -1);
			lyramilk::data::string err = perr?perr:"";
			clear();
			//lua_gc(L,LUA_GCCOLLECT,0);
			lyramilk::klog(lyramilk::log::error,"lyramilk.script.lua.engine.load_file") << err << std::endl;
			return false;
		}
		const char* perr = lua_tostring(L, -1);
		lyramilk::data::string err = perr?perr:"";
		lyramilk::klog(lyramilk::log::error,"lyramilk.script.lua.engine.load_file") << err << std::endl;
		return false;
	}

	bool script_lua::call(const lyramilk::data::var& func,const lyramilk::data::array& args,lyramilk::data::var* ret)
	{
		if(ret) ret->clear();
		init();
		lua_getglobal(L, func.str().c_str());
		vtos(L,args);
		if(lua_pcall(L,args.size(),LUA_MULTRET,0) == 0){
			if(lua_gettop(L) > 0){
				if(ret) *ret = luaget(L,-1);
			}
			clear();
			//lua_gc(L,LUA_GCCOLLECT,0);
			return true;
		}
		lyramilk::data::string err = lua_tostring(L, -1);
		clear();
		//lua_gc(L,LUA_GCCOLLECT,0);
		lyramilk::klog(lyramilk::log::error,"lyramilk.script.lua.engine.pcall") << err << std::endl;
		return false;
	}

	void script_lua::reset()
	{
		if(L){
			lua_pop(L_template,1);
			lua_gc(L_template,LUA_GCCOLLECT,0);
			L = nullptr;
			scriptfilename.clear();
		}
		engine::reset();
	}

	void script_lua::define(const lyramilk::data::string& classname,functional_map m,class_builder builder,class_destoryer destoryer)
	{
		//std::cout << "注册类：" << classname << ",构造:" << (void*)builder << ",释放" << (void*)destoryer << std::endl;
		luaL_newmetatable(L_template,classname.c_str());

		metainfo& mi = minfo[classname];
		mi.ctr = builder;
		mi.dtr = destoryer;
		mi.env = this;
		mi.name = classname;
		lua_pushstring(L_template,"__lua_metainfo");
		lua_pushlightuserdata(L_template,&mi);
		lua_settable(L_template, -3);
		functional_map::iterator it = m.begin();
		for(;it!=m.end();++it){
			//std::cout << "\t注册函数：" << it->first << "," << (void*)it->second << std::endl;
			lua_pushstring(L_template,it->first.c_str());
			lua_pushlightuserdata(L_template,(void*)it->second);
			lua_settable(L_template, -3);
		}

		lua_pushstring(L_template, "__index");
		lua_pushcfunction(L_template, lua_func_meta_index);
		lua_settable(L_template, -3);

		lua_pushstring(L_template, "__gc");
		lua_pushcfunction(L_template, lua_func_meta_dtr);
		lua_settable(L_template, -3);

		lua_pushstring(L_template, "new");
		lua_pushcfunction(L_template, lua_func_meta_ctr);
		lua_settable(L_template, -3);

		lua_setglobal(L_template,classname.c_str());
	}
	

	void script_lua::define(const lyramilk::data::string& funcname,functional_type func)
	{
		//std::cout << "注册全局函数：" << funcname << "," << (void*)func << std::endl;
		lua_pushlightuserdata(L_template,(void*)func);
		lua_pushcclosure(L_template, lua_func_adapter_single,1);
		lua_setglobal(L_template,funcname.c_str());
	}

	void script_lua::define_const(const lyramilk::data::string& key,const lyramilk::data::var& value)
	{
		luaset(L_template,value);
		lua_setglobal(L_template,key.c_str());
	}

	lyramilk::data::var script_lua::createobject(const lyramilk::data::string& classname,const lyramilk::data::array& args)
	{
		init();
		std::map<lyramilk::data::string,metainfo>::iterator it = minfo.find(classname);
		if(it==minfo.end()) return lyramilk::data::var::nil;
		//assert(lua_gettop(L) == 0);
		lua_getglobal(L,classname.c_str());
		lua_getfield(L,-1,"__lua_metainfo");
		script_lua::metainfo* mi = (script_lua::metainfo*)lua_touserdata(L,-1);
		lua_pop(L,2);
		if(mi == nullptr) return lyramilk::data::var::nil;

		lyramilk::data::var v;
		v.assign(engine::s_user_objectid(),&mi->name);
		v.userdata(engine::s_user_nativeobject(),mi->ctr(args));
		return v;
	}

	void script_lua::gc()
	{
		init();
		lua_gc(L,LUA_GCCOLLECT,0);
	}

	void script_lua::clear()
	{
		init();
		int stacksize = lua_gettop(L);
		lua_pop(L,stacksize);
	}


	lyramilk::data::string script_lua::name()
	{
		return "lua";
	}

	lyramilk::data::string script_lua::filename()
	{
		return scriptfilename;
	}

	bool script_lua::init()
	{
		if(L == nullptr){
			L = lua_newthread(L_template);
			return L != nullptr;
		}
		return false;
	}

	static lyramilk::script::engine* __ctr()
	{
		return new script_lua();
	}

	static void __dtr(lyramilk::script::engine* eng)
	{
		delete (script_lua*)eng;
	}

	static bool ___init()
	{
		lyramilk::script::engine::define("lua",__ctr,__dtr);
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


/**
		lyramilk::data::var v(engine::s_user_objectid(),(void*)jsretid);
		v.userdata(engine::s_user_nativeobject(),pnewobj);

**/