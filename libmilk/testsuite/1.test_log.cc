#include <iostream>
#include "thread.h"
#include "log.h"
#include <unistd.h>

int r = 100;


class ssss:public lyramilk::threading::threads
{
	int svc()
	{
		lyramilk::log::logss log(lyramilk::klog,"ttt");
		for(int i=0;i<100;++i){
			log << "百度" << "营销" << "大学" << "以" << "“让营销人都懂互联网营销”" << "为" << "使命" << "，" << "致力于" << "推动" << "中国" << "营销人员" << "互联网" << "营销水平" << "的" << "不断提升" << "！" << std::endl;
		}
		__sync_sub_and_fetch(&r,1);
		return 0;
	}
};

int main(int argc,const char* argv[])
{
	ssss ss;
	ss.active(r);

	while(r);
	printf("结束\n");
	return 0;
}
