#include <iostream>
#include <fstream>
#include "log.h"
#include "multilanguage.h"
#define D(x...) lyramilk::kdict(x)

#include "script_js.h"

#define TESTJS "/root/libmilk/testsuite/test2.js"

class os
{
	int i;
  public:
	static void* ctr(lyramilk::data::var::array ar)
	{
		return new os(ar[0]);
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

	os(int k)
	{
std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++构造os " << k << "," << this << std::endl;
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
	static void* ctr(lyramilk::data::var::array ar)
	{
		return new number(ar[0]);
	}
	static void dtr(void* p)
	{
		delete (number*)p;
	}

	number(int k):log(lyramilk::klog,"app.lua.number")
	{
		i = k;
std::cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++构造number " << this << ",i=" << i << std::endl;
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








int main(int argc,const char* argv[])
{
	lyramilk::script::js::script_js engs_js;
	lyramilk::script::engine* eng = &engs_js;
	{
		lyramilk::script::engine::functional_map fn;
		fn["add"] = lyramilk::script::engine::functional<number,&number::add>;
		fn["sub"] = lyramilk::script::engine::functional<number,&number::sub>;
		fn["testmap"] = lyramilk::script::engine::functional<number,&number::testmap>;
		eng->define("niuniu",fn,number::ctr,number::dtr);
	}

	{
		lyramilk::script::engine::functional_map fn;
		fn["print"] = lyramilk::script::engine::functional<os,&os::print>;
		eng->define("os",fn,os::ctr,os::dtr);
	}
	{
		eng->load_file(TESTJS);
		eng->pcall();
		std::cout << "加载" << TESTJS << "完成" << std::endl;

		lyramilk::data::var::array r;
		r.push_back("Hello World!!!!肚子");
		lyramilk::data::var::array r2;
		r2.push_back(100000);
		r.push_back(eng->createobject("niuniu",r2));
		std::cout << "调用script的test函数的结果：" << eng->call("test",r).userdata() << std::endl;
	}
	return 0;
}
