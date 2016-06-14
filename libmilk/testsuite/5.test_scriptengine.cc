#include <iostream>
#include <fstream>
#include "lua.h"
#include "log.h"
#include "multilanguage.h"
#define D(x...) lyramilk::kdict(x)

#include "script_js.h"
#include "script_lua.h"

#define TESTLUA "/root/libmilk/testsuite/test.lua"
#define TESTJS "/root/libmilk/testsuite/test.js"

class os
{
  public:
	static void* ctr(lyramilk::data::var::array)
	{
		return new os();
	}
	static void dtr(void* p)
	{
		delete (os*)p;
	}
	lyramilk::data::var print(lyramilk::data::var::array params,lyramilk::data::var::map env)
	{
		MILK_CHECK_SCRIPT_ARGS(params,0,lyramilk::data::var::t_str);
		std::cout << params[0] << std::endl;
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


class number
{
	int i;
	lyramilk::log::logss log;
  public:
	static void* ctr(lyramilk::data::var::array)
	{
		return new number();
	}
	static void dtr(void* p)
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

	lyramilk::data::var add(lyramilk::data::var::array params,lyramilk::data::var::map env)
	{
		std::cout << "[add]this=" << this << "," << i << "," << params.size() << std::endl;
		MILK_CHECK_SCRIPT_ARGS_LOG(log,lyramilk::log::warning,__FUNCTION__,params,0,lyramilk::data::var::t_str);
		return i + (int)params[0];
	}

	lyramilk::data::var sub(lyramilk::data::var::array params,lyramilk::data::var::map env)
	{
		std::cout << "[sub]this=" << this << "," << i << "," << params << std::endl;
		MILK_CHECK_SCRIPT_ARGS_LOG(log,lyramilk::log::warning,__FUNCTION__,params,0,lyramilk::data::var::t_str);
		return i - (int)params[0];
	}

	lyramilk::data::var testmap(lyramilk::data::var::array params,lyramilk::data::var::map env)
	{
		std::cout << "[testmap]" << params << std::endl;
		lyramilk::data::var::map m;
		m["apple"] = "苹果";
		m["orange"] = 333;
		return m;
	}
};



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
	lua_engines engs_lua;
	js_engines engs_js;

	{
		lyramilk::script::engines::ptr eng = engs_lua.get();
		lyramilk::script::engines::ptr eng1 = engs_lua.get();
		lyramilk::script::engines::ptr eng2 = engs_lua.get();
		lyramilk::script::engines::ptr eng3 = engs_lua.get();
		lyramilk::script::engines::ptr eng4 = engs_lua.get();

		if(eng){
			std::cout << "取得了eng" << std::endl;
			eng->load_file(TESTLUA);

			lyramilk::data::var::array r;
			r.push_back("Hello World!!!!肚子");
			eng->pcall();
			

			std::cout << "调用script的test函数的结果：" << eng->call("test",r) << std::endl;
		}

		if(eng1){
			std::cout << "取得了eng1" << std::endl;
			eng1->load_file(TESTLUA);

			lyramilk::data::var::array r;
			r.push_back("Hello World!!!!肚子");
			eng1->pcall();
			

			std::cout << "调用script的test函数的结果：" << eng1->call("test",r) << std::endl;
		}

		if(eng2){
			std::cout << "取得了eng2" << std::endl;
			eng2->load_file(TESTLUA);

			lyramilk::data::var::array r;
			r.push_back("Hello World!!!!肚子");
			eng2->pcall();
			

			std::cout << "调用script的test函数的结果：" << eng2->call("test",r) << std::endl;
		}


		if(eng3){
			std::cout << "取得了eng3" << std::endl;
			eng3->load_file(TESTLUA);

			lyramilk::data::var::array r;
			r.push_back("Hello World!!!!肚子");
			eng3->pcall();
			

			std::cout << "调用script的test函数的结果：" << eng3->call("test",r) << std::endl;
		}


		if(eng4){
			std::cout << "取得了eng4" << std::endl;
			eng4->load_file(TESTLUA);

			lyramilk::data::var::array r;
			r.push_back("Hello World!!!!肚子");
			eng4->pcall();
			

			std::cout << "调用script的test函数的结果：" << eng4->call("test",r) << std::endl;
		}
	}
	{
		lyramilk::script::engines::ptr eng = engs_lua.get();
		lyramilk::script::engines::ptr eng1 = engs_lua.get();
		lyramilk::script::engines::ptr eng2 = engs_lua.get();
		lyramilk::script::engines::ptr eng3 = engs_js.get();
		lyramilk::script::engines::ptr eng4 = engs_js.get();

		if(eng){
			std::cout << "取得了eng" << std::endl;
			eng->load_file(TESTLUA);

			lyramilk::data::var::array r;
			r.push_back("Hello World!!!!肚子");
			eng->pcall();
			

			std::cout << "调用script的test函数的结果：" << eng->call("test",r) << std::endl;
		}

		if(eng1){
			std::cout << "取得了eng1" << std::endl;
			eng1->load_file(TESTLUA);

			lyramilk::data::var::array r;
			r.push_back("Hello World!!!!肚子");
			eng1->pcall();
			

			std::cout << "调用script的test函数的结果：" << eng1->call("test",r) << std::endl;
		}

		if(eng2){
			std::cout << "取得了eng2" << std::endl;
			eng2->load_file(TESTLUA);

			lyramilk::data::var::array r;
			r.push_back("Hello World!!!!肚子");
			eng2->pcall();
			

			std::cout << "调用script的test函数的结果：" << eng2->call("test",r) << std::endl;
		}


		if(eng3){
			std::cout << "取得了eng3" << std::endl;
			eng3->load_file(TESTJS);

			lyramilk::data::var::array r;
			r.push_back("Hello World!!!!肚子");
			eng3->pcall();
			

			std::cout << "调用script的test函数的结果：" << eng3->call("test",r) << std::endl;
		}

		if(eng4){
			std::cout << "取得了eng4" << std::endl;
			eng4->load_file(TESTJS);

			lyramilk::data::var::array r;
			r.push_back("Hello World!!!!肚子");
			eng4->pcall();
			

			std::cout << "调用script的test函数的结果：" << eng4->call("test",r) << std::endl;
		}
	}

	return 0;
}
