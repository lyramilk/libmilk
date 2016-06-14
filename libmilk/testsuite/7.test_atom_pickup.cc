#include <iostream>
#include <fstream>
#include <unistd.h>
#include "thread.h"
#include "var.h"
#include "log.h"
#include <cassert>


int r = 100;
__thread int tid = 0;
int tindex = 0;

class wer
{
  public:
	int i;
	wer()
	{
		i = 0;
	}
	void test()
	{
		if(tid == 0){
			tid = __sync_fetch_and_add(&tindex,1);
		}
		++i;
		i += 9999;
		i -= 9999;
		int k = i;
		i = 0;
		usleep(1);
		i = k;

	}
};

class atomiclist:public lyramilk::threading::exclusive::list<wer>
{

	lyramilk::threading::mutex_os l;
  public:
	void test()
	{
		int i = 0;
		int sum = 0;
		list_type::iterator it = es.begin();
		for(;it!=es.end();++it,++i){
			printf("对象%d,i=%d,状态%d\n",i,it->t->i,it->l.test());
			sum += it->t->i;
		}
		printf("总调用次数%d\n",sum);
		assert(sum == 10000);
	}
  protected:
	virtual wer* underflow()
	{
		return new wer();
	}
};

atomiclist c;




class ssss:public lyramilk::threading::threads
{
	void qqqq(atomiclist::ptr e)
	{
		(*e).test();
	}


	int svc()
	{
		for(int i=0;i<100;++i){
			atomiclist::ptr p = c.get();
			qqqq(p);
		}
		__sync_sub_and_fetch(&r,1);
		return 0;
	}
};


int main(int argc,const char* argv[])
{
	{
		r = 100;
		ssss ss;
		ss.active(r);
		usleep(1000);
		while(r);
	}
	c.test();
	return 0;
}
