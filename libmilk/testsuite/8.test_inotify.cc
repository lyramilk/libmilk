#include <iostream>
#include <fstream>
#include <unistd.h>
#include "inotify.h"
#include <cassert>


class mywatcher:public lyramilk::data::inotify
{
};

int main(int argc,const char* argv[])
{
	mywatcher m;
	lyramilk::io::aiopoll aip;
	aip.add(&m);
	aip.active(4);

	m.add("/root/watch/abc1.txt");
	m.add("/root/watch/abc2.txt");
	m.add("/root/watch/abc3.txt");


	m.add("/root/watch");

	int i = 0;
	std::cin >> i;
	return 0;
}
