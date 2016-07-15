#include <iostream>
#include <fstream>
#include <cassert>
#include "json.h"
#include "testing.h"

int main(int argc,const char* argv[])
{
	lyramilk::data::var tmpv;
	tmpv.path("/ats/lol") = "lol.qq.com";
	tmpv.path("/ats/lol") = "lol.qq.com";
	tmpv.path("/ats/baidu") = "lol.qq.com";
	lyramilk::data::var tmpv2;
	tmpv2.path("/ats2/lol") = "lol.qq.com";
	tmpv2.path("/ats2/lol") = "lol.qq.com";
	tmpv2.path("/ats2/baidu") = "lol.qq.com";
	lyramilk::data::var v;
	v.path("/game/lol") = "lol.qq.com";
	v.path("/game/lol") = "lol.qq.com";
	v.path("/search/baidu") = "lol.qq.com";
	v.path("/search/bing") = "lol.qq.com";
	v.path("/number") = 1142;
	v.path("/float") = 458.599999;
	v.path("/boolean") = false;
	v.path("/urz/0/") = tmpv;
	v.path("/urz/1/") = tmpv2;
	std::cout << "准备好的var=" << v << std::endl;

	lyramilk::data::stringstream ss;
	ss << lyramilk::data::json(v) << std::endl;

	lyramilk::data::stringstream ss2(ss.str());
	lyramilk::data::var v2;
	{
		lyramilk::data::json j(v2);
		ss2 >> j;
	}

	
	lyramilk::data::stringstream ss3;
	ss3 << lyramilk::data::json(v2) << std::endl;
	assert(ss.str() == ss3.str());

	std::cout << "转换得到的json" << lyramilk::data::json(v2) << std::endl;

	return 0;
}
