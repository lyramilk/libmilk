#include <iostream>
#include <fstream>
#include "log.h"
#include "multilanguage.h"
#include "scriptengine.h"
#include "ansi_3_64.h"
#include "testing.h"
#define D(x...) lyramilk::kdict(x)

class os
{
	int i;
  public:

	static void* ctr(const lyramilk::data::var::array& ar)
	{
		return new os(ar[0]);
	}

	static void dtr(void* p)
	{
		delete (os*)p;
	}

	lyramilk::data::var print(const lyramilk::data::var::array& args,const lyramilk::data::var::map& env)
	{
printf("%s\n",__FUNCTION__);
		MILK_CHECK_SCRIPT_ARGS(args,0,lyramilk::data::var::t_str);
		std::cout << lyramilk::ansi_3_64::green << args[0] << lyramilk::ansi_3_64::reset << std::endl;
		return true;
	}
	lyramilk::data::var desc(const lyramilk::data::var::array& args,const lyramilk::data::var::map& env)
	{
printf("%s\n",__FUNCTION__);
		return "test_system";
	}

	os(int k)
	{
		std::cout << lyramilk::ansi_3_64::green << "+++++++++++++++++++++++++++++++++++++++ 构造os " << this << ",i=" << k << lyramilk::ansi_3_64::reset << std::endl;
	}

	~os()
	{
		std::cout << lyramilk::ansi_3_64::green << "+++++++++++++++++++++++++++++++++++++++ 析构os " << this << lyramilk::ansi_3_64::reset << std::endl;
	}
};

class number
{
	int i;
	lyramilk::log::logss log;
  public:
	static void* ctr(const lyramilk::data::var::array& ar)
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
		std::cout << lyramilk::ansi_3_64::green << "+++++++++++++++++++++++++++++++++++++++ 构造number " << this << ",i=" << k << lyramilk::ansi_3_64::reset << std::endl;
	}

	~number()
	{
		std::cout << lyramilk::ansi_3_64::green << "+++++++++++++++++++++++++++++++++++++++ 析构number " << this << lyramilk::ansi_3_64::reset << std::endl;
	}

	lyramilk::data::var add(const lyramilk::data::var::array& args,const lyramilk::data::var::map& env)
	{
printf("%s\n",__FUNCTION__);
		std::cout << lyramilk::ansi_3_64::green << "[add]this=" << this << "," << i << "," << args.size() << lyramilk::ansi_3_64::reset << std::endl;
		MILK_CHECK_SCRIPT_ARGS_LOG(log,lyramilk::log::warning,__FUNCTION__,args,0,lyramilk::data::var::t_str);
		return i + (int)args[0];
	}

	lyramilk::data::var sub(const lyramilk::data::var::array& args,const lyramilk::data::var::map& env)
	{
printf("%s\n",__FUNCTION__);
		std::cout << lyramilk::ansi_3_64::green << "[sub]this=" << this << "," << i << "," << args << lyramilk::ansi_3_64::reset << std::endl;
		MILK_CHECK_SCRIPT_ARGS_LOG(log,lyramilk::log::warning,__FUNCTION__,args,0,lyramilk::data::var::t_str);
		return i - (int)args[0];
	}

	lyramilk::data::var testmap(const lyramilk::data::var::array& args,const lyramilk::data::var::map& env)
	{
printf("%s\n",__FUNCTION__);
		std::cout << lyramilk::ansi_3_64::green << "[testmap]" << args << lyramilk::ansi_3_64::reset << std::endl;
		lyramilk::data::var::map m;
		m["apple"] = "苹果";
		m["orange"] = 333;
		return m;
	}
	lyramilk::data::var desc(const lyramilk::data::var::array& args,const lyramilk::data::var::map& env)
	{
printf("%s\n",__FUNCTION__);
		return "test_number";
	}
};

lyramilk::data::var echo(const lyramilk::data::var::array& args,const lyramilk::data::var::map& env)
{
printf("%s\n",__FUNCTION__);
	MILK_CHECK_SCRIPT_ARGS(args,0,lyramilk::data::var::t_str);
	std::cout << lyramilk::ansi_3_64::green << args[0] << lyramilk::ansi_3_64::reset << std::endl;

	lyramilk::data::string str = args[0];
COUT << "str.size=" << str.size() << std::endl;

	return true;
}

lyramilk::data::var add(const lyramilk::data::var::array& args,const lyramilk::data::var::map& env)
{
printf("%s\n",__FUNCTION__);
	MILK_CHECK_SCRIPT_ARGS(args,0,lyramilk::data::var::t_int32);
	MILK_CHECK_SCRIPT_ARGS(args,1,lyramilk::data::var::t_int32);
	return (int)args[0] + (int)args[1];
}

lyramilk::data::var addm(const lyramilk::data::var::array& args,const lyramilk::data::var::map& env)
{
printf("%s\n",__FUNCTION__);
	MILK_CHECK_SCRIPT_ARGS(args,0,lyramilk::data::var::t_map);
	return (int)args[0]["k"] + (int)args[0]["v"];
}

void test_script(lyramilk::data::string file,lyramilk::script::engine* eng)
{
	{
		lyramilk::script::engine::functional_map fn;
		fn["add"] = lyramilk::script::engine::functional<number,&number::add>;
		fn["sub"] = lyramilk::script::engine::functional<number,&number::sub>;
		fn["testmap"] = lyramilk::script::engine::functional<number,&number::testmap>;
		fn["desc"] = lyramilk::script::engine::functional<number,&number::desc>;
		eng->define("test_number",fn,number::ctr,number::dtr);
	}

	{
		lyramilk::script::engine::functional_map fn;
		fn["print"] = lyramilk::script::engine::functional<os,&os::print>;
		fn["desc"] = lyramilk::script::engine::functional<os,&os::desc>;
		eng->define("test_system",fn,os::ctr,os::dtr);
	}
	eng->define("myecho",echo);
	eng->define("myadd",add);
	eng->define("myaddm",addm);
	{
		eng->load_file(file);
		//std::cout << "加载" << file << "完成" << std::endl;

		lyramilk::data::var::array r;
		lyramilk::data::var::array r2;
		r2.push_back(100000);
		r.push_back(eng->createobject("test_system",r2));
		r.push_back(eng->createobject("test_number",r2));
		std::cout << "调用script的test函数的结果：" << eng->call("mytest",r) << std::endl;
		eng->gc();
	}
	eng->reset();
	delete eng;
}

int main(int argc,const char* argv[])
{

	for(int i=0;i<1;++i){

	std::map<lyramilk::data::string,lyramilk::script::engine*> engs;
#ifdef JS17_FOUND
	engs["js"] = lyramilk::script::engine::createinstance("js");
#endif
#ifdef LUA_FOUND
	engs["lua"] = lyramilk::script::engine::createinstance("lua");
#endif

	for(std::map<lyramilk::data::string,lyramilk::script::engine*>::iterator it = engs.begin();it!=engs.end();++it){
		std::cout << "*********************************************测试" << it->first << "*********************************************" << std::endl;
		lyramilk::data::string test = "/root/libmilk/testsuite/test." + it->first;
		test_script(test,it->second);
	}
	}
	return 0;
}
