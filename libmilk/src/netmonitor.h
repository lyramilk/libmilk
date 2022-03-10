#ifndef _lyramilk_netio_netmonitor_h_
#define _lyramilk_netio_netmonitor_h_

#include "netaio.h"
#include "atom.h"
/**
	@namespace lyramilk::netio
*/

namespace lyramilk{namespace netio
{
	class _lyramilk_api_ aiomonitor
	{
	  protected:
		std::set<int> fds;
		lyramilk::threading::mutex_rw lock;
	  public:
		aiomonitor();
	  	virtual ~aiomonitor();

		bool add(int fd);
		bool remove(int fd);
		bool send(const lyramilk::data::string& msg);
		bool empty();
	};
}}

#endif
