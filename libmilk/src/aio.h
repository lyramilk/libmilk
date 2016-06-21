#ifndef _lyramilk_io_aio_h_
#define _lyramilk_io_aio_h_

#include "thread.h"

namespace lyramilk{namespace io
{
	typedef int native_epool_type;
	typedef int native_filedescriptor_type;
	typedef unsigned int uint32;

	/**
		@brief 异步容器的选择子
	*/
	class _lyramilk_api_ aiopoll;
	class _lyramilk_api_ aioselector
	{
	  protected:
		friend class aiopoll;
		aiopoll* container;
		virtual void ondestory() = 0;
	  protected:
		/**
			@brief 文件可读时发生该通知。
		*/
		virtual bool notify_in () = 0;
		/**
			@brief 文件可写时发生该通知。
		*/
		virtual bool notify_out() = 0;
		/**
			@brief 文件被关闭时发生该通知。
		*/
		virtual bool notify_hup() = 0;
		/**
			@brief 文件发生错误时发生该通知。
		*/
		virtual bool notify_err() = 0;
		/**
			@brief 文件中有可读的私有数据时发生该通知。
		*/
		virtual bool notify_pri() = 0;

		virtual native_filedescriptor_type getfd() = 0;
	  public:
		aioselector();
		virtual ~aioselector();
	};

	/**
		@brief 异步文件句柄容器
	*/
	class _lyramilk_api_ aiopoll : public lyramilk::threading::threads
	{
	  protected:
		friend class aioselector;
		const static int pool_max = 1000000;
		native_epool_type epfd;
		virtual bool transmessage();
	  public:
		aiopoll();
		virtual ~aiopoll();

		virtual bool add(aioselector* r);
		virtual bool add(aioselector* r,uint32 mask);
		virtual bool reset(aioselector* r,uint32 mask);
		virtual bool remove(aioselector* r);

		virtual void onevent(aioselector* r,uint32 events);
		virtual int svc();
	};
}}

#endif
