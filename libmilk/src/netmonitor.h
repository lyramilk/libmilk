#ifndef _lyramilk_netio_netmonitor_h_
#define _lyramilk_netio_netmonitor_h_

#include "netaio.h"
#include "atom.h"
/**
	@namespace lyramilk::netio
*/

namespace lyramilk{namespace netio
{
	class _lyramilk_api_ aiomonitor : public lyramilk::threading::threads
	{
	  protected:
		int aiofd;
		lyramilk::data::int64 fdcount;
		std::set<int> fds;
	  public:
		aiomonitor();
	  	virtual ~aiomonitor();

		bool add(int fd);
		bool remove(int fd);
		bool send(const lyramilk::data::string& msg);
		bool empty();
	  protected:
		int svc();
		lyramilk::threading::mutex_rw fds_lock;
		lyramilk::threading::mutex_spin data_lock;
	};
}}

#endif
