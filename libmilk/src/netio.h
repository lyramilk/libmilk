#ifndef _lyramilk_system_netio_h_
#define _lyramilk_system_netio_h_

#include <memory>
#include <exception>

#include "var.h"
#include "thread.h"

#ifndef SSL_TYPE
	typedef void* SSL_TYPE;
	typedef void* SSL_CTX_TYPE;
#endif

/**
	@namespace lyramilk::netio
	@brief 网络io
	@details 该命名空间描述网络io通信
*/
namespace lyramilk{namespace netio
{
#ifdef WIN32
	typedef SOCKET native_socket_type;
#elif defined __linux__
	typedef int native_socket_type;
#endif
	typedef SSL_TYPE ssl_type;
	typedef SSL_CTX_TYPE ssl_ctx_type;

	class _lyramilk_api_ netaddress
	{
	  public:
		///网络字节序
		lyramilk::data::string host;
		///本地字节序
		lyramilk::data::uint16 port;

		netaddress(lyramilk::data::string host, lyramilk::data::uint16 port);
		netaddress(lyramilk::data::uint32 ipv4, lyramilk::data::uint16 port);
		netaddress(lyramilk::data::uint16 port);
		netaddress(lyramilk::data::string hostandport);
		netaddress();
		lyramilk::data::string ip_str() const;
	};

	class _lyramilk_api_ socket
	{
	  protected:
		friend class socket_stream_buf;
		ssl_type sslobj;
		bool sslenable;
		native_socket_type sock;
	  public:
		socket();
		virtual ~socket();
		//检查该套接字的通信是否己加密
		virtual bool ssl();

		//判断套接字是否可用
		virtual bool isalive();

		//关闭套接字
		virtual bool close();

		/// 取得本端ip
		virtual netaddress source() const;

		/// 取得对端ip
		virtual netaddress dest() const;

		/// 取得套接字
		virtual native_socket_type fd() const;
	};

	class _lyramilk_api_ socket_stream;

	/// 以流的方式操作套接字的流缓冲
	class _lyramilk_api_ socket_stream_buf : public std::basic_streambuf<char>
	{
		friend class socket_stream;
	  protected:
		lyramilk::netio::socket* psock;
		std::vector<char> putbuf;
		std::vector<char> getbuf;
		virtual int_type sync();
		virtual int_type overflow (int_type c = traits_type::eof());
		virtual int_type underflow();
	  public:
		socket_stream_buf();
		virtual ~socket_stream_buf();
	};

	/*
		@brief 以流的方式操作套接字的流
		@details 只支持读写
	*/
	class _lyramilk_api_ socket_stream : public lyramilk::data::stringstream
	{
		socket_stream_buf sbuf;
		socket& c;
	  public:
		socket_stream(socket& ac);
		virtual ~socket_stream();
	};

	/// 客户端套接字
	class _lyramilk_api_ client : public socket
	{
		ssl_ctx_type sslctx;
		bool use_ssl;
	public:
		client();
		virtual ~client();

		virtual bool open(const netaddress& addr);
		virtual bool open(lyramilk::data::string host,lyramilk::data::uint16 port);

		virtual bool ssl();
		virtual void ssl(bool use_ssl);
		virtual bool init_ssl(lyramilk::data::string certfilename = "", lyramilk::data::string keyfilename = "");

		/*
			@brief 从套接字中读取数据
			@param buf 从套接字中读取的数据将写入该缓冲区。
			@param len buf的内存长度。
			@param delay 等待的毫秒数。
			@return 实际读取得字符数。小于1时表示读取失败。
		*/

		virtual lyramilk::data::int32 read(char* buf, lyramilk::data::int32 len,lyramilk::data::uint32 delay = 3000);

		/*
			@brief 向套接字写入数据
			@param buf 该缓冲区的数据将写入到套接字中。
			@param len buf的内存长度。
			@param delay 等待的毫秒数。
			@return 在delay毫秒内如果套接字可写则返回true。
		*/
		virtual lyramilk::data::int32 write(const char* buf, lyramilk::data::int32 len,lyramilk::data::uint32 delay = 3000);

		/*
			@brief 检查套接字是否可读
			@param delay 等待的毫秒数。
			@return 在delay毫秒内如果套接字可读则返回true。
		*/
		virtual bool check_read(lyramilk::data::uint32 delay);

		/*
			@brief 检查套接字是否可写
			@param delay 等待的毫秒数。
			@return 在delay毫秒内如果套接字可写则返回true。
		*/
		virtual bool check_write(lyramilk::data::uint32 delay);

		/*
			@brief 检查套接字是有错误
		*/
		virtual bool check_error();
	};


}}

#endif
