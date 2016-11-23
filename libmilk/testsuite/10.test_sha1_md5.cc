#include <iostream>
#include <fstream>
#include <cassert>
#include "sha1.h"
#include "md5.h"

int main(int argc,const char* argv[])
{
	lyramilk::cryptology::sha1 c1;
	lyramilk::cryptology::md5 c2;

	lyramilk::data::string src = "HelloWorld";
	COUT << "step.1 " << src << "的哈希：" << std::endl;
	c1 << src;
	std::cout << "SHA1=" << c1.get_key().str() << std::endl;
	std::cout << "SHA1=" << c1.get_key().str() << std::endl;
	c2 << src;
	std::cout << "16位MD5=" << c2.get_key().str16() << std::endl;
	std::cout << "32位MD5=" << c2.get_key().str32() << std::endl;


	COUT << "step.2 " << src << "!!!" << "的哈希：" << std::endl;
	c1 << "!!!";
	std::cout << "SHA1=" << c1.get_key().str() << std::endl;
	std::cout << "SHA1=" << c1.get_key().str() << std::endl;
	c2 << "!!!";
	std::cout << "16位MD5=" << c2.get_key().str16() << std::endl;
	std::cout << "32位MD5=" << c2.get_key().str32() << std::endl;
	return 0;
}
