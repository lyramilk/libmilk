#include "multilanguage.h"
#include "ansi_3_64.h"
#include "log.h"

#include <errno.h>
#include <string.h>
#include <cassert>

#include <iostream>
#include <streambuf>

#define Z_HAVE_OPENSSL

#ifdef Z_HAVE_OPENSSL
	#include <openssl/ssl.h>
	#include <openssl/bio.h>
	#include <openssl/crypto.h>
	#include <openssl/evp.h>
	#include <openssl/x509.h>
	#include <openssl/x509v3.h>
	#include <openssl/ssl.h>
	#include <openssl/err.h>
	#include <string.h>
#endif
#define SSL_TYPE SSL*
#define SSL_CTX_TYPE SSL_CTX*
#include "netio.h"

#ifdef Z_HAVE_OPENSSL
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

#ifdef WIN32
	struct __socket_library
	{
		__socket_library()
		{
			WSADATA wsa;
			WSAStartup(MAKEWORD(2, 2), &wsa);
		}
		~__socket_library()
		{
			WSACleanup();
		}
	};

	static __socket_library _socklib;
#endif



namespace lyramilk{namespace system{namespace netio
{
	netaddress::netaddress(lyramilk::data::string host, lyramilk::data::uint16 port):u_ip(0),u_port(0)
	{
		hostent* h = gethostbyname(host.c_str());
		if (h == NULL){
			return ;
		}

		in_addr* inaddr = (in_addr*)h->h_addr;;
		if (inaddr == NULL){
			return ;
		}

	}

	netaddress::netaddress(lyramilk::data::uint16 port) :u_ip(htonl(INADDR_ANY)), u_port(port)
	{
	}

	netaddress::netaddress() : u_ip(htonl(INADDR_ANY)), u_port(0)
	{
	}

	lyramilk::data::string netaddress::ip_str()
	{
		in_addr in;
		in.s_addr = u_ip;
		return inet_ntoa(in);

	}

	socket::socket()
	{
		fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	}

	socket::~socket()
	{
		close();
	}

	bool socket::ssl()
	{
		return sslobj != nullptr;
	}

	lyramilk::data::int32 socket::read(char* buf, lyramilk::data::int32 len)
	{
#ifdef Z_HAVE_OPENSSL
		if (sslobj) return SSL_read(sslobj, buf, len);
#endif
		return ::recv(fd, buf, len, 0);
	}

	lyramilk::data::int32 socket::write(const char* buf, lyramilk::data::int32 len)
	{
#ifdef Z_HAVE_OPENSSL
		if (sslobj) return SSL_write(sslobj, buf, len);
#endif
		return ::send(fd, buf, len, 0);
	}

	bool socket::check_read(lyramilk::data::uint32 delay)
	{
		fd_set fdset;
		FD_ZERO(&fdset);
		FD_SET(fd, &fdset);
		timeval tv;
		tv.tv_sec = delay / 1000;
		tv.tv_usec = 1000 * (delay % 1000);
		select(2, &fdset, nullptr, nullptr, &tv);
		return FD_ISSET(fd, &fdset) != 0;
	}

	bool socket::check_write(lyramilk::data::uint32 delay)
	{
		fd_set fdset;
		FD_ZERO(&fdset);
		FD_SET(fd, &fdset);
		timeval tv;
		tv.tv_sec = delay / 1000;
		tv.tv_usec = 1000 * (delay % 1000);
		select(2, nullptr, &fdset, nullptr, &tv);
		return FD_ISSET(fd, &fdset) != 0;
	}

	bool socket::isalive()
	{
		if (fd == 0) return false;
		fd_set fdset;
		FD_ZERO(&fdset);
		FD_SET(fd, &fdset);
		timeval tv = {0};
		select(2, nullptr, nullptr, &fdset, &tv);
		return FD_ISSET(fd, &fdset) != 0;
	}

	bool socket::close()
	{
#ifdef Z_HAVE_OPENSSL
		if (sslobj){
			SSL_shutdown(sslobj);
			SSL_free(sslobj);
			sslobj = nullptr;
		}
#endif
		bool ret = ::closesocket(fd) == 0;
		fd = 0;
		return false;
	}

	netaddress socket::source() const
	{
		netaddress ret;
		sockaddr_in addr;
		int size = sizeof addr;
		if (getpeername(fd, (sockaddr*)&addr, &size) == 0){
			ret.u_ip = addr.sin_addr.s_addr;
			ret.u_port = addr.sin_port;
		}
		return ret;
	}

	netaddress socket::desst() const
	{
		netaddress ret;
		sockaddr_in addr;
		int size = sizeof addr;
		if (getsockname(fd, (sockaddr*)&addr, &size) == 0){
			ret.u_ip = addr.sin_addr.s_addr;
			ret.u_port = addr.sin_port;
		}
		return ret;
	}

	client::client()
	{
	}

	client::~client()
	{
	}

	bool client::open(netaddress addr)
	{
		sockaddr_in saddr = { 0 };
		saddr.sin_addr.s_addr = addr.u_ip;
		saddr.sin_family = AF_INET;
		saddr.sin_port = htons(addr.u_port);
		if (0 == ::connect(fd, (const sockaddr*)&saddr, sizeof(saddr))){


			return true;
		}
		
		return false;
	}

	bool client::ssl(bool use_ssl)
	{
		sslenable = use_ssl;
#ifdef Z_HAVE_OPENSSL
		if (sslobj == nullptr && sslctx == nullptr){
			sslobj = SSL_new(sslctx);
			int r = SSL_set_fd(sslobj, fd);
			if (r != 1) {
				lyramilk::klog(lyramilk::log::warning, "lyramilk.system.netio.client.ssl") << lyramilk::kdict("绑定套接字失败:%s", _ssl.err().c_str()) << std::endl;
				return false;
			}
		}
#endif
		return true;
	}

	bool client::init_ssl(lyramilk::data::string certfilename, lyramilk::data::string keyfilename)
	{
#ifdef Z_HAVE_OPENSSL
#endif
	}

}}}

