#ifndef _lyramilk_io_aio_h_
#define _lyramilk_io_aio_h_

#include "thread.h"
#include "atom.h"
#include <set>


/**
	@namespace lyramilk::io
	@brief io
	@details 该命名空间描述输入/输出
*/

namespace lyramilk{namespace io
{
	typedef int native_epool_type;
	typedef int native_filedescriptor_type;
	typedef unsigned int uint32;

	/**
		@brief 异步容器的选择子
	*/
	class _lyramilk_api_ aiopoll_safe;
	class _lyramilk_api_ aioselector
	{
	  protected:
		uint32 mask;
		uint32 flag;
		lyramilk::data::int64 thread_idx;

		friend class aiopoll;
		friend class aiopoll_safe;
		lyramilk::threading::mutex_spin mlock;
		aiopoll_safe* pool;
		virtual void ondestory() = 0;
	  protected:
		/**
			@brief 文件可读时发生该通知。
		*/
		virtual bool notify_in() = 0;
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
		/**
			@brief 加入容器时发生该通知。
		*/
		virtual bool notify_attach(aiopoll_safe* container);
		/**
			@brief 从容器中移除时发生该通知。
		*/
		virtual void notify_detach(aiopoll_safe* container);

	  public:
		aioselector();
		virtual ~aioselector();
		/**
			@brief 暂时锁定这个事件，不会再触发事件
		*/
		virtual bool lock();
		/**
			@brief 恢复lock状态
		*/
		virtual bool unlock();
		virtual lyramilk::data::int64 get_thread_idx();
		virtual native_filedescriptor_type getfd() = 0;
	};

	//TODO 要改为每个线程一个epoll来做。记录业务工作时间，时间过长的时候创建新线程接管，同时标记旧线程。 
	class _lyramilk_api_ aiopoll_safe : public lyramilk::threading::threads
	{
	  protected:
		friend class aioselector;
		lyramilk::threading::atomic<std::size_t> thread_idx;
		lyramilk::data::int64 fdcount;
		pthread_key_t seq_key;
		struct epinfo
		{
			native_epool_type epfd;
			lyramilk::data::uint64 payload;
		};
		std::vector<epinfo> epfds;
		virtual bool active(std::size_t threadcount);
	  protected:
		virtual int svc();
		virtual native_epool_type getfd(std::size_t thread_idx);
		virtual native_epool_type getfd();
	  public:
		virtual bool add_to_thread(lyramilk::data::int64 idx,aioselector* r,lyramilk::data::int64 mask);
	  public:
		aiopoll_safe(std::size_t threadcount);
		virtual ~aiopoll_safe();

		virtual bool add(aioselector* r,lyramilk::data::int64 mask = -1);
		virtual bool reset(aioselector* r,lyramilk::data::int64 mask);
		virtual bool detach(aioselector* r);
		virtual bool remove(aioselector* r);

		virtual lyramilk::data::int64 get_thread_idx();
		virtual lyramilk::data::int64 get_fd_count();
		virtual void onevent(aioselector* r,lyramilk::data::int64 events);
		virtual bool active();
	};

	/**
		@brief 异步文件句柄容器
	*/
	class _lyramilk_api_ aiopoll : protected aiopoll_safe
	{
	  protected:
		friend class aioselector;
		virtual bool transmessage();
	  public:
		aiopoll();
		virtual ~aiopoll();

		/*
			@brief 把事件添加到消息池中
			@details  注意！！！执行完这一行代码后，r就有被释放掉的可能性。不能再用了。
		*/
		virtual bool add(aioselector* r,lyramilk::data::int64 mask = -1);
		/*
			@brief 重置池中的事件
			@details  注意！！！执行完这一行代码后，r就有被释放掉的可能性。不能再用了。
		*/
		virtual bool reset(aioselector* r,lyramilk::data::int64 mask);

		/// 把r从池中移除，会导致r被释放。
		virtual bool remove(aioselector* r);

		/// 把r从池中移除，不会导致r被释放。
		virtual bool detach(aioselector* r);
		virtual int svc();
		virtual bool active(std::size_t threadcount);
	};


}}

#endif
