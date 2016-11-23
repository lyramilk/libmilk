#include <iostream>
#include <fstream>
#include <cassert>
#include "json.h"
#include "testing.h"

int main(int argc,const char* argv[])
{
	const unsigned char* pbin = (const unsigned char*)"字节数组值";

	lyramilk::data::var tmpv;
	tmpv.path("/数组1/lol") = "lol.qq.com";
	tmpv.path("/数组1/baidu") = "www.baidu.com";
	lyramilk::data::var tmpv2;
	tmpv2.path("/数组2/lol") = "lol.qq.com";
	tmpv2.path("/数组2/baidu") = "www.baidu.com";
	lyramilk::data::var v;
	v.path("/网址/baidu") = "www.baidu.com";
	v.path("/网址/bing") = "www.bing.com";
	v.path("/带引号的字符串键") = "\"lol.qq.com\"";
	v.path("/整数") = 1142;
	v.path("/浮点数") = 458.599999;
	v.path("/布尔") = false;
	v.path("/字节数组") = pbin;
	v.path("/字节数组").type(lyramilk::data::var::t_bin);
	v.path("/无效的数值") = lyramilk::data::var::nil;
	v.path("/用户数据").assign("userdata",nullptr);
	v.path("/嵌套数组/0/") = tmpv;
	v.path("/嵌套数组/1/") = tmpv2;
	v.path("/嵌套数组/2/") = v;
	std::cout << "准备好的var=" << v << std::endl;

	lyramilk::data::stringstream ss;
	ss << lyramilk::data::json(v) << std::endl;

	lyramilk::data::stringstream ss2(ss.str());
	lyramilk::data::var v2;
	{
		lyramilk::data::json j(v2);
		ss2 >> j;
	}
	std::cout << "解析的var2=" << v2 << std::endl;

	
	lyramilk::data::stringstream ss3;
	ss3 << lyramilk::data::json(v2) << std::endl;

	std::cout << "str1=" << ss.str() << std::endl;
	std::cout << "str3=" << ss3.str() << std::endl;

	std::cout << "转换得到的json   " << lyramilk::data::json(v2) << std::endl;
	return 0;
}
