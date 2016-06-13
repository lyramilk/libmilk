#include "script_lua.h"
#include "multilanguage.h"
#include "log.h"
#include <fstream>
#include <cassert>
#include <string.h>
//script_lua
namespace lyramilk{namespace script{namespace lua
{

	void lua_lookup_stack(lua_State *L,int index,const char* flag = NULL)
	{
		std::cout << "[" << flag << "]查看Lua堆栈[" << index << "]=" << lua_typename(L,lua_type(L,index)) << std::endl;
	}

	void lua_lookup_fullstack(lua_State *L,const char* flag = NULL)
	{
		for(int i=1;i<=lua_gettop(L);++i){
			lua_lookup_stack(L,i,flag);
		}
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
				lyramilk::data::var::map m;
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
		  case lyramilk::data::var::t_int8:
		  case lyramilk::data::var::t_uint8:
		  case lyramilk::data::var::t_int16:
		  case lyramilk::data::var::t_uint16:
		  case lyramilk::data::var::t_int32:
		  case lyramilk::data::var::t_uint32:
		  case lyramilk::data::var::t_int64:
		  case lyramilk::data::var::t_uint64:
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
				const lyramilk::data::var::array &a = v;
				lyramilk::data::var::array::const_iterator it = a.begin();
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
				const lyramilk::data::var::map &m = v;
				lyramilk::data::var::map::const_iterator it = m.begin();
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
			if(v.userdata("__script_object_id")){
				const void* pid = v.userdata("__script_object_id");
				const void* pobj = v.userdata("__script_native_object");
				lyramilk::data::string* pstr = (lyramilk::data::string*)pid;

				lua_pushlightuserdata(L,(void*)pobj);
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

	static lyramilk::data::var::array stov(lua_State *L)
	{
		lyramilk::data::var::array r;
		int m=lua_gettop(L);
		for(int index=1;index<=m;++index){
			lyramilk::data::var v = luaget(L,index);
			r.push_back(v);
		}
		return r;
	}

	static int vtos(lua_State *L,lyramilk::data::var::array& v)
	{
		lyramilk::data::var::array::iterator it = v.begin();
		for(;it!=v.end();++it){
			lyramilk::data::var &v = *it;
			luaset(L,v);
		}
		return v.size();
	}


	script_lua::script_lua():L(luaL_newstate())
	{
		luaL_openlibs(L);

		lua_pushlightuserdata(L,this);
		lua_setglobal(L,"__global_engine_pointer");
	}

	script_lua::~script_lua()
	{
		lua_close(L);
	}

	bool script_lua::load_string(lyramilk::data::string scriptstring)
	{
		if(luaL_loadstring(L, scriptstring.c_str()) == 0){
			return true;
		}
		lyramilk::data::string err = lua_tostring(L, -1);
		lyramilk::klog(lyramilk::log::error,"lyramilk.script.lua.engine.load_string") << lyramilk::kdict(err.c_str()) << std::endl;
		return false;
	}

	bool script_lua::load_file(lyramilk::data::string scriptfile)
	{
		if(luaL_loadfile(L, scriptfile.c_str()) == 0){
			return true;
		}
		lyramilk::data::string err = lua_tostring(L, -1);
		lyramilk::klog(lyramilk::log::error,"lyramilk.script.lua.engine.load_file") << err << std::endl;
		return false;
	}

	lyramilk::data::var script_lua::pcall(lyramilk::data::var::array args)
	{
		vtos(L,args);

		if(lua_pcall(L,args.size(),LUA_MULTRET,0) == 0){
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
		lyramilk::klog(lyramilk::log::error,"lyramilk.script.lua.engine.pcall") << err << std::endl;
		return lyramilk::data::var::nil;
	}

	lyramilk::data::var script_lua::call(lyramilk::data::string func,lyramilk::data::var::array args)
	{
		lua_getglobal(L, func.c_str());
		return pcall(args);
	}

	void script_lua::reset()
	{
		TODO();
	}



	static int lua_func_adapter(lua_State *L)
	{
		lyramilk::data::string funcname;
		{
			lua_getglobal(L,"__global_engine_pointer");
			void* p = lua_touserdata(L,-1);
			lua_pop(L,1);
			script_lua* plua = (script_lua*)p;
			funcname = plua->callstack.top();
			plua->callstack.pop();
		}
		lua_getfield(L,1,"__lua_metainfo");
		void* pclass = lua_touserdata(L,-1);
		lyramilk::data::var** pthis = (lyramilk::data::var**)lua_touserdata(L,1);
		if(pclass == NULL) return 0;
		lua_pop(L,1);
		lua_remove(L,1);
		script_lua::metainfo* mi = (script_lua::metainfo*)pclass;

		lyramilk::data::var::array params = stov(L);
		lua_pop(L,lua_gettop(L));



		lyramilk::data::var::map env;
		env["this"].assign("this",(*pthis)->userdata("__lua_native_object"));
		lyramilk::data::var ret = mi->funcmap[funcname](params,env);
		luaset(L,ret);
		return 1;
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
		lua_getmetatable(L,1);
		lua_getfield(L,-1,name);
		if(LUA_TFUNCTION == lua_type(L,-1)){
			lua_getglobal(L,"__global_engine_pointer");
			void* p = lua_touserdata(L,-1);
			lua_pop(L,1);

			script_lua* plua = (script_lua*)p;
			plua->callstack.push(name);
		}
		return 1;
	}

	static int lua_func_meta_ctr(lua_State *L)
	{
		int argc = lua_gettop(L);
		if(argc != 1) return 0;
		lua_getfield(L,-1,"__lua_metainfo");
		void* cls = lua_touserdata(L,-1);
		if(cls == NULL) return 0;
		lua_pop(L,1);
		script_lua::metainfo* mi = (script_lua::metainfo*)cls;

		lyramilk::data::var::array r;
		int m=lua_gettop(L);
		for(int index=2;index<=m;++index){
			lyramilk::data::var v = luaget(L,index);
			r.push_back(v);
		}
		lua_pop(L,m-1);

		lyramilk::data::var** p = (lyramilk::data::var**)lua_newuserdata(L,sizeof(void*));
		*p = new lyramilk::data::var;
		(*p)->assign("__lua_native_object",mi->ctr(r));

		lua_insert(L,1);
		lua_setmetatable(L,-2);

		return 1;
	}

	static int lua_func_meta_dtr(lua_State *L)
	{
		int argc = lua_gettop(L);
		if(argc != 1) return 0;
		lua_getfield(L,-1,"__lua_metainfo");
		void* cls = lua_touserdata(L,-1);
		if(cls == NULL) return 0;
		lua_pop(L,1);
		script_lua::metainfo* mi = (script_lua::metainfo*)cls;
		//void** p = (void**)lua_touserdata(L,1);
		lyramilk::data::var** p = (lyramilk::data::var**)lua_touserdata(L,1);
		void* pdata = (void*)(*p)->userdata("__lua_native_object");
		mi->dtr(pdata);
		delete *p;
		return 0;
	}

	void script_lua::define(lyramilk::data::string classname,functional_map m,class_builder builder,class_destoryer destoryer)
	{
		luaL_newmetatable(L,classname.c_str());

		metainfo& mi = minfo[classname];
		mi.ctr = builder;
		mi.dtr = destoryer;
		lua_pushstring(L,"__lua_metainfo");
		lua_pushlightuserdata(L,&mi);
		lua_settable(L, -3);
		functional_map::iterator it = m.begin();
		for(;it!=m.end();++it){
			lua_pushstring(L,it->first.c_str());
			lua_pushcfunction(L,lua_func_adapter);
			mi.funcmap[it->first] = it->second;
			lua_settable(L, -3);
		}

		lua_pushstring(L, "__index");
		lua_pushcfunction(L, lua_func_meta_index);
		lua_settable(L, -3);

		lua_pushstring(L, "__gc");
		lua_pushcfunction(L, lua_func_meta_dtr);
		lua_settable(L, -3);

		lua_pushstring(L, "new");
		lua_pushcfunction(L, lua_func_meta_ctr);
		lua_settable(L, -3);

		lua_setglobal(L,classname.c_str());
	}
	
	lyramilk::data::var script_lua::createobject(lyramilk::data::string classname,lyramilk::data::var::array args)
	{
		std::map<lyramilk::data::string,metainfo>::iterator it = minfo.find(classname);
		if(it==minfo.end()) return lyramilk::data::var::nil;

		lua_getglobal(L,classname.c_str());
		lua_func_meta_ctr(L);
		const void* ptr = lua_touserdata(L,-1);
		/*
		lyramilk::data::stringstream ss;
		ss << classname << ptr << std::endl;
		*/

		lyramilk::data::var v("__script_native_object",ptr);
		v.userdata("__script_object_id",&it->first);
		lua_pop(L,1);
		return v;
	}

	void script_lua::gc()
	{
		lua_gc(L,LUA_GCCOLLECT,0);
	}

	void script_lua::clear()
	{
		int stacksize = lua_gettop(L);
		lua_pop(L,stacksize);
	}
}}}


/**
		lyramilk::data::var v("__script_object_id",(void*)jsretid);
		v.userdata("__script_native_object",pnewobj);

**/