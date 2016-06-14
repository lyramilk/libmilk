#include <iostream>
#include "thread.h"
#include "log.h"
#include <unistd.h>

int r = 10;


class ssss:public lyramilk::threading::threads
{
	int svc()
	{
		lyramilk::data::string ttt = "ttt";
		for(int i=0;i<100;++i){
			lyramilk::klog(ttt) << "百度营销大学以“让营销人都懂互联网营销”为使命";
			lyramilk::klog(ttt) << "，";
			lyramilk::klog(ttt) << "致力于推动中国营销人员互联网营销水平的不断提升！" << std::endl;
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
	return 0;
}
