#include "netaio.h"
#include "dict.h"
#include "log.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <cassert>
#include <stdlib.h>
#include <testing.h>

namespace lyramilk{namespace io
{
	const static int pool_max = 10000000;

	// aioselector
	aioselector::aioselector()
	{
		pool = nullptr;
		mask = 0;
		thread_idx = -1;
		flag = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLONESHOT;
	}

	aioselector::~aioselector()
	{}

	bool aioselector::notify_attach(aiopoll_safe* container)
	{
		this->pool = container;
		return this->pool != nullptr;
	}

	void aioselector::notify_detach(aiopoll_safe* container)
	{
		assert(this->pool == container);
	}

	lyramilk::data::int64 aioselector::get_thread_idx()
	{
		return thread_idx;
	}

	bool aioselector::lock()
	{
		mlock.lock();
		if(this->pool){
			epoll_event ee;
			ee.data.ptr = this;
			ee.events = 0;
			if (epoll_ctl(this->pool->getfd(get_thread_idx()), EPOLL_CTL_DEL,getfd(), &ee) == -1) {
				mlock.unlock();
				return false;
			}
		}
		return true;
	}

	bool aioselector::unlock()
	{
		if(this->pool){
			epoll_event ee;
			ee.data.ptr = this;
			ee.events = mask;
			if (epoll_ctl(this->pool->getfd(get_thread_idx()), EPOLL_CTL_ADD,getfd(), &ee) == -1) {
				return false;
			}
		}
		mlock.unlock();
		return true;
	}


	//	aiopoll_safe
	aiopoll_safe::aiopoll_safe(std::size_t threadcount)
	{
		busy_thread_count = 0;
		seq_key = -1;
		pthread_key_create(&seq_key,nullptr);
		thread_idx = 0;
		fdcount = 0;

		if(threadcount > 0){
			epfds.resize(threadcount);
			for(std::size_t idx = 0;idx < epfds.size();++idx){
				epfds[idx].epfd = epoll_create(pool_max);
				epfds[idx].payload = 0;
			}
			//lyramilk::threading::threads::active(threadcount);
		}
	}


	aiopoll_safe::~aiopoll_safe()
	{
		for(std::size_t idx = 0;idx < epfds.size();++idx){
			::close(epfds[idx].epfd);
		}
		if((int)seq_key != -1){
			pthread_key_delete(seq_key);
		}
	}

	bool aiopoll_safe::add(aioselector* r,lyramilk::data::int64 mask)
	{
		return add_to_thread(-1,r,mask);
	}

	bool aiopoll_safe::reset(aioselector* r,lyramilk::data::int64 mask)
	{
		assert(r);
		epoll_event ee;
		ee.data.ptr = r;
		ee.events = mask;
		r->mask = mask;

		if (r->mlock.test() && epoll_ctl(getfd(), EPOLL_CTL_MOD, r->getfd(), &ee) == -1) {
			lyramilk::klog(lyramilk::log::error,"lyramilk.aio.epoll.reset") << lyramilk::kdict("修改epoll[%d]中的套接字%d时发生错误%s",getfd(),r->getfd(),strerror(errno)) << std::endl;
			return false;
		}
		//r->mask = mask;
		return true;
	}

	bool aiopoll_safe::remove(aioselector* r)
	{
		if(detach(r)){
			r->ondestory();
			return true;
		}
		return false;
	}


	bool aiopoll_safe::destory_by_fd(int fd)
	{
		// 这个操作可以导致主线程的epoll拿到一个事件，但是处理该事件的时候会出错，从而使得该fd对应的会话在epollwait线程中被安全销毁。
		::shutdown(fd,SHUT_WR);
		return true;
	}

	bool aiopoll_safe::detach(aioselector* r)
	{
		if(r->thread_idx < 0 || r->thread_idx >= (lyramilk::data::int64)epfds.size()){
			return false;
		}
		epinfo& epi = epfds[r->thread_idx];

		if(!r->mlock.test()){
			lyramilk::klog(lyramilk::log::error,"lyramilk.aio.epoll.remove") << lyramilk::kdict("从epoll[%d]中移除套接字%d时发生错误%s",getfd(),r->getfd(),"事件己上锁") << std::endl;
			return false;
		}

		epoll_event ee;
		ee.data.ptr = nullptr;
		ee.events = 0;

		if (epoll_ctl(epi.epfd, EPOLL_CTL_DEL, r->getfd(), &ee) == -1) {
			lyramilk::klog(lyramilk::log::error,"lyramilk.aio.epoll.remove") << lyramilk::kdict("从epoll[%d]中移除套接字%d时发生错误%s",getfd(),r->getfd(),strerror(errno)) << std::endl;
			return false;
		}

		r->notify_detach(this);
		__sync_sub_and_fetch(&fdcount,1);
		__sync_sub_and_fetch(&epi.payload,1);
		r->mask = 0;
		return true;
	}

	bool aiopoll_safe::add_to_thread(lyramilk::data::int64 supper_idx,aioselector* r,lyramilk::data::int64 mask)
	{
		if(mask == -1) mask = r->flag;
		if(supper_idx == -1){
			std::size_t min_idx = 0;
			std::size_t min_val = epfds[0].payload;
			for(std::size_t idx = 1;idx < epfds.size();++idx){
				if(epfds[idx].payload < min_val){
					min_val = epfds[idx].payload;
					min_idx = idx;
				}
			}
			supper_idx = min_idx;
		}

		epinfo& epi = epfds[supper_idx];

		assert(r);
		if(!r->mlock.test()){
			lyramilk::klog(lyramilk::log::error,"lyramilk.aio.aiopoll_safe.add") << lyramilk::kdict("向epoll[%d]中添加套接字%d时发生错误%s",epi.epfd,r->getfd(),"事件己上锁") << std::endl;
			return false;
		}

		epoll_event ee;
		ee.data.ptr = r;
		ee.events = mask;
		r->mask = mask;

		r->thread_idx = supper_idx;
		if(!r->notify_attach(this)){
			lyramilk::klog(lyramilk::log::error,"lyramilk.aio.aiopoll_safe.add") << lyramilk::kdict("向epoll[%d]中添加套接字%d时发生错误%s",epi.epfd,r->getfd(),"关联失败") << std::endl;
			return false;
		}
		if (epoll_ctl(epi.epfd, EPOLL_CTL_ADD, r->getfd(), &ee) == -1) {
			lyramilk::klog(lyramilk::log::error,"lyramilk.aio.aiopoll_safe.add") << lyramilk::kdict("向epoll[%d]中添加套接字%d时发生错误%s",epi.epfd,r->getfd(),strerror(errno)) << std::endl;
			return false;
		}
		__sync_add_and_fetch(&fdcount,1);
		__sync_add_and_fetch(&epi.payload,1);
		return true;
	}

	lyramilk::data::int64 aiopoll_safe::get_fd_count()
	{
		return fdcount;
	}

	native_epool_type aiopoll_safe::getfd(std::size_t thread_idx)
	{
		if(thread_idx < 0 || thread_idx >= epfds.size()){
			return -1;
		}
		return epfds[thread_idx].epfd;
	}

	native_epool_type aiopoll_safe::getfd()
	{
		lyramilk::data::int64 idx = get_thread_idx();
		if(idx == -1) return -1;
		return epfds[idx].epfd;
	}

	lyramilk::data::int64 aiopoll_safe::get_thread_idx()
	{
		std::size_t seq = (std::size_t)(void*)pthread_getspecific(seq_key);
		if(seq < 1 || seq > epfds.size()) return -1;
		return seq - 1;
	}

	bool aiopoll_safe::active(std::size_t threadcount)
	{
		if(epfds.size() > 0){
			return lyramilk::threading::threads::active(epfds.size());
		}
		return false;
	}

	bool aiopoll_safe::active()
	{
		if(epfds.size() > 0){
			return lyramilk::threading::threads::active(epfds.size());
		}
		return false;
	}

	void aiopoll_safe::onevent(aioselector* r,lyramilk::data::int64 events)
	{
		assert(r);
		if(events & EPOLLPRI){
			if(!r->notify_pri()){
				if(remove(r)) r->ondestory();
			}
			return;
		}else if(events & EPOLLIN){
			if(!r->notify_in()){
				if(remove(r)) r->ondestory();
			}
			return;
		}else if(events & EPOLLOUT){
			if(!r->notify_out()){
				if(remove(r)) r->ondestory();
			}
			return;
		}else if(events & (EPOLLHUP | EPOLLRDHUP)){
			if(!r->notify_hup()){
				if(remove(r)) r->ondestory();
			}
			return;
		}else if(events & EPOLLERR){
			r->notify_err();
			if(remove(r)) r->ondestory();
			return;
		}
	}


	int aiopoll_safe::svc()
	{
		std::size_t seq = ++thread_idx;
		pthread_setspecific(seq_key,(void*)seq);
		epinfo& epi = epfds[seq - 1];

		lyramilk::debug::nsecdiff nd;
		while(running){
			epoll_event ee;
			int ee_count = epoll_wait(epi.epfd, &ee,1, 2000);

			for(int i=0;i<ee_count;++i){
				aioselector* selector = (aioselector*)ee.data.ptr;
				onevent(selector,ee.events);
			}
		}
		return 0;
	}



	// aiopoll
	bool aiopoll::transmessage()
	{
		const int ee_max = 1;
		epoll_event ees[ee_max];
		int ee_count = epoll_wait(epfds[0].epfd, ees, ee_max,200);
		for(int i=0;i<ee_count;++i){
			epoll_event &ee = ees[i];
			aioselector* selector = (aioselector*)ee.data.ptr;
			if(selector){
				onevent(selector,ee.events);
			}
		}
		return true;
	}


	aiopoll::aiopoll():aiopoll_safe(1)
	{
	}

	aiopoll::~aiopoll()
	{
	}

	bool aiopoll::add(aioselector* r,lyramilk::data::int64 mask)
	{
		if(mask == -1) mask = r->flag;
		assert(r);
		if(!r->mlock.test()){
			lyramilk::klog(lyramilk::log::error,"lyramilk.aio.epoll.add") << lyramilk::kdict("向epoll[%d]中添加套接字%d时发生错误%s",epfds[0].epfd,r->getfd(),"事件己上锁") << std::endl;
			return false;
		}

		epoll_event ee;
		ee.data.ptr = r;
		ee.events = mask;
		r->mask = mask;

		if(!r->notify_attach(this)){
			lyramilk::klog(lyramilk::log::error,"lyramilk.aio.epoll.add") << lyramilk::kdict("向epoll[%d]中添加套接字%d时发生错误%s",epfds[0].epfd,r->getfd(),"关联失败") << std::endl;
			return false;
		}
		if (epoll_ctl(epfds[0].epfd, EPOLL_CTL_ADD, r->getfd(), &ee) == -1) {
			lyramilk::klog(lyramilk::log::error,"lyramilk.aio.epoll.add") << lyramilk::kdict("向epoll[%d]中添加套接字%d时发生错误%s",epfds[0].epfd,r->getfd(),strerror(errno)) << std::endl;
			return false;
		}
		__sync_add_and_fetch(&fdcount,1);
		//r->mask = mask;	有bug。对象刚被add到池中就可能己经被删掉除了。
		return true;
	}

	bool aiopoll::reset(aioselector* r,lyramilk::data::int64 mask)
	{
		assert(r);
		epoll_event ee;
		ee.data.ptr = r;
		ee.events = mask;
		r->mask = mask;

		if (r->mlock.test() && epoll_ctl(epfds[0].epfd, EPOLL_CTL_MOD, r->getfd(), &ee) == -1) {
			lyramilk::klog(lyramilk::log::error,"lyramilk.aio.epoll.reset") << lyramilk::kdict("修改epoll[%d]中的套接字%d时发生错误%s",epfds[0].epfd,r->getfd(),strerror(errno)) << std::endl;
			return false;
		}
		//r->mask = mask;
		return true;
	}

	bool aiopoll::remove(aioselector* r)
	{
		if(detach(r)){
			return true;
		}
		return false;
	}

	bool aiopoll::detach(aioselector* r)
	{
		assert(r);

		if(!r->mlock.test()){
			lyramilk::klog(lyramilk::log::error,"lyramilk.aio.epoll.remove") << lyramilk::kdict("从epoll[%d]中移除套接字%d时发生错误%s",epfds[0].epfd,r->getfd(),"事件己上锁") << std::endl;
			return false;
		}

		epoll_event ee;
		ee.data.ptr = nullptr;
		ee.events = 0;

		if (epoll_ctl(epfds[0].epfd, EPOLL_CTL_DEL, r->getfd(), &ee) == -1) {
			lyramilk::klog(lyramilk::log::error,"lyramilk.aio.epoll.remove") << lyramilk::kdict("从epoll[%d]中移除套接字%d时发生错误%s",epfds[0].epfd,r->getfd(),strerror(errno)) << std::endl;
			return false;
		}

		r->notify_detach(this);
		__sync_sub_and_fetch(&fdcount,1);
		r->mask = 0;
		return true;
	}


	bool aiopoll::active(std::size_t threadcount)
	{
		if(threadcount >= 0){
			return lyramilk::threading::threads::active(threadcount);
		}
		return false;
	}

	int aiopoll::svc()
	{
		while(running && transmessage());
		return 0;
	}

	lyramilk::data::int64 aiopoll::get_fd_count()
	{
		return fdcount;
	}


}}
