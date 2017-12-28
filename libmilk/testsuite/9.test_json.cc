#include <iostream>
#include <fstream>
#include <cassert>
#include "json.h"
#include "testing.h"

lyramilk::debug::nsecdiff nd;


int main(int argc,const char* argv[])
{
	std::vector<lyramilk::data::string> jsons;
	jsons.push_back("[.3,3,0.3,3e5,3.3e5]");
	jsons.push_back("{\"a1\":\"a\",\"a2\":[\"a\",\"b\",\"c\"]}");
	jsons.push_back("{\"嵌套数组\":[{\"数组1\":{\"baidu\":\"www.baidu.com\",\"lol\":\"lol.qq.com\"}},{\"数组2\":{\"baidu\":\"www.baidu.com\",\"lol\":\"lol.qq.com\"}},{\"嵌套数组\":[{\"数组1\":{\"baidu\":\"www.baidu.com\",\"lol\":\"lol.qq.com\"}},{\"数组2\":{\"baidu\":\"www.baidu.com\",\"lol\":\"lol.qq.com\"}}],\"带引号的字符串键\":\"\\\"lol.qq.com\\\"\",\"带emoji表情\":\"👑\",\"网址\":{\"bing\":\"www.bing.com\",\"baidu\":\"www.baidu.com\"},\"布尔\":false,\"整数\":1142,\"多行字符串\":\"第一行\\n第二行\",\"浮点数\":458.599999}],\"带引号的字符串键\":\"\\\"lol.qq.com\\\"\",\"带emoji表情\":\"👑\",\"网址\":{\"bing\":\"www.bing.com\",\"baidu\":\"www.baidu.com\"},\"布尔\":false,\"整数\":1142,\"多行字符串\":\"第一行\\n第二行\",\"浮点数\":458.599999}");
	jsons.push_back("{\r\n\t\"a1\":\"a\",\r\n\t\"a2\":[\n\t\t\"a\",\r\n\t\t\"b\",\r\n\t\t\"c\"\r\n\t]\r\n}");
	jsons.push_back("[\"\\ud83d\\udc94\",\"\\ud83d\\udc94\",\"\\ud83d\\udc94\"]");
	jsons.push_back("\"\\ud83d\\udc94\"");
	jsons.push_back("\"\\ud83d\\udc9\"");
	jsons.push_back("\"\\ud83d\\udc94");
	jsons.push_back("\"你好");
	jsons.push_back("{\"你好\"");
	jsons.push_back("\"\\x61\"");
	jsons.push_back("\"\\x61");
	jsons.push_back("\"\\x6");
	jsons.push_back("\"\\x");
	jsons.push_back("\"\\");
	jsons.push_back("\"");
	jsons.push_back("\"\\141\"");
	jsons.push_back("\"\\\\141\"");
	jsons.push_back("\"\\141");
	jsons.push_back("\"\\14\"");
	jsons.push_back("\"\\1\"");
	jsons.push_back("\"\\14");

	std::cout << lyramilk::data::json::unescape("\u5317\u4eac\\ud83d\\udc94 \\ud83d\\udc94 \\ud83d\\udc94 \\141") << std::endl;
	std::cout << lyramilk::data::json::unescape("\u5317\u4eac\\ud83d\\udc94 \\\\ud83d \\ud83d\\udc94 \\141") << std::endl;

	{
		std::vector<lyramilk::data::string>::const_iterator it = jsons.begin();
		for(;it!=jsons.end();++it){
			lyramilk::data::var v;
			lyramilk::data::json::parse(*it,&v);
			std::cout << "-------------------------\n测试json:" << *it << "\n\x1b[33m结果:" << v << "\x1b[0m" << std::endl;

			lyramilk::data::string jsonstr;
			lyramilk::data::json::stringify(v,&jsonstr);
			lyramilk::data::var v2;
			lyramilk::data::json::parse(jsonstr,&v2);
			lyramilk::data::string jsonstr2;
			lyramilk::data::json::stringify(v2,&jsonstr2);

			if(v == v2){
				std::cout << "序列化/串行化验证成功" << std::endl;
			}else{
				std::cout << "序列化/串行化验证失败" << std::endl;
				std::cout << "目标1:" << jsonstr << std::endl;
				std::cout << "目标2:" << jsonstr2 << std::endl;
				std::cout << "目标1:" << v.str() << std::endl;
				std::cout << "目标2:" << v2.str() << std::endl;
			}

		}
	}
	return 0;
}
