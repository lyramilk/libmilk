#ifndef _lyramilk_netio_netproxy_h_
#define _lyramilk_netio_netproxy_h_

#include "netaio.h"
#include "netio.h"
#include "aio.h"

/**
	@namespace lyramilk::netio
*/

namespace lyramilk{namespace netio
{

	class aioproxysession_supper : public lyramilk::netio::aiosession
	{
		bool inited;
		lyramilk::data::stringstream scache;
		aioproxysession_supper* endpoint;
		friend class aioproxysession;
		friend class aioproxysession_upstream;
	  public:
		aioproxysession_supper();
	  	virtual ~aioproxysession_supper();
		virtual bool init();
	  protected:
		virtual bool notify_in();
		virtual bool notify_out();
		virtual bool ondata(const char* cache, int size,int* size_used, std::ostream& os);
		virtual bool notify_hup();
		virtual bool notify_err();
		virtual bool notify_pri();
		virtual bool oninit(std::ostream& os) = 0;
		virtual bool ondownstream(const char* cache, int size,int* size_used, std::ostream& os) = 0;
	};


	/**
		@brief 异步代理会话
		@details 用作中间人转发上下流数据。
	*/
	class _lyramilk_api_ aioproxysession : public lyramilk::netio::aioproxysession_supper
	{
		friend class aioproxysession_upstream;
	  public:
		aioproxysession();
		virtual ~aioproxysession();
		virtual lyramilk::netio::netaddress get_upstream_address() = 0;
	  protected:
		virtual bool oninit(std::ostream& os);
		virtual bool ondata(const char* cache, int size,int* size_used, std::ostream& os);
		virtual bool onupstream(const char* cache, int size,int* size_used, std::ostream& os);
		virtual bool ondownstream(const char* cache, int size,int* size_used, std::ostream& os);
	};





	/**
		@brief 由 aioproxysession 支配
	*/
	class aioproxysession_upstream : public lyramilk::netio::aioproxysession_supper
	{
	  public:
		aioproxysession_upstream();
	  	virtual ~aioproxysession_upstream();
		virtual bool open(lyramilk::data::string host,lyramilk::data::uint16 port);
	  protected:
		virtual bool oninit(std::ostream& os);
		virtual bool ondata(const char* cache, int size,int* size_used, std::ostream& os);
		virtual bool ondownstream(const char* cache, int size,int* size_used, std::ostream& os);
	};
}}

#endif
