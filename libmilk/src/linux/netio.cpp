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



namespace lyramilk{namespace netio
{
	/* netaddress */
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

	netaddress::netaddress(lyramilk::data::string hostandport)
	{
		std::size_t sz = hostandport.find(':');
		if(sz == hostandport.npos){
			host = hostandport;
			port = 0;
		}else{
			host = hostandport.substr(0,sz);
			lyramilk::data::var vport = hostandport.substr(sz+1);
			port = vport;
		}

	}

	netaddress::netaddress()
	{
		this->port = 0;
	}

	lyramilk::data::string netaddress::ip_str() const
	{
		return this->host;
	}

	/* socket */
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
		return sslobj != nullptr;
	}


	bool socket::isalive()
	{
		pollfd pfd;
		pfd.fd = sock;
		pfd.events = POLLOUT;
		pfd.revents = 0;
		int ret = ::poll(&pfd,1,0);
		if(ret > 0){
			if(pfd.revents & POLLOUT){
				return true;
			}
		}
		return false;
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
		if(getsockname(sock,(sockaddr*)&addr,&size) !=0 ) return netaddress();
		return netaddress(addr.sin_addr.s_addr,ntohs(addr.sin_port));
	}

	netaddress socket::dest() const
	{
		sockaddr_in addr;
		socklen_t size = sizeof addr;
		if(getpeername(sock,(sockaddr*)&addr,&size) !=0 ) return netaddress();
		return netaddress(addr.sin_addr.s_addr,ntohs(addr.sin_port));
	}

	native_socket_type socket::fd() const
	{
		return sock;
	}

	/* socket_stream_buf */
	socket_stream_buf::int_type socket_stream_buf::sync()
	{
		return overflow();
	}
	socket_stream_buf::int_type socket_stream_buf::overflow (int_type c)
	{
		const char* p = pbase();
		int l = pptr() - pbase();
		int r = 0;
#ifdef OPENSSL_FOUND
		if(psock->ssl()){
			r = SSL_write((SSL*)psock->sslobj, p,l);
		}else{
			r = ::send(psock->sock,p,l,0);
		}
#else
		r = ::send(psock->sock,p,l,0);
#endif
		if(r>0){
			setp(putbuf.data() + r,putbuf.data() + putbuf.size());
			sputc(c);
			return 0;
		}
		return traits_type::eof();
	}

	socket_stream_buf::int_type socket_stream_buf::underflow()
	{
		char* p = getbuf.data();
		int l = getbuf.size();
		int r = 0;
errno = 0;
#ifdef OPENSSL_FOUND
		if(psock->ssl()){
			r = SSL_read((SSL*)psock->sslobj, p,l);
		}else{
			r = ::recv(psock->sock,p,l,0);
		}
#else
		r = ::recv(psock->sock,p,l,0);
#endif
		if(r>0){
			setg(getbuf.data(),getbuf.data(),getbuf.data() + r);
			//return sgetc();
			//return sbumpc();
			return *egptr();
		}
		return traits_type::eof();
	}

	socket_stream_buf::socket_stream_buf()
	{
		putbuf.assign(0x4000,0);
		setp(putbuf.data(),putbuf.data() + putbuf.size());

		getbuf.assign(0x4000,0);
		setg(getbuf.data(),getbuf.data() + getbuf.size(),getbuf.data() + getbuf.size());
	}

	socket_stream_buf::~socket_stream_buf()
	{
	}

	/* socket_stream */
	socket_stream::socket_stream(socket& ac):c(ac)
	{
		sbuf.psock = &ac;
		lyramilk::data::stringstream::init(&sbuf);
	}

	socket_stream::~socket_stream()
	{
	
	}

	/* client */
	client::client():use_ssl(false)
	{}

	client::~client()
	{}

	bool client::open(const netaddress& addr)
	{
		return open(addr.ip_str(),addr.port);
	}

	bool client::open(lyramilk::data::string host,lyramilk::data::uint16 port)
	{
		if(sock){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.client.open") << lyramilk::kdict("打开监听套件字失败，因为该套接字己打开。") << std::endl;
			return false;
		}
		hostent* h = gethostbyname(host.c_str());
		if(h == nullptr){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.client.open") << lyramilk::kdict("获取IP地址失败：%s",strerror(errno)) << std::endl;
			return false;
		}

		in_addr* inaddr = (in_addr*)h->h_addr;
		if(inaddr == nullptr){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.client.open") << lyramilk::kdict("获取IP地址失败：%s",strerror(errno)) << std::endl;
			return false;
		}

		native_socket_type tmpsock = ::socket(AF_INET,SOCK_STREAM, IPPROTO_IP);
		if(!tmpsock){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.client.open") << lyramilk::kdict("打开监听套件字失败：%s",strerror(errno)) << std::endl;
			return false;
		}

		sockaddr_in addr = {0};
		addr.sin_addr.s_addr = inaddr->s_addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);


		if(0 == ::connect(tmpsock,(const sockaddr*)&addr,sizeof(addr))){
#ifdef OPENSSL_FOUND
			if(use_ssl && sslctx){
				SSL* sslptr = SSL_new((SSL_CTX*)sslctx);
				if(SSL_set_fd(sslptr,tmpsock) != 1) {
					sslptr = nullptr;
					lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.client.ssl.onevent") << lyramilk::kdict("绑定套接字失败:%s",_ssl.err().c_str()) << std::endl;
					::close(tmpsock);
					return false;
				}

				SSL_set_connect_state(sslptr);
				if(SSL_do_handshake(sslptr) != 1) {
					lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.client.ssl.onevent") << lyramilk::kdict("握手失败:%s",_ssl.err().c_str()) << std::endl;
					::close(tmpsock);
					return false;
				}
				this->sslobj = sslptr;
			}
#endif
			unsigned int argp = 1;
			//ioctlsocket(sock,FIONBIO,&argp);
			ioctl(sock,FIONBIO,&argp);
			this->sock = tmpsock;
			return true;
		}
		::close(tmpsock);
		return false;
	}

	bool client::ssl()
	{
		return socket::ssl();
	}

	void client::ssl(bool use_ssl)
	{
		this->use_ssl = use_ssl;
	}

	bool client::init_ssl(lyramilk::data::string certfilename, lyramilk::data::string keyfilename)
	{
#ifdef OPENSSL_FOUND
		sslctx = SSL_CTX_new(SSLv23_client_method());
		int r = 0;
		if(!certfilename.empty()){
			r = SSL_CTX_use_certificate_file((SSL_CTX*)sslctx, certfilename.c_str(), SSL_FILETYPE_PEM);
			if(r != 1) {
				lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.client.ssl.init_ssl") << lyramilk::kdict("设置公钥失败:%s",_ssl.err().c_str()) << std::endl;
				return false;
			}
		}
		if(!keyfilename.empty()){
			r = SSL_CTX_use_PrivateKey_file((SSL_CTX*)sslctx, keyfilename.c_str(), SSL_FILETYPE_PEM);
			if(r != 1) {
				lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.client.ssl.init_ssl") << lyramilk::kdict("设置私钥失败:%s",_ssl.err().c_str()) << std::endl;
				return false;
			}
		}
		if(!certfilename.empty() && !keyfilename.empty()){
			r = SSL_CTX_check_private_key((SSL_CTX*)sslctx);
			if(r != 1) {
				lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.client.ssl.init_ssl") << lyramilk::kdict("验证公钥失败:%s",_ssl.err().c_str()) << std::endl;
				return false;
			}
		}
		SSL_CTX_set_options((SSL_CTX*)sslctx, SSL_OP_TLS_ROLLBACK_BUG);
		ssl(true);
		return true;
#else
		lyramilk::klog(lyramilk::log::error,"lyramilk.netio.client.ssl.init_ssl") << lyramilk::kdict("不支持SSL") << std::endl;
		return false;
#endif
	}

	lyramilk::data::int32 client::read(char* buf, lyramilk::data::int32 len,lyramilk::data::uint32 delay)
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
			lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.client.read") << lyramilk::kdict("从套接字中读取数据时发生错误:%s",strerror(errno)) << std::endl;
		}else if(rt == 0){
			lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.client.read") << lyramilk::kdict("从套接字中读取数据时发生错误:%s","套接字己关闭") << std::endl;
		}
		return rt;
	}

	lyramilk::data::int32 client::write(const char* buf, lyramilk::data::int32 len,lyramilk::data::uint32 delay)
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
			lyramilk::klog(lyramilk::log::warning,"lyramilk.netio.client.write") << lyramilk::kdict("发生了错误:%s",strerror(errno)) << std::endl;
		}
		return rt;
	}

	bool client::check_read(lyramilk::data::uint32 delay)
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

	bool client::check_write(lyramilk::data::uint32 delay)
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

	bool client::check_error()
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
}}

