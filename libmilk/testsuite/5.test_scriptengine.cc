#include <iostream>
#include <fstream>
#ifdef LUAJIT_FOUND
	#include <lua.h>
#endif
#include "log.h"
#include "dict.h"
#define D(x...) lyramilk::kdict(x)

#include "script_js.h"
#include "script_lua.h"

#define TESTLUA "../testsuite/test.lua"
#define TESTJS "../testsuite/test.js"

class os:public lyramilk::script::sclass
{
  public:
	static lyramilk::script::sclass* ctr(const lyramilk::data::var::array& args)
	{
		return new os();
	}
	static void dtr(lyramilk::script::sclass* p)
	{
		delete (os*)p;
	}
	lyramilk::data::var print(const lyramilk::data::var::array& args,const lyramilk::data::var::map& env)
	{
		MILK_CHECK_SCRIPT_ARGS(args,0,lyramilk::data::var::t_str);
		std::cout << args[0] << std::endl;
		return true;
	}

	os()
	{
std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++构造os " << this << std::endl;
	}

	~os()
	{
std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++析构os " << this << std::endl;
	}
};


class number:public lyramilk::script::sclass
{
	int i;
	lyramilk::log::logss log;
  public:
	static lyramilk::script::sclass* ctr(const lyramilk::data::var::array& args)
	{
		return new number();
	}
	static void dtr(lyramilk::script::sclass* p)
	{
		delete (number*)p;
	}

	number():log(lyramilk::klog,"app.lua.number")
	{
		i = 1000000;
std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++构造number " << this << std::endl;
	}

	~number()
	{
std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++析构number " << this << std::endl;
	}

	lyramilk::data::var add(const lyramilk::data::var::array& args,const lyramilk::data::var::map& env)
	{
		std::cout << "[add]this=" << this << "," << i << "," << args.size() << std::endl;
		MILK_CHECK_SCRIPT_ARGS_LOG(log,lyramilk::log::warning,__FUNCTION__,args,0,lyramilk::data::var::t_str);
		return i + (int)args[0];
	}

	lyramilk::data::var sub(const lyramilk::data::var::array& args,const lyramilk::data::var::map& env)
	{
		std::cout << "[sub]this=" << this << "," << i << "," << args << std::endl;
		MILK_CHECK_SCRIPT_ARGS_LOG(log,lyramilk::log::warning,__FUNCTION__,args,0,lyramilk::data::var::t_str);
		return i - (int)args[0];
	}

	lyramilk::data::var testmap(const lyramilk::data::var::array& args,const lyramilk::data::var::map& env)
	{
		std::cout << "[testmap]" << args << std::endl;
		lyramilk::data::var::map m;
		m["apple"] = "苹果";
		m["orange"] = 333;
		return m;
	}
};



#ifdef LUAJIT_FOUND
class lua_engines:public lyramilk::script::engines
{
	virtual lyramilk::script::engine* underflow()
	{
		std::cout << "lua 触发了创建对象" << std::endl;
		lyramilk::script::engine* p = new lyramilk::script::lua::script_lua();
		{
			lyramilk::script::engine::functional_map fn;
			fn["add"] = lyramilk::script::engine::functional<number,&number::add>;
			fn["sub"] = lyramilk::script::engine::functional<number,&number::sub>;
			fn["testmap"] = lyramilk::script::engine::functional<number,&number::testmap>;
			p->define("niuniu",fn,number::ctr,number::dtr);
		}

		{
			lyramilk::script::engine::functional_map fn;
			fn["print"] = lyramilk::script::engine::functional<os,&os::print>;
			p->define("os",fn,os::ctr,os::dtr);
		}
		return p;
	}
};
#endif

class js_engines:public lyramilk::script::engines
{
	virtual lyramilk::script::engine* underflow()
	{
		std::cout << "js 触发了创建对象" << std::endl;
		lyramilk::script::engine* p = new lyramilk::script::js::script_js();
		{
			lyramilk::script::engine::functional_map fn;
			fn["add"] = lyramilk::script::engine::functional<number,&number::add>;
			fn["sub"] = lyramilk::script::engine::functional<number,&number::sub>;
			fn["testmap"] = lyramilk::script::engine::functional<number,&number::testmap>;
			p->define("niuniu",fn,number::ctr,number::dtr);
		}

		{
			lyramilk::script::engine::functional_map fn;
			fn["print"] = lyramilk::script::engine::functional<os,&os::print>;
			p->define("os",fn,os::ctr,os::dtr);
		}
		return p;
	}
};



int main(int argc,const char* argv[])
{
#ifdef LUAJIT_FOUND
	lua_engines engs_lua;
#endif
	js_engines engs_js;

#ifdef LUAJIT_FOUND
	{
		lyramilk::script::engines::ptr eng = engs_lua.get();
		lyramilk::script::engines::ptr eng1 = engs_lua.get();
		lyramilk::script::engines::ptr eng2 = engs_lua.get();
		lyramilk::script::engines::ptr eng3 = engs_lua.get();
		lyramilk::script::engines::ptr eng4 = engs_lua.get();

		if(eng){
			std::cout << "取得了eng" << std::endl;
			lyramilk::data::var::array r;
			r.push_back("Hello World!!!!肚子");
			eng->load_file(TESTLUA);
			std::cout << "调用script的test函数的结果：" << eng->call("test",r) << std::endl;
		}

		if(eng1){
			std::cout << "取得了eng1" << std::endl;

			lyramilk::data::var::array r;
			r.push_back("Hello World!!!!肚子");
			eng1->load_file(TESTLUA);

			std::cout << "调用script的test函数的结果：" << eng1->call("test",r) << std::endl;
		}

		if(eng2){
			std::cout << "取得了eng2" << std::endl;

			lyramilk::data::var::array r;
			r.push_back("Hello World!!!!肚子");
			eng2->load_file(TESTLUA);

			std::cout << "调用script的test函数的结果：" << eng2->call("test",r) << std::endl;
		}


		if(eng3){
			std::cout << "取得了eng3" << std::endl;

			lyramilk::data::var::array r;
			r.push_back("Hello World!!!!肚子");
			eng3->load_file(TESTLUA);

			std::cout << "调用script的test函数的结果：" << eng3->call("test",r) << std::endl;
		}


		if(eng4){
			std::cout << "取得了eng4" << std::endl;

			lyramilk::data::var::array r;
			r.push_back("Hello World!!!!肚子");
			eng4->load_file(TESTLUA);

			std::cout << "调用script的test函数的结果：" << eng4->call("test",r) << std::endl;
		}
	}
#endif
	{
#ifdef LUAJIT_FOUND
		lyramilk::script::engines::ptr eng = engs_lua.get();
		lyramilk::script::engines::ptr eng1 = engs_lua.get();
		lyramilk::script::engines::ptr eng2 = engs_lua.get();
#endif
		lyramilk::script::engines::ptr eng3 = engs_js.get();
		lyramilk::script::engines::ptr eng4 = engs_js.get();

#ifdef LUAJIT_FOUND
		if(eng){
			std::cout << "取得了eng" << std::endl;

			lyramilk::data::var::array r;
			r.push_back("Hello World!!!!肚子");
			eng->load_file(TESTLUA);

			std::cout << "调用script的test函数的结果：" << eng->call("test",r) << std::endl;
		}

		if(eng1){
			std::cout << "取得了eng1" << std::endl;

			lyramilk::data::var::array r;
			r.push_back("Hello World!!!!肚子");
			eng1->load_file(TESTLUA);

			std::cout << "调用script的test函数的结果：" << eng1->call("test",r) << std::endl;
		}

		if(eng2){
			std::cout << "取得了eng2" << std::endl;

			lyramilk::data::var::array r;
			r.push_back("Hello World!!!!肚子");
			eng2->load_file(TESTLUA);

			std::cout << "调用script的test函数的结果：" << eng2->call("test",r) << std::endl;
		}
#endif

		if(eng3){
			std::cout << "取得了eng3" << std::endl;

			lyramilk::data::var::array r;
			r.push_back("Hello World!!!!肚子");
			eng3->load_file(TESTJS);

			std::cout << "调用script的test函数的结果：" << eng3->call("test",r) << std::endl;
		}

		if(eng4){
			std::cout << "取得了eng4" << std::endl;

			lyramilk::data::var::array r;
			r.push_back("Hello World!!!!肚子");
			eng4->load_file(TESTJS);

			std::cout << "调用script的test函数的结果：" << eng4->call("test",r) << std::endl;
		}
	}

	return 0;
}
