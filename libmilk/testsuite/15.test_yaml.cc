#include "config.h"
#ifdef YAML_FOUND
#include <iostream>
#include <fstream>
#include <cassert>
#include "yaml1.h"
#include "json.h"
#include "testing.h"
#include "ansi_3_64.h"
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
lyramilk::debug::nsecdiff nd;



int main(int argc,const char* argv[])
{
	std::string path = "../testsuite/yaml/";
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
		lyramilk::data::var v = lyramilk::data::yaml::parse(filestr);
		std::cout << "解析结果1：" << lyramilk::ansi_3_64::yellow << lyramilk::data::json::stringify(v) << lyramilk::ansi_3_64::reset << std::endl;
		std::cout << "解析结果2：" << lyramilk::ansi_3_64::yellow << lyramilk::data::yaml::stringify(v) << lyramilk::ansi_3_64::reset << std::endl;
	}
	return 0;
}
#else
int main(int argc,const char* argv[])
{
	return 0;
}
#endif