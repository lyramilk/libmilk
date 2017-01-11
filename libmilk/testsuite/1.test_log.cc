#include <iostream>
#include "thread.h"
#include "testing.h"
#include "log.h"
#include <unistd.h>

int r = 100;

lyramilk::log::logss log(lyramilk::klog,"ttt");

class ssss:public lyramilk::threading::threads
{
	int svc()
	{
		for(int i=0;i<100;++i){
			log(lyramilk::log::warning,"svc") << "百度" << "营销" << "大学" << "以" << "“让营销人都懂互联网营销”" << "为" << "使命" << "，" << "致力于" << "推动" << "中国" << "营销人员" << "互联网" << "营销水平" << "的" << "不断提升" << "！" << std::endl;
		}
		__sync_sub_and_fetch(&r,1);
		return 0;
	}
};

int main(int argc,const char* argv[])
{
	lyramilk::data::string k = "打印1万条日志耗时 ";
	lyramilk::debug::nsecdiff td;
	lyramilk::debug::clocktester _d(td,log(lyramilk::log::debug),k);

	ssss ss;
	ss.active(r);

	while(r);
	printf("结束\n");
	return 0;
}
