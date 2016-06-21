#include "netaio.h"
#include "multilanguage.h"
#include "ansi_3_64.h"
#include "log.h"
#include <sys/epoll.h>
#include <sys/poll.h>

#include <sys/ioctl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <cassert>

#ifdef OPENSSL_FOUND
	#include <openssl/ssl.h>
	#include <openssl/err.h>
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
	// aiosession
	bool aiosession::notify_in()
	{
		char buff[4096];
		int i = 0;
		if(!sendcache){
			sendcache.str("");
			sendcache.clear();
		}
		do{
#ifdef OPENSSL_FOUND
			if(ssl()){
				i = SSL_read((SSL*)sslobj, buff,4096);
			}else{
				i = ::recv(sock,buff,4096,0);
			}
#else
			i = ::recv(sock,buff,4096,0);
#endif
			if(i == 0) return false;
			if(i == -1){
				if(errno == EAGAIN ) break;
				if(errno == EINTR) continue;
				return false;
			}
			assert(i > 0);

			if(!onrequest(buff,i,sendcache)){
				flag &= ~EPOLLIN;
				return notify_out();
			}
		}while(i > 0);

		return notify_out();
	}

	bool aiosession::notify_out()
	{
		char buff[4096];
		while(sendcache){
			sendcache.read(buff,4096);
			int sendcount = sendcache.gcount();
			if(sendcount == 0){
				break;
			}
			int rt = 0;
#ifdef OPENSSL_FOUND
			if(ssl()){
				rt = SSL_write((SSL*)sslobj, buff,sendcount);
			}else{
				rt = ::send(sock,buff,sendcount,0);
			}
#else
			rt = ::send(sock,buff,sendcount,0);
#endif
			if(rt == 0) return false;
			if(rt == -1){
				if(errno == EAGAIN || errno == EINTR){
					sendcache.seekg( 0 - sendcount,std::stringstream::cur);
					break;
				}
				return false;
			}
			assert(rt > 0);
			if(rt < sendcount){
				int rc = rt - sendcount;
				assert(rc < 0);
				sendcache.seekg(rc,std::stringstream::cur);
				break;
			}
		}
		if(sendcache && sendcache.rdbuf()->in_avail()){
			return container->reset(this,EPOLLOUT | flag);
		}else{
			sendcache.str("");
			sendcache.clear();
		}

		if(flag & EPOLLIN){
			return container->reset(this,flag);
		}
		return false;
	}

	bool aiosession::notify_hup()
	{
		return false;
	}

	bool aiosession::notify_err()
	{
		return false;
	}

	bool aiosession::notify_pri()
	{
		return false;
	}

	void aiosession::ondestory()
	{
		onfinally(sendcache);
		assert(dtr);
		dtr(this);
	}

	aiosession::aiosession()
	{
		flag = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLONESHOT;
	}

	aiosession::~aiosession()
	{
		close();
	}

	bool aiosession::oninit(lyramilk::data::ostream& os)
	{
		return true;
	}

	void aiosession::onfinally(lyramilk::data::ostream& os)
	{
	}
	
	native_filedescriptor_type aiosession::getfd()
	{
		return sock;
	}

	bool aiosession::read(void* buf, lyramilk::data::uint32 len,lyramilk::data::uint32 delay)
	{
		lyramilk::klog(lyramilk::log::error,"lyramilk.system.netio.aiosession.read") << lyramilk::kdict("不该调用此方法。") << std::endl;
		TODO();
	}

	bool aiosession::write(const void* buf, lyramilk::data::uint32 len,lyramilk::data::uint32 delay)
	{
		lyramilk::klog(lyramilk::log::error,"lyramilk.system.netio.aiosession.write") << lyramilk::kdict("不该调用此方法。") << std::endl;
		TODO();
	}

	bool aiosession::check_read(lyramilk::data::uint32 delay)
	{
		lyramilk::klog(lyramilk::log::error,"lyramilk.system.netio.aiosession.check_read") << lyramilk::kdict("不该调用此方法。") << std::endl;
		TODO();
	}

	bool aiosession::check_write(lyramilk::data::uint32 delay)
	{
		lyramilk::klog(lyramilk::log::error,"lyramilk.system.netio.aiosession.check_write") << lyramilk::kdict("不该调用此方法。") << std::endl;
		TODO();
	}

	bool aiosession::check_error()
	{
		lyramilk::klog(lyramilk::log::error,"lyramilk.system.netio.aiosession.check_error") << lyramilk::kdict("不该调用此方法。") << std::endl;
		TODO();
	}

	// aiolistener
#ifdef OPENSSL_FOUND
	static bool sock_read_able(native_socket_type sock,int delay)
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
#endif

	bool aiolistener::notify_in()
	{
		sockaddr_in addr;
		socklen_t addr_size = sizeof(addr);
		native_socket_type acceptfd = ::accept(sock,(sockaddr*)&addr,&addr_size);
		if(acceptfd > 0){
#ifdef OPENSSL_FOUND
			SSL* sslptr = nullptr;
			if(ssl() && sslctx){
				sslptr = SSL_new((SSL_CTX*)sslctx);
				if(SSL_set_fd(sslptr,acceptfd) != 1) {
					SSL_shutdown(sslptr);
					SSL_free(sslptr);
					sslptr = nullptr;
					if(!ssl_self_adaptive){
						lyramilk::klog(lyramilk::log::warning,"lyramilk.system.netio.aiolistener.EPOLLIN.ssl") << lyramilk::kdict("绑定套接字失败:%s",_ssl.err().c_str()) << std::endl;
						::close(acceptfd);
						return true;
					}else{
						lyramilk::klog(lyramilk::log::warning,"lyramilk.system.netio.aiolistener.EPOLLIN.ssl") << lyramilk::kdict("在自适应模式中绑定套接字失败:%s",_ssl.err().c_str()) << std::endl;
					}
				}
				if(ssl_self_adaptive){
					if(sslptr){
						SSL_set_accept_state(sslptr);
						if(sock_read_able(acceptfd,600)){
							char buff[6];
							if(::recv(acceptfd,buff,6,MSG_PEEK)>0){
								if(buff[0] == 0x16 && buff[5] == 0x01){
									if(SSL_do_handshake(sslptr) != 1) {
										lyramilk::klog(lyramilk::log::warning,"lyramilk.system.netio.aiolistener.EPOLLIN.ssl.handshake") << lyramilk::kdict("在自适应模式中握手失败:%s",_ssl.err().c_str()) << std::endl;
										SSL_shutdown(sslptr);
										SSL_free(sslptr);
										::close(acceptfd);
										return true;
									}
								}else{
									SSL_shutdown(sslptr);
									SSL_free(sslptr);
									sslptr = nullptr;
								}
							}
						}else{
							lyramilk::klog(lyramilk::log::warning,"lyramilk.system.netio.aiolistener.EPOLLIN.ssl.handshake") << lyramilk::kdict("在自适应模式中握手失败:%s","握手超时。") << std::endl;
							SSL_shutdown(sslptr);
							SSL_free(sslptr);
							sslptr = NULL;
						}
					}
				}else{
					SSL_set_accept_state(sslptr);
					if(SSL_do_handshake(sslptr) != 1) {
						lyramilk::klog(lyramilk::log::warning,"lyramilk.system.netio.aiolistener.EPOLLIN.ssl.handshake") << lyramilk::kdict("在自适应模式中握手失败:%s",_ssl.err().c_str()) << std::endl;
						SSL_shutdown(sslptr);
						SSL_free(sslptr);
						::close(acceptfd);
						return true;
					}
				}
			}
#endif
			aiosession* s = create();
			if(s == nullptr){
				lyramilk::klog(lyramilk::log::warning,"lyramilk.system.netio.aiolistener.EPOLLIN") << lyramilk::kdict("创建会话失败") << std::endl;
				::close(acceptfd);
				return true;
			}
			s->sock = acceptfd;
#ifdef OPENSSL_FOUND
			s->sslobj = sslptr;
#endif
			if(s->oninit(s->sendcache)){
				/*非阻塞模式*/
				unsigned int argp = 1;
				ioctl(acceptfd,FIONBIO,&argp);
				assert(container);
				container->add(s,s->flag);
			}else{
				s->ondestory();
			}
			return true;
		}
		return true;
	}

	bool aiolistener::notify_out()
	{
		return false;
	}

	bool aiolistener::notify_hup()
	{
		return false;
	}

	bool aiolistener::notify_err()
	{
		return false;
	}

	bool aiolistener::notify_pri()
	{
		return false;
	}

	aiolistener::aiolistener() : sslctx(nullptr),use_ssl(false),ssl_self_adaptive(false)
	{
	}

	aiolistener::~aiolistener()
	{
		close();
#ifdef OPENSSL_FOUND
		if(sslctx){
			SSL_CTX_free((SSL_CTX*)sslctx);
		}
#endif
	}

	bool aiolistener::open(lyramilk::data::uint16 port)
	{
		if(sock){
			lyramilk::klog(lyramilk::log::error,"lyramilk.system.netio.aiolistener.open") << lyramilk::kdict("打开监听套件字失败，因为该套接字己打开。") << std::endl;
			return false;
		}

		sock = ::socket(AF_INET,SOCK_STREAM, IPPROTO_IP);
		if(!sock){
			lyramilk::klog(lyramilk::log::error,"lyramilk.system.netio.aiolistener.open") << lyramilk::kdict("监听时发生错误：%s",strerror(errno)) << std::endl;
			return false;
		}

		sockaddr_in addr = {0};
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		int opt = 1;
		setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
		if(::bind(sock,(const sockaddr*)&addr,sizeof(addr))){
			close();
			lyramilk::klog(lyramilk::log::error,"lyramilk.system.netio.aiolistener.open") << lyramilk::kdict("绑定地址(%s:%d)时发生错误：%s",inet_ntoa(addr.sin_addr),port,strerror(errno)) << std::endl;
			return false;
		}
		unsigned int argp = 1;
		ioctl(sock,FIONBIO,&argp);


		int ret = listen(sock,5);
		if(ret == 0){
			lyramilk::klog(lyramilk::log::debug,"lyramilk.system.netio.aiolistener.open") << lyramilk::kdict("监听：%d",port) << std::endl;
			//signal(SIGPIPE, SIG_IGN);
			return true;
		}
		close();
		return false;
	}

	bool aiolistener::init_ssl(lyramilk::data::string certfilename, lyramilk::data::string keyfilename)
	{
#ifdef OPENSSL_FOUND
		sslctx = SSL_CTX_new(SSLv23_server_method());
		
		int r = 0;
		if(!certfilename.empty()){
			r = SSL_CTX_use_certificate_file((SSL_CTX*)sslctx, certfilename.c_str(), SSL_FILETYPE_PEM);
			if(r != 1) {
				lyramilk::klog(lyramilk::log::warning,"lyramilk.system.netio.aiolistener.ssl.init_ssl") << lyramilk::kdict("设置公钥失败:%s",_ssl.err().c_str()) << std::endl;
				return false;
			}
		}
		if(!keyfilename.empty()){
			r = SSL_CTX_use_PrivateKey_file((SSL_CTX*)sslctx, keyfilename.c_str(), SSL_FILETYPE_PEM);
			if(r != 1) {
				lyramilk::klog(lyramilk::log::warning,"lyramilk.system.netio.aiolistener.ssl.init_ssl") << lyramilk::kdict("设置私钥失败:%s",_ssl.err().c_str()) << std::endl;
				return false;
			}
		}
		if(!certfilename.empty() && !keyfilename.empty()){
			r = SSL_CTX_check_private_key((SSL_CTX*)sslctx);
			if(r != 1) {
				lyramilk::klog(lyramilk::log::warning,"lyramilk.system.netio.aiolistener.ssl.init_ssl") << lyramilk::kdict("验证密钥失败:%s",_ssl.err().c_str()) << std::endl;
				return false;
			}
		}
		
		SSL_CTX_set_options((SSL_CTX*)sslctx, SSL_OP_TLS_ROLLBACK_BUG);
		ssl(true);
		return true;
#else
		lyramilk::klog(lyramilk::log::error,"lyramilk.system.netio.aiolistener.ssl.init_ssl") << lyramilk::kdict("不支持SSL") << std::endl;
		return false;
#endif
	}

	void aiolistener::ssl(bool use_ssl, bool ssl_self_adaptive)
	{
		this->use_ssl = use_ssl;
		this->ssl_self_adaptive = ssl_self_adaptive;
	}

	bool aiolistener::ssl()
	{
		return use_ssl && sslctx;
	}

	native_filedescriptor_type aiolistener::getfd()
	{
		return sock;
	}

	
	void aiolistener::ondestory()
	{
	}

}}
