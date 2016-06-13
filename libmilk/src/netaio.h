#ifndef _lyramilk_system_netaio_h_
#define _lyramilk_system_netaio_h_

#include "netio.h"

namespace lyramilk{namespace system{namespace netio
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
	class _lyramilk_api_ aiopoll : public lyramilk::system::threading::threads
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

	/**
		@brief 异步套接字会话
	*/
	class _lyramilk_api_ aiosession : public aioselector,public socket
	{
	public:
		typedef aiosession* (*builder)();
		typedef void(*destoryer)(aiosession* s);
	protected:
		destoryer dtr;
	private:
		friend class aiolistener;
		virtual bool notify_in();
		virtual bool notify_out();
		virtual bool notify_hup();
		virtual bool notify_err();
		virtual bool notify_pri();
		lyramilk::data::stringstream sendcache;
		int flag;
		virtual void ondestory();
	public:
		aiosession();
		virtual ~aiosession();

		/**
			@brief 连接时触发
			@return 返回false会导致服务器主动断开链接。
		*/
		virtual bool oninit(lyramilk::data::ostream& os);

		/**
			@brief 断开时触发
			@return 返回false会导致服务器主动断开链接。
		*/
		virtual void onfinally(lyramilk::data::ostream& os);

		/**
			@brief 收到数据时触发。
			@param cache 这里面有新数据。
			@param size 新数据的字节数。
			@return 返回false会导致服务器主动断开链接。
		*/
		virtual bool onrequest(const char* cache, int size, lyramilk::data::ostream& os) = 0;

		/// 模板化会话对象的销毁函数
		template <typename T>
		static void __tdestoryer(aiosession* s)
		{
			delete static_cast<T*>(s);
		}

		/// 模板化会话对象的生成函数
		template <typename T>
		static T* __tbuilder()
		{
			T* p = new T();
			p->dtr = __tdestoryer<T>;
			return p;
		}
		virtual native_filedescriptor_type getfd();
	private:
		virtual bool read(void* buf, lyramilk::data::uint32 len,lyramilk::data::uint32 delay);
		virtual bool write(const void* buf, lyramilk::data::uint32 len,lyramilk::data::uint32 delay);

		virtual bool check_read(lyramilk::data::uint32 delay);
		virtual bool check_write(lyramilk::data::uint32 delay);
		virtual bool check_error();
	};

	/**
		@brief 异步套接字监听会话
	*/
	class _lyramilk_api_ aiolistener : public aioselector,public socket
	{
		ssl_ctx_type sslctx;
		bool use_ssl;
		bool ssl_self_adaptive;
		virtual bool notify_in();
		virtual bool notify_out();
		virtual bool notify_hup();
		virtual bool notify_err();
		virtual bool notify_pri();
	public:
		aiolistener();
		virtual ~aiolistener();

		/**
			@brief 打开一个端口并监听。
			@param port 被打开的端口。
			@return 如果打开成功返回true。
		*/
		virtual bool open(lyramilk::data::uint16 port);

		/**
			@brief 创建一个会话。
			@return 创建的会话。这个会话应该用destory释放。
		*/
		virtual aiosession* create() = 0;

		/**
			@brief 初始化SSL并自动开启SSL。
			@param certfilename 证书
			@param keyfilename 私钥
			@return false标示初始化SSL失败。
		*/
		virtual bool init_ssl(lyramilk::data::string certfilename, lyramilk::data::string keyfilename);

		/**
			@brief 开启或关闭SSL。
			@param use_ssl 为false表示关闭SSL，新创建的会话将以明文传输。
			@param ssl_self_adaptive 当开启SSL时对客户端自适应。为true时，当客户端以非SSL进行连接时，服务器以非SSL接受。为false时，客户端以非SSL进行连接时，强制关闭该会话。
		*/
		virtual void ssl(bool use_ssl, bool ssl_self_adaptive = false);

		/// 检查该套接字的通信是否己加密。
		virtual bool ssl();


		virtual native_filedescriptor_type getfd();

		virtual void ondestory();
	};

	/**
	@brief 异步套接字服务器
	*/
	template <typename T>
	class aioserver : public aiolistener
	{
		virtual aiosession* create()
		{
			return aiosession::__tbuilder<T>();
		}
	};
}}}

#endif
