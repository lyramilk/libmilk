#include "netaio.h"
#include "multilanguage.h"
#include "log.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

namespace lyramilk{namespace io
{
	// aioselector
	aioselector::aioselector()
	{
		container = NULL;
	}

	aioselector::~aioselector()
	{}

	// aiopoll
	bool aiopoll::transmessage()
	{
		const int ee_max = 1;
		epoll_event ees[ee_max];
		int ee_count = epoll_wait(epfd, ees, ee_max, -1);
		for(int i=0;i<ee_count;++i){
			epoll_event &ee = ees[i];
			onevent((aioselector*)ee.data.ptr,ee.events);
		}
		return true;
	}
	aiopoll::aiopoll()
	{
		epfd = epoll_create(pool_max);
	}

	aiopoll::~aiopoll()
	{
		if(epfd){
			::close(epfd);
		}
	}

	bool aiopoll::add(aioselector* r)
	{
		return add(r,EPOLLIN);
	}

	bool aiopoll::add(aioselector* r,uint32 mask)
	{
		assert(r);
		epoll_event ee;
		ee.data.ptr = r;
		ee.events = mask;
		r->container = this;
		if (epoll_ctl(epfd, EPOLL_CTL_ADD, r->getfd(), &ee) == -1) {
			lyramilk::klog(lyramilk::log::error,"lyramilk.aio.epoll.add") << lyramilk::kdict("向epoll中添加套接字%d时发生错误%s",r->getfd(),strerror(errno)) << std::endl;
			r->ondestory();
			return false;
		}
		return true;
	}

	bool aiopoll::reset(aioselector* r,uint32 mask)
	{
		assert(r);
		epoll_event ee;
		ee.data.ptr = r;
		ee.events = mask;

		if (epoll_ctl(epfd, EPOLL_CTL_MOD, r->getfd(), &ee) == -1) {
			lyramilk::klog(lyramilk::log::error,"lyramilk.aio.epoll.reset") << lyramilk::kdict("修改epoll中的套接字%d时发生错误%s",r->getfd(),strerror(errno)) << std::endl;
			return false;
		}
		return true;

	}

	bool aiopoll::remove(aioselector* r)
	{
		assert(r);
		epoll_event ee;
		ee.data.ptr = NULL;
		ee.events = 0;

		if (epoll_ctl(epfd, EPOLL_CTL_DEL, r->getfd(), &ee) == -1) {
			lyramilk::klog(lyramilk::log::error,"lyramilk.aio.epoll.remove") << lyramilk::kdict("从epoll中移除套接字%d时发生错误%s",r->getfd(),strerror(errno)) << std::endl;
		}
		r->ondestory();
		return true;
	}

	void aiopoll::onevent(aioselector* r,uint32 events)
	{
		assert(r);
		if(events & EPOLLHUP){
			if(!r->notify_hup()){
				remove(r);
			}
			return;
		}else if(events & EPOLLERR){
			if(!r->notify_err()){
				remove(r);
			}
			return;
		}else if(events & EPOLLPRI){
			if(!r->notify_pri()){
				remove(r);
			}
			return;
		}else if(events & EPOLLIN){
			if(!r->notify_in()){
				remove(r);
			}
			return;
		}else if(events & EPOLLOUT){
			if(!r->notify_out()){
				remove(r);
			}
			return;
		}
	}

	int aiopoll::svc()
	{
		while(transmessage());
		return 0;
	}

}}
