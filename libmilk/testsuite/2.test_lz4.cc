#include <iostream>
#include "codes.h"

int main(int argc,const char* argv[])
{
	lyramilk::data::string str = "Hello World !!!ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss!";
	lyramilk::data::string deststr = lyramilk::data::codes::instance()->encode(str,"lz4");
	lyramilk::data::string str2 = lyramilk::data::codes::instance()->decode(deststr,"lz4");
	std::cout << lyramilk::data::codes::instance()->supports() << std::endl;
	std::cout << deststr.size() << std::endl;
	std::cout << str2.size() << "," << str2 << std::endl;

	return 0;
}
