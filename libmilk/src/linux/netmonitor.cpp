#include "netmonitor.h"
#include <sys/epoll.h>
#include <sys/poll.h>
#include <errno.h>
#include <string.h>

namespace lyramilk{namespace netio
{
	// aiomonitor
	aiomonitor::aiomonitor()
	{
	}

	aiomonitor::~aiomonitor()
	{
	}

	bool aiomonitor::add(int fd)
	{
		fds.insert(fd);
		return true;
	}

	bool aiomonitor::remove(int fd)
	{
		fds.erase(fd);
		return true;
	}

	bool aiomonitor::send(const lyramilk::data::string& msg)
	{
		std::set<int>::iterator it = fds.begin();
		while(it!=fds.end()){
			int i = ::send(*it,msg.c_str(),msg.size(),MSG_WAITALL);
			if(i < 1){
				it = fds.erase(it);
				continue;
			}else{
				++it;
			}
		}
		return true;
	}

	bool aiomonitor::empty()
	{
		return fds.empty();
	}

}}
