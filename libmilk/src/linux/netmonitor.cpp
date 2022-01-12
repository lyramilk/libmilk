#include "netmonitor.h"
#include <sys/epoll.h>
#include <sys/poll.h>
#include <errno.h>
#include <string.h>

namespace lyramilk{namespace netio
{
	aiomonitor::aiomonitor()
	{
		aiofd = epoll_create(100000);
	}

	aiomonitor::~aiomonitor()
	{
		::close(aiofd); 
	}

	bool aiomonitor::add(int fd)
	{
		if(size() == 0) lyramilk::threading::threads::active(1);

		epoll_event ee;
		ee.data.fd = fd;
		ee.events = EPOLLERR | EPOLLHUP | EPOLLRDHUP ;
		if(epoll_ctl(aiofd, EPOLL_CTL_ADD, fd, &ee) == -1){
			return false;
		}
		{
			lyramilk::threading::mutex_sync _(fds_lock.w());
			fds.insert(fd); 
		}
		__sync_add_and_fetch(&fdcount,1);
		return true;
	}

	bool aiomonitor::remove(int fd)
	{
		{
			lyramilk::threading::mutex_sync _(fds_lock.w());
			std::set<int>::iterator it = fds.find(fd);
			if(it!=fds.end()){
				fds.erase(it);
			}
		}
		epoll_event ee;
		ee.data.fd = fd;
		ee.events = 0;
		if(epoll_ctl(aiofd, EPOLL_CTL_DEL, fd, &ee) == -1){
			return false;
		}
		__sync_sub_and_fetch(&fdcount,1);
		return true;
	}




	bool inline check_write(int fd,lyramilk::data::uint32 msec)
	{
		pollfd pfd;
		pfd.fd = fd;
		pfd.events = POLLOUT;
		pfd.revents = 0;
		int ret = ::poll(&pfd,1,msec);
		if(ret > 0){
			if(pfd.revents & (POLLHUP | POLLRDHUP | POLLERR)){
				return false;
			}
			if(pfd.revents & POLLOUT){
				return true;
			}
		}
		return false;
	}


	bool aiomonitor::send(const lyramilk::data::string& msg)
	{
		std::vector<int> closefd;
		{
			lyramilk::threading::mutex_sync _(fds_lock.r());
			std::set<int>::iterator it = fds.begin();
			for(;it!=fds.end();++it){
				int fd = *it;
label_send:
				int r = ::send(fd,msg.c_str(),msg.size(),MSG_WAITALL);
				if(r == 0){
					closefd.push_back(fd);
				}else if(r == -1 && errno == EAGAIN){
					if(check_write(fd,10000)){
						goto label_send;
					}else{
						closefd.push_back(fd);
					}
				}else if(r == -1){
					closefd.push_back(fd);
				}
			}
		}

		{
			lyramilk::threading::mutex_sync _(fds_lock.w());
			for(std::vector<int>::iterator it = closefd.begin();it!=closefd.end();++it){
				::close(*it);
				fds.erase(*it);
			}
			__sync_sub_and_fetch(&fdcount,closefd.size());
		}
		return true;
	}

	bool aiomonitor::empty()
	{
		return fdcount == 0;
	}

	int aiomonitor::svc()
	{
		while(running){
			const int ee_max = 1;
			epoll_event ees[ee_max];
			int ee_count = epoll_wait(aiofd, ees, ee_max,2000);
			for(int i=0;i<ee_count;++i){
				epoll_event &ee = ees[i];
				if(ee.events & (EPOLLHUP | EPOLLRDHUP)){
					remove(ee.data.fd);
				}else if(ee.events & EPOLLERR){
					remove(ee.data.fd);
				}
			}
		}
		return 0;
	}
}}
