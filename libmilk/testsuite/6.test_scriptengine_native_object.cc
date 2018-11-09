#include <iostream>
#include <fstream>
#include "log.h"
#include "dict.h"
#include "scriptengine.h"
#include "ansi_3_64.h"
#include "testing.h"
#include <jsapi.h>
#define D(x...) lyramilk::kdict(x)

class os:public lyramilk::script::sclass
{
	int i;
  public:

	static lyramilk::script::sclass* ctr(const lyramilk::data::var::array& ar)
	{
		return new os(ar[0]);
	}

	static void dtr(lyramilk::script::sclass* p)
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

class number:public lyramilk::script::sclass
{
	int i;
	lyramilk::log::logss log;
  public:
	static lyramilk::script::sclass* ctr(const lyramilk::data::var::array& ar)
	{
		return new number(ar[0]);
	}

	static void dtr(lyramilk::script::sclass* p)
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

class test_enum:public lyramilk::script::sclass
{
	int i;
  public:

	static lyramilk::script::sclass* ctr(const lyramilk::data::var::array& ar)
	{
		return new test_enum();
	}

	static void dtr(lyramilk::script::sclass* p)
	{
		delete (test_enum*)p;
	}

	lyramilk::data::var desc(const lyramilk::data::var::array& args,const lyramilk::data::var::map& env)
	{
printf("%s\n",__FUNCTION__);
		return "test_system";
	}


	lyramilk::data::var::array ar;

	test_enum()
	{
		ar.push_back("月光");
		ar.push_back("星辰");
		ar.push_back("烈日");
	}

	~test_enum()
	{
	}

	virtual int iterator_begin(lyramilk::data::var* v)
	{
		return 3;
	}

	virtual bool iterator_next(std::size_t idx,lyramilk::data::var* v)
	{
		if(idx >= ar.size()) return false;
		*v = ar[idx];
		return true;
	}

	virtual void iterator_end()
	{
	}

	lyramilk::data::var::map m;

	virtual bool set(const lyramilk::data::string& k,const lyramilk::data::var& v)
	{
		lyramilk::data::string prx = "prefix.";
		m[k] = prx + v.str();
		return true;
	}

	virtual bool get(const lyramilk::data::string& k,lyramilk::data::var* v)
	{
		*v = m[k] + ".surfix";
		return true;
	}

};

lyramilk::data::var echo(const lyramilk::data::var::array& args,const lyramilk::data::var::map& env)
{
	std::cout << lyramilk::ansi_3_64::green << args[0] << lyramilk::ansi_3_64::reset << std::endl;
	return true;
}

lyramilk::data::var add(const lyramilk::data::var::array& args,const lyramilk::data::var::map& env)
{
printf("%s\n",__FUNCTION__);
	MILK_CHECK_SCRIPT_ARGS(args,0,lyramilk::data::var::t_int);
	MILK_CHECK_SCRIPT_ARGS(args,1,lyramilk::data::var::t_int);
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

	{
		lyramilk::script::engine::functional_map fn;
		fn["desc"] = lyramilk::script::engine::functional<test_enum,&test_enum::desc>;
		eng->define("test_enum",fn,test_enum::ctr,test_enum::dtr);
	}


	eng->define("myecho",echo);
	eng->define("myadd",add);
	eng->define("myaddm",addm);
	eng->define_const("IsDebug",true);
	{
		eng->load_file(file);
		//std::cout << "加载" << file << "完成" << std::endl;

		lyramilk::data::var::array r;
		lyramilk::data::var::array r2;
		r2.push_back(100000);
		//r.push_back(eng->createobject("test_system",r2));
		//r.push_back(eng->createobject("test_number",r2));
		r.push_back(10000000000000011);
		r.push_back(10000000000000010);
		std::cout << "调用script的test函数的结果：" << eng->call("mytest",r) << std::endl;

		std::cout << (10000000000000011 + 10000000000000010) << std::endl;
		eng->gc();
	}
	eng->reset();
	delete eng;
}

int main(int argc,const char* argv[])
{
	printf("min=%llu,max=%llu\n",JSVAL_INT_MIN,JSVAL_INT_MAX);

	for(int i=0;i<1;++i){

		std::map<lyramilk::data::string,lyramilk::script::engine*> engs;
#ifdef JS17_FOUND
		engs["js"] = lyramilk::script::engine::createinstance("js");
#endif
#ifdef LUAJIT_FOUND
		engs["lua"] = lyramilk::script::engine::createinstance("lua");
#endif

		for(std::map<lyramilk::data::string,lyramilk::script::engine*>::iterator it = engs.begin();it!=engs.end();++it){
			std::cout << "*********************************************测试" << it->first << "*********************************************" << std::endl;
			lyramilk::data::string test = "/data/src/libmilk/testsuite/test." + it->first;
			test_script(test,it->second);
		}
	}
	return 0;
}
