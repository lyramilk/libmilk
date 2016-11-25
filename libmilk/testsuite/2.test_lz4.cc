#include <iostream>
#include <fstream>
#include <cassert>
#include "codes.h"
#include "testing.h"

#define SIZE 1000000



class coding_test:public lyramilk::data::coding
{
  public:
	virtual lyramilk::data::string decode(const lyramilk::data::string& str)
	{
		return str;
	}
	virtual lyramilk::data::string encode(const lyramilk::data::string& str)
	{
		char* rp = new char[str.size()];
		char* p = rp;
		char last;
		lyramilk::data::string::const_iterator it = str.begin();
		for(;it!=str.end();++it,++p){
			const char &c = *it;
			*p = c - last;
			last = c;
		}
		/*
		char last = 0;
		const char* psrc = str.c_str();
		const char* pend = psrc + str.size();
		char* p = new char[str.size()];
		while(psrc < pend){
			char c = *psrc;
			*p = c - last;
			last = c;
			++psrc,p++;
		}
		*/
		/*
		lyramilk::data::string str2;
		str2.reserve(str.size());
		char last = 0;
		for(int i = 0;i<str.size();++i){
			char c = str[i];
			str2.push_back(c - last);
			last = c;
		}
		return str2;
		*/
		return lyramilk::data::string(rp,str.size());
	}
	static coding* getter()
	{
		static coding_test _mm;
		return &_mm;
	}
};



void test_c(const lyramilk::data::string& str,lyramilk::data::string alg)
{
	lyramilk::debug::nsecdiff nd;
	long long d1,d2;

	nd.mark();
	lyramilk::data::string deststr = lyramilk::data::codes::instance()->encode(alg,str);
	d1 = nd.diff();
	nd.mark();
	lyramilk::data::string str2 = lyramilk::data::codes::instance()->decode(alg,deststr);
	d2 = nd.diff();

	std::cout << "变换算法" << alg << "，en=" << deststr.size() << "(" << d1 << "),de=" << str2.size() << "(" << d2 << ")" << std::endl;
	for(unsigned int i = 0;i<str.size() && i < str2.size();++i){
		if(str[i] != str2[i]){
			std::cout << "从" << i << "开始不一样" << (int)(str[i]&0xff) << "!=" << (int)(str2[i]&0xff) << "," << (int)(str2[i+1]&0xff) << std::endl;
			return;
		}
	}
	if(str.size() > str2.size()){
		std::cout << "长度不同，变短了" << std::endl;
	}else if(str.size() < str2.size()){
		std::cout << "长度不同，变长了" << std::endl;
	}
	/*
	std::cout << "  源" << str << std::endl;
	std::cout << "中间" << deststr << std::endl;
	std::cout << "目标" << str2 << std::endl;*/
}

int main(int argc,const char* argv[])
{
	//lyramilk::data::codes::instance()->define("test",coding_test::getter);
	//lyramilk::data::string str = "Hello World !!!ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss!";
	/*
	lyramilk::data::string str;
	str.reserve(SIZE);
	const char cz[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-=`~!@#$%^&*)_+";
	std::cout << "字符串表大小" << sizeof(cz) << std::endl;
	for(unsigned int i = 0;i < SIZE;++i){
		int e = rand();
		str.push_back(cz[e%sizeof(cz)]);
	}

	std::ofstream ofs("abc.txt",std::ofstream::binary|std::ofstream::out);
	ofs.write(str.c_str(),str.size());
	ofs.close();
*/
	lyramilk::data::string str;
	std::ifstream ofs("/root/11M_test.txt",std::ifstream::binary|std::ifstream::in);
	while(ofs){
		char buff[65536];
		ofs.read(buff,sizeof(buff));
		str.append(buff,ofs.gcount());
	}
	ofs.close();
	//str.erase(str.begin() + 65536,str.end());


	std::cout << lyramilk::data::codes::instance()->supports() << std::endl;
	std::cout << "数据长度" << str.size() << std::endl;
	test_c(str,"lz4");
	test_c(str,"url");
	test_c(str,"utf16");
	//test_c(str,"test");
	return 0;
}
