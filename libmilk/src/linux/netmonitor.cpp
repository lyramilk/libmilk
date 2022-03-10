#include "netmonitor.h"
#include <sys/epoll.h>
#include <sys/poll.h>
#include <errno.h>
#include <string.h>
#include <vector>

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
		lyramilk::threading::mutex_sync _(lock.w());
		fds.insert(fd);
		return true;
	}

	bool aiomonitor::remove(int fd)
	{
		lyramilk::threading::mutex_sync _(lock.w());
		fds.erase(fd);
		return true;
	}

	bool aiomonitor::send(const lyramilk::data::string& msg)
	{
		std::vector<int> fds_erase;
		{
			lyramilk::threading::mutex_sync _(lock.r());
			std::set<int>::iterator it = fds.begin();
			while(it!=fds.end()){
				int i = ::send(*it,msg.c_str(),msg.size(),MSG_WAITALL);
				if(i < 1){
					fds_erase.push_back(*it);
					continue;
				}else{
					++it;
				}
			}
		}

		{
			lyramilk::threading::mutex_sync _(lock.w());
			std::vector<int>::iterator it = fds_erase.begin();
			for(;it!=fds_erase.end();++it){
				fds.erase(*it);
			}
		}

		return true;
	}

	bool aiomonitor::empty()
	{
		return fds.empty();
	}

}}
