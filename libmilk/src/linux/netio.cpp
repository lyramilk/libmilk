#include "netio.h"
#include "multilanguage.h"
#include "ansi_3_64.h"
#include "log.h"
#include <sys/epoll.h>
#include <sys/poll.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <cassert>

#include <iostream>
#include <streambuf>


#ifdef OPENSSL_FOUND
	#include <openssl/bio.h>
	#include <openssl/crypto.h>
	#include <openssl/evp.h>
	#include <openssl/x509.h>
	#include <openssl/x509v3.h>
	#include <openssl/ssl.h>
	#include <openssl/err.h>
	#include <string.h>
#endif

#ifdef OPENSSL_FOUND
	struct __ssl
	{
		__ssl()
		{
			SSL_library_init();
			SSL_load_error_strings();
			ERR_load_BIO_strings();
			OpenSSL_add_all_algorithms();
		}
		~__ssl()
		{
		}

		lyramilk::data::string err()
		{
			char buff[4096] = {0};
			ERR_error_string(ERR_get_error(),buff);
			return buff;
		}
	};

	static __ssl _ssl;
#endif



namespace lyramilk{namespace system{namespace netio
{
	netaddress::netaddress(lyramilk::data::string host, lyramilk::data::uint16 port)
	{
		this->host = host;
		this->port = port;

	}

	netaddress::netaddress(lyramilk::data::uint32 ipv4, lyramilk::data::uint16 port)
	{
		in_addr in;
		in.s_addr = ipv4;
		host = inet_ntoa(in);
		this->port = port;
	}

	netaddress::netaddress(lyramilk::data::uint16 port)
	{
		this->port = port;
	}

	netaddress::netaddress()
	{
		this->port = 0;
	}

	lyramilk::data::string netaddress::ip_str()
	{
		return this->host;
	}

	//////
	socket::socket()
	{
		sslobj = nullptr;
		sslenable = false;
		sock = 0;
	}

	socket::~socket()
	{
		close();
#ifdef OPENSSL_FOUND
		if(sslobj){
			SSL_shutdown((SSL*)sslobj);
			SSL_free((SSL*)sslobj);
		}
#endif
	}

	bool socket::ssl()
	{
		return sslobj != NULL;
	}

	lyramilk::data::int32 socket::read(char* buf, lyramilk::data::int32 len,lyramilk::data::uint32 delay)
	{
		if(!check_read(delay)){
			errno = EAGAIN;
			return -1;
		}
		int rt = 0;
#ifdef OPENSSL_FOUND
		if(ssl()){
			rt = SSL_read((SSL*)sslobj, buf, len);
		}else{
			rt = ::recv(sock,buf,len,0);
		}
#else
		rt = ::recv(sock,buf,len,0);
#endif
		if(rt < 0){
			lyramilk::klog(lyramilk::log::warning,"lyramilk.system.nio.socket.read") << lyramilk::kdict("从套接字中读取数据时发生错误:%s",strerror(errno)) << std::endl;
		}else if(rt == 0){
			lyramilk::klog(lyramilk::log::warning,"lyramilk.system.nio.socket.read") << lyramilk::kdict("从套接字中读取数据时发生错误:%s","套接字己关闭") << std::endl;
		}
		return rt;
	}

	lyramilk::data::int32 socket::write(const char* buf, lyramilk::data::int32 len,lyramilk::data::uint32 delay)
	{
		if(!check_write(delay)){
			errno = EAGAIN;
			return -1;
		}
		int rt = 0;
#ifdef OPENSSL_FOUND
		if(ssl()){
			rt = SSL_write((SSL*)sslobj, buf, len);
		}else{
			rt = ::send(sock,buf,len,0);
		}
#else
		rt = ::send(sock,buf,len,0);
#endif
		if(rt < 0){
			lyramilk::klog(lyramilk::log::warning,"lyramilk.system.nio.client.write") << lyramilk::kdict("发生了错误:%s",strerror(errno)) << std::endl;
		}
		return rt;
	}

	bool socket::check_read(lyramilk::data::uint32 delay)
	{
		pollfd pfd;
		pfd.fd = sock;
		pfd.events = POLLIN;
		pfd.revents = 0;
		int ret = ::poll(&pfd,1,delay);
		if(ret > 0){
			if(pfd.revents & POLLIN){
				return true;
			}
		}
		return false;
	}

	bool socket::check_write(lyramilk::data::uint32 delay)
	{
		pollfd pfd;
		pfd.fd = sock;
		pfd.events = POLLOUT;
		pfd.revents = 0;
		int ret = ::poll(&pfd,1,delay);
		if(ret > 0){
			if(pfd.revents & POLLOUT){
				return true;
			}
		}
		return false;
	}

	bool socket::check_error()
	{
		pollfd pfd;
		pfd.fd = sock;
		pfd.events = POLLHUP | POLLERR;
		pfd.revents = 0;
		int ret = ::poll(&pfd,1,0);
		if(ret > 0){
			if(pfd.revents){
				return true;
			}
		}
		return false;
	}

	bool socket::isalive()
	{
		return check_write(0);
	}

	bool socket::close()
	{
		if(sock){
			::close(sock);
			sock = 0;
			return true;
		}
		return false;
	}

	netaddress socket::source() const
	{
		sockaddr_in addr;
		socklen_t size = sizeof addr;
		if(getpeername(sock,(sockaddr*)&addr,&size) !=0 ) return netaddress();
		return netaddress(addr.sin_addr.s_addr,addr.sin_port);
	}

	netaddress socket::dest() const
	{
		sockaddr_in addr;
		socklen_t size = sizeof addr;
		if(getsockname(sock,(sockaddr*)&addr,&size) !=0 ) return netaddress();
		return netaddress(addr.sin_addr.s_addr,addr.sin_port);
	}

	client::client()
	{}

	client::~client()
	{}

	bool client::open(netaddress addr)
	{
		TODO();
	}

	bool client::open(lyramilk::data::string host,lyramilk::data::uint16 port)
	{
		TODO();
	}

	bool client::ssl(bool use_ssl)
	{
		TODO();
	}

	bool client::init_ssl(lyramilk::data::string certfilename, lyramilk::data::string keyfilename)
	{
		TODO();
	}

}}}

