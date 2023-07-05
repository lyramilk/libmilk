#include <iostream>
#include <fstream>
#include <cassert>
#include "json.h"
#include "testing.h"
#include "ansi_3_64.h"
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
lyramilk::debug::nsecdiff nd;


int main(int argc,const char* argv[])
{
	/*
	std::string path = "../testsuite/json/";
	struct dirent* ent = NULL;
	DIR *pDir = opendir(path.c_str());
	while (NULL != (ent=readdir(pDir))){
		if(ent->d_type != 8) continue;
		printf("ent->d_name=%s\n",ent->d_name);
		lyramilk::data::string filestr;
		std::ifstream ifs((path + ent->d_name).c_str(),std::ifstream::binary);
		while(ifs){
			char buff[4096];
			ifs.read(buff,sizeof(buff));
			filestr.append(buff,ifs.gcount());
		}
		ifs.close();

		//printf("%d,%s\n",ent->d_type,ent->d_name);
		std::cout << "---------------测试" << ent->d_name << "---------------" << std::endl;
		std::cout << "测试文件：" << lyramilk::ansi_3_64::cyan << filestr << lyramilk::ansi_3_64::reset << std::endl;
		lyramilk::data::var v = lyramilk::data::json::parse(filestr);
		std::cout << "解析结果：" << lyramilk::ansi_3_64::yellow << lyramilk::data::json::stringify(v) << lyramilk::ansi_3_64::reset << std::endl;

	}*/

	{
		lyramilk::data::string filestr;
		nd.mark();
		//std::ifstream ifs("/data/rec/model/user_asset_fav.20221013/data.json",std::ifstream::binary);
		std::ifstream ifs("/home/lyramilk/static/data.json",std::ifstream::binary);
		while(ifs){
			char buff[4096];
			ifs.read(buff,sizeof(buff));
			filestr.append(buff,ifs.gcount());
		}
		long long uc = nd.diff();
		printf("加载耗时:%.3f 毫秒\n",double(uc)/1000000);
		/*
		nd.mark();
		lyramilk::data::var v = lyramilk::data::json::parse(filestr);
		uc = nd.diff();
		printf("解析耗时:%.3f 毫秒\n",double(uc)/1000000);
		*/
		nd.mark();
		lyramilk::data::var v2;
		lyramilk::data::json::parse(filestr,&v2);
		uc = nd.diff();
		printf("解析耗时:%.3f 毫秒\n",double(uc)/1000000);
	}
	return 0;
}
