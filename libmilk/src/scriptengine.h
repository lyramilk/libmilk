#ifndef _lyramilk_script_engine_h_
#define _lyramilk_script_engine_h_

#include "var.h"
#include "thread.h"
#include "exception.h"

/**
	@namespace lyramilk::script
	@brief 该命名空间用来封装脚本，对每种脚本提供统一的定义形式。
*/

namespace lyramilk{namespace script
{
	/**
		@brief 脚本引擎接口
	*/
	class _lyramilk_api_ engine
	{
		lyramilk::data::var::map _mparams;
	  public:

		// 脚本向C++传递对象id时使用。
		static lyramilk::data::string s_user_objectid()
		{
			return "__script_object_id";
		}
		// 脚本向C++传递对象指针时使用。
		static lyramilk::data::string s_user_nativeobject()
		{
			return "__script_native_object";
		}
		// 脚本向C++传递函数id时使用。
		static lyramilk::data::string s_user_functionid()
		{
			return "__script_function_id";
		}

		// 环境变量：this指针。
		static lyramilk::data::string s_env_this()
		{
			return "__this";
		}
		// 环境变量：脚本引擎对象指针。
		static lyramilk::data::string s_env_engine()
		{
			return "__engine";
		}

		// 环境变量：脚本引擎对象指针的var用户变量名。
		static lyramilk::data::string s_user_engineptr()
		{
			return "__engine";
		}

		/**
			@brief 函数指针：适配脚本可访问的C++对象中的函数
		*/
		typedef lyramilk::data::var (*functional_type)(lyramilk::data::var::array argv,lyramilk::data::var::map env);

		/**
			@brief functional_type的map
		*/
		typedef std::map<lyramilk::data::string,functional_type,std::less<lyramilk::data::string>,lyramilk::data::allocator<lyramilk::data::string> > functional_map;

		/**
			@brief 函数指针：创建脚本可访问的C++对象
		*/
		typedef void* (*class_builder)(lyramilk::data::var::array);

		/**
			@brief 函数指针：销毁脚本可访问的C++对象
		*/
		typedef void (*class_destoryer)(void*);

		engine();
		virtual ~engine();

		virtual lyramilk::data::var& get(lyramilk::data::string k);
		virtual bool set(lyramilk::data::string k,lyramilk::data::var v);
		/**
			@brief 从一个字符串中加载脚本代码
			@param script 字符串形式的脚本代码
			@return 返回true表示成功
		*/
		virtual bool load_string(lyramilk::data::string script) = 0;

		/**
			@brief 从一个文件中加载脚本代码
			@param scriptfile 脚本文件路径
			@return 返回true表示成功
		*/
		virtual bool load_file(lyramilk::data::string scriptfile);

		/**
			@brief 执行脚本函数。
			@param func 脚本函数名
			@return 脚本的返回值
		*/
		virtual lyramilk::data::var call(lyramilk::data::var func);

		/**
			@brief 执行脚本函数。
			@param func 脚本函数名
			@param args 参数
			@return 脚本的返回值
		*/
		virtual lyramilk::data::var call(lyramilk::data::var func,lyramilk::data::var::array args) = 0;

		/**
			@brief 重置脚本引擎。
		*/
		virtual void reset() = 0;

		/**
			@brief 将一个脚本可访问的C++对象注入到脚本引擎中。
			@param classname 脚本中对象名字
			@param m 对象的成员函数表
			@param builder 该对象的创建函数
			@param destoryer 该对象的销毁函数
		*/
		virtual void define(lyramilk::data::string classname,functional_map m,class_builder builder,class_destoryer destoryer) = 0;

		/**
			@brief 将一个脚本可访问的C++函数注入到脚本引擎中。
			@param funcname 函数名
			@param func 脚本可访问的C++本地函数
		*/
		virtual void define(lyramilk::data::string funcname,functional_type func) = 0;

		/**
			@brief 将一个脚本可访问的C++对象装载到一个var对象中以作为参数由C++传递给脚本引擎。
			@param classname 脚本中对象名字
			@param args 参数
			@return 装载了脚本对象的var对象，该var对象只能传递给创建该对象的脚本引擎对象。
		*/
		virtual lyramilk::data::var createobject(lyramilk::data::string classname,lyramilk::data::var::array args) = 0;

		/**
			@brief 对于支持垃圾回收的脚本引擎，主动促使其进行垃圾回收。
		*/
		virtual void gc();

		/**
			@brief 该模板用于适配C++可访问的对象的成员函数到脚本引擎支持的形式。
			@details 该模板用于适配C++可访问的对象的成员函数到脚本引擎支持的形式。举例，如果number是一个C++类，而number的普通成员函数add函数符合functional_type形式，那么 lyramilk::script::engine::functional<number,&number::add>可以将该函数适配到非成员的functional_type形式。
		*/
		template <typename T,lyramilk::data::var (T::*Q)(lyramilk::data::var::array ,lyramilk::data::var::map )>
		static lyramilk::data::var functional(lyramilk::data::var::array params,lyramilk::data::var::map env)
		{
			T* pthis = (T*)env[engine::s_env_this()].userdata(engine::s_user_nativeobject());
			return (pthis->*(Q))(params,env);
		}
	};

	class _lyramilk_api_ engines : public lyramilk::threading::exclusive::list<engine>
	{
	  public:
		engines();
		virtual ~engines();

		virtual engine* underflow() = 0;
		virtual void onfire(engine* o);
		virtual void reset();
	};

	#define MILK_CHECK_SCRIPT_ARGS_LOG(log,lt,m,params,i,t)  {	\
		if(params.size() < i + 1){	\
			lyramilk::data::string str = D("参数太少");	\
			log(lt,m) << str << std::endl;	\
			throw lyramilk::exception(str);	\
		}	\
		lyramilk::data::var& v = params.at(i);	\
		if(!v.type_compat(t)){	\
			lyramilk::data::string str = D("参数%d类型不兼容:%s，期待%s",i+1,v.type_name().c_str(),lyramilk::data::var::type_name(t).c_str());	\
			log(lt,m) << str << std::endl;	\
			throw lyramilk::exception(str);	\
		}	\
	}

	#define MILK_CHECK_SCRIPT_ARGS(params,i,t)  {	\
		if(params.size() < i + 1){	\
			throw lyramilk::exception(D("参数太少"));	\
		}	\
		lyramilk::data::var& v = params.at(i);	\
		if(!v.type_compat(t)){	\
			throw lyramilk::exception(D("参数%d类型不兼容:%s，期待%s",i+1,v.type_name().c_str(),lyramilk::data::var::type_name(t).c_str()));	\
		}	\
	}



}}

#endif
