#include "netproxy.h"
#include "dict.h"
#include "ansi_3_64.h"
#include "log.h"
#include "debug.h"
#include "testing.h"

#include <sys/epoll.h>
#include <sys/poll.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <cassert>
#include <netdb.h>
#include <sys/ioctl.h>

#ifdef OPENSSL_FOUND
	#include <openssl/ssl.h>
	#include <openssl/err.h>
#endif


namespace lyramilk{namespace netio
{
	lyramilk::data::string inline ssl_err()
	{
		char buff[4096] = {0};
#ifdef OPENSSL_FOUND
		ERR_error_string(ERR_get_error(),buff);
#endif
		return buff;
	}



	const unsigned int flag_default = EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLONESHOT;
	//const unsigned int flag_default = EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLET;
	

	// aioproxysession_connector
	bool aioproxysession_connector::ssl()
	{
		return false;
	}

	void aioproxysession_connector::ssl(bool use_ssl)
	{
	}

	bool aioproxysession_connector::init_ssl(const lyramilk::data::string& certfilename, const lyramilk::data::string& keyfilename)
	{
		lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aioproxysession_connector.ssl.init_ssl") << lyramilk::kdict("不支持SSL") << std::endl;
		return false;
	}

	ssl_ctx_type aioproxysession_connector::get_ssl_ctx()
	{
		return nullptr;
	}

	bool aioproxysession_connector::open(const lyramilk::data::string& host,lyramilk::data::uint16 port,int timeout_msec)
	{
		errno = 0;
		if(fd() >= 0){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aioproxysession_connector.open") << lyramilk::kdict("打开套接字失败：%s","套接字己经打开。") << std::endl;
			return false;
		}

		hostent h;
		in_addr* inaddr;
		char __buff[8192];
		{
			hostent* phe = nullptr;
			int herrno;
			gethostbyname_r(host.c_str(),&h,__buff,sizeof(__buff),&phe,&herrno);
			if(phe == nullptr){
				lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aioproxysession_connector.open") << lyramilk::kdict("打开套接字(%s:%u)失败2：%s",host.c_str(),port,strerror(herrno)) << std::endl;
				return false;
			}
			inaddr = (in_addr*)h.h_addr;
			if(inaddr == nullptr){
				lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aioproxysession_connector.open") << lyramilk::kdict("打开套接字(%s:%u)失败3：%s",host.c_str(),port,strerror(herrno)) << std::endl;
				return false;
			}
		}

		sockaddr_in addr = {0};
		addr.sin_addr.s_addr = inaddr->s_addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);

		return open(addr,timeout_msec);
	}

	bool aioproxysession_connector::open(const sockaddr_in& saddr,int timeout_msec)
	{
		native_socket_type tmpsock = ::socket(AF_INET,SOCK_STREAM, IPPROTO_IP);
		if(tmpsock < 0){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aioproxysession_connector.open") << lyramilk::kdict("打开套接字%d失败：%s",tmpsock,strerror(errno)) << std::endl;
			return false;
		}

		unsigned int argp = 1;
		ioctl(tmpsock,FIONBIO,&argp);

		int r = ::connect(tmpsock,(const sockaddr*)&saddr,sizeof(saddr));

		if(r == -1 && errno == EINPROGRESS && (timeout_msec == -2 || check_write(tmpsock,timeout_msec))){
			this->fd(tmpsock);
			setnodelay(true);
			setkeepalive(20,3);
			return true;
		}

		lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aioproxysession_connector.open") << lyramilk::kdict("打开套接字%d失败：%s",tmpsock,strerror(errno)) << std::endl;
		::close(tmpsock);
		return false;
	}

	aioproxysession_connector::aioproxysession_connector()
	{
		endpoint = nullptr;
		flag = EPOLLIN | flag_default;
	}
	aioproxysession_connector::~aioproxysession_connector()
	{
		if(endpoint){
			endpoint->endpoint = nullptr;
			pool->destory_by_fd(endpoint->fd());
		}
	}
	bool aioproxysession_connector::init()
	{
		return true;
	}

	bool aioproxysession_connector::start_async_redirect(bool sw)
	{
		return false;
	}

	bool aioproxysession_connector::notify_in()
	{
		if(endpoint == nullptr) return false;
		// 发生读事件时，由对端的写事件驱动。
		return pool->reset(endpoint,EPOLLOUT| flag_default) && pool->reset(this,flag_default);
	}


	bool aioproxysession_connector::notify_out()
	{
		if(endpoint == nullptr) return false;
		char buff[4096];
		int i = 0;
		do{
			i = endpoint->peek(buff,sizeof(buff));
			if(i == 0) return false;
			if(i == -1){
				if(errno == EAGAIN ) break;
				if(errno == EINTR) continue;
				return false;
			}
			int r = write(buff,i);

			if(r == -1){
				if(errno == EAGAIN ) {
					// 没写完，保持本端写事件（此时对端读事件是屏蔽状态）
					return pool->reset(this,EPOLLOUT | flag_default);
				
				}
				if(errno == EINTR) continue;
				return false;
			}
			if(r > 0){
				i = endpoint->read(buff,r);
			}
		}while(i > 0);
		// 对端己经读不到数据了，此时重置两端读事件。
		return pool->reset(endpoint,EPOLLIN | flag_default) && pool->reset(this,EPOLLIN | flag_default);
	}

	bool aioproxysession_connector::notify_hup()
	{
		return false;
	}

	bool aioproxysession_connector::notify_err()
	{
		lyramilk::klog(lyramilk::log::debug,"lyramilk.netio.aioproxysession_connector.notify_err") << lyramilk::kdict("发生了ERR事件") << std::endl;
		return false;
	}

	bool aioproxysession_connector::notify_pri()
	{
		lyramilk::klog(lyramilk::log::debug,"lyramilk.netio.aioproxysession_connector.notify_pri") << lyramilk::kdict("发生了PRI事件") << std::endl;
		return false;
	}


	// aioproxysession_speedy_async
	aioproxysession_speedy_async::aioproxysession_speedy_async()
	{
		connect_status = 0;
		flag = EPOLLOUT | flag_default;
	}

	aioproxysession_speedy_async::~aioproxysession_speedy_async()
	{
	}

	bool aioproxysession_speedy_async::async_open(const lyramilk::data::string& host,lyramilk::data::uint16 port)
	{
		errno = 0;
		if(fd() >= 0){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aioproxysession_connector.open") << lyramilk::kdict("打开套接字失败：%s","套接字己经打开。") << std::endl;
			return false;
		}
		hostent h;
		in_addr* inaddr;
		char __buff[8192];
		{
			hostent* phe = nullptr;
			int herrno;
			gethostbyname_r(host.c_str(),&h,__buff,sizeof(__buff),&phe,&herrno);
			if(phe == nullptr){
				lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aioproxysession_connector.open") << lyramilk::kdict("打开套接字(%s:%u)失败2：%s",host.c_str(),port,strerror(herrno)) << std::endl;
				return false;
			}
			inaddr = (in_addr*)h.h_addr;
			if(inaddr == nullptr){
				lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aioproxysession_connector.open") << lyramilk::kdict("打开套接字(%s:%u)失败3：%s",host.c_str(),port,strerror(herrno)) << std::endl;
				return false;
			}
		}

		sockaddr_in addr = {0};
		addr.sin_addr.s_addr = inaddr->s_addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);

		return async_open(addr);
	}

	bool aioproxysession_speedy_async::async_open(const sockaddr_in& saddr)
	{
		native_socket_type tmpsock = ::socket(AF_INET,SOCK_STREAM, IPPROTO_IP);
		if(tmpsock < 0){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aioproxysession_connector.open") << lyramilk::kdict("打开套接字%d失败：%s",tmpsock,strerror(errno)) << std::endl;
			return false;
		}

		unsigned int argp = 1;
		ioctl(tmpsock,FIONBIO,&argp);

		int r = ::connect(tmpsock,(const sockaddr*)&saddr,sizeof(saddr));

		if(r == -1 && errno == EINPROGRESS){
			connect_status = 1;
			this->fd(tmpsock);
			setnodelay(true);
			setkeepalive(20,3);
			return true;
		}

		lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aioproxysession_connector.open") << lyramilk::kdict("打开套接字%d失败：%s",tmpsock,strerror(errno)) << std::endl;
		::close(tmpsock);
		return false;
	}

	bool aioproxysession_speedy_async::oninit(lyramilk::data::ostream& os)
	{
		return true;
	}

	void aioproxysession_speedy_async::onfinally(lyramilk::data::ostream& os)
	{
	}

	bool aioproxysession_speedy_async::onrequest(const char* cache, int size, int* sizeused, lyramilk::data::ostream& os)
	{
		//不应该调用到这个函数。
		return false;
	}

	bool aioproxysession_speedy_async::notify_in()
	{
		if(connect_status == 3) return aioproxysession_connector::notify_in();
		char buff[4096];
		int i = 0;
		do{
			i = peek(buff,sizeof(buff));
			if(i == 0) return false;
			if(i == -1){
				if(errno == EAGAIN ) break;
				if(errno == EINTR) continue;
				return false;
			}
			assert(i > 0);

			int bytesused = i;
			if(!onrequest(buff,i,&bytesused,aos)){
				aos.flush();
				return false;
			}
			if(bytesused > i){
				return false;
			}
			if(bytesused > 0){
				read(buff,bytesused);
			}
			aos.flush();
		}while(false);

		return pool->reset(this,flag);
	}

	bool aioproxysession_speedy_async::notify_out()
	{
		if(connect_status == 3) return aioproxysession_connector::notify_out();
		if(connect_status == 1){
			connect_status = 2;
			aos.init(this);
			if(oninit(aos)){
				aos.flush();
				flag = EPOLLIN | flag_default;
				if(!aos.good()) return false;
				return pool->reset(this,flag);
			}
			return false;
		}

		return false;
	}

	bool aioproxysession_speedy_async::enable_async_redirect(bool sw)
	{
		if(sw){
			connect_status = 3;
			endpoint->start_async_redirect(sw);
			return true;
		}else{
			connect_status = 3;
			endpoint->start_async_redirect(sw);
			return true;
		}
	}

	void aioproxysession_speedy_async::ondestory()
	{
		onfinally(aos);
		aiosession::ondestory();
	}

	// aioproxysession
	aioproxysession::aioproxysession()
	{
		directmode = false;
	}

	aioproxysession::~aioproxysession()
	{
		start_async_redirect(false);
		//directmode = false;
	}

	bool aioproxysession::notify_in()
	{
		if(directmode) return aioproxysession_connector::notify_in();
		char buff[4096];
		int i = 0;
		do{
			i = peek(buff,sizeof(buff));
			if(i == 0) return false;
			if(i == -1){
				if(errno == EAGAIN ) break;
				if(errno == EINTR) continue;
				return false;
			}
			assert(i > 0);

			int bytesused = i;

			if(!onrequest(buff,i,&bytesused,aos)){
				aos.flush();
				return false;
			}
			if(bytesused > i){
				return false;
			}
			if(bytesused > 0){
				read(buff,bytesused);
			}
			aos.flush();
		}while(false);

		return pool->reset(this,flag);
	}

	bool aioproxysession::init()
	{
		aos.init(this);
		return oninit(aos);
	}

	void aioproxysession::ondestory()
	{
		onfinally(aos);
		aiosession::ondestory();
	}
	
	bool aioproxysession::oninit(lyramilk::data::ostream& os)
	{
		return true;
	}

	void aioproxysession::onfinally(lyramilk::data::ostream& os)
	{
	}

	bool aioproxysession::async_redirect_to(const lyramilk::data::string& host,lyramilk::data::uint16 port)
	{
		if(endpoint) return false;
		endpoint = lyramilk::netio::aiosession::__tbuilder<aioproxysession_connector>();

		if(endpoint->open(host,port,200)){
			endpoint->endpoint = this;
			setnodelay(true);
			setkeepalive(20,3);

			endpoint->flag = EPOLLIN | flag_default;

			//不需要加 start_async_redirect(true),因为原生的 aioproxysession_connector 默认就是透传。
			lyramilk::io::aiopoll_safe* pool = (lyramilk::io::aiopoll_safe*)this->pool;
			if(start_async_redirect(true) && pool->add_to_thread(get_thread_idx(),endpoint,-1)){
				return true;
			}
			start_async_redirect(false);
		}
		endpoint->destory();
		endpoint = nullptr;
		return false;
	}

	bool aioproxysession::async_redirect_to(const sockaddr_in& saddr)
	{
		if(endpoint) return false;
		endpoint = lyramilk::netio::aiosession::__tbuilder<aioproxysession_connector>();

		if(endpoint->open(saddr,200)){
			endpoint->endpoint = this;
			setnodelay(true);
			setkeepalive(20,3);

			endpoint->flag = EPOLLIN | flag_default;

			//不需要加 start_async_redirect(true),因为原生的 aioproxysession_connector 默认就是透传。
			lyramilk::io::aiopoll_safe* pool = (lyramilk::io::aiopoll_safe*)this->pool;
			if(start_async_redirect(true) && pool->add_to_thread(get_thread_idx(),endpoint,-1)){
				return true;
			}
			start_async_redirect(false);
		}
		endpoint->destory();
		endpoint = nullptr;
		return false;
	}

	bool aioproxysession::async_redirect_to(aioproxysession_connector* dest)
	{
		if(endpoint) return false;
		endpoint = dest;
		endpoint->endpoint = this;
		setnodelay(true);
		setkeepalive(20,3);

		endpoint->flag = EPOLLIN | flag_default;

		//因为 dest 有可能是被继承的,所以需要通过async_redirect_connect(true)切换到透传模式。
		lyramilk::io::aiopoll_safe* pool = (lyramilk::io::aiopoll_safe*)this->pool;
		if(start_async_redirect(true) && endpoint->start_async_redirect(true) && pool->add_to_thread(get_thread_idx(),endpoint,-1)){
			return true;
		}
		start_async_redirect(false);

		endpoint = nullptr;
		dest->endpoint = nullptr;
		return false;
	}

	bool aioproxysession::async_redirect_connect(aioproxysession_connector* dest)
	{
		if(endpoint) return false;
		endpoint = dest;
		endpoint->endpoint = this;
		setnodelay(true);
		setkeepalive(20,3);

		lyramilk::io::aiopoll_safe* pool = (lyramilk::io::aiopoll_safe*)this->pool;
		if(pool->add_to_thread(get_thread_idx(),endpoint,-1)){
			return true;
		}
		endpoint = nullptr;
		dest->endpoint = nullptr;
		return false;
	}


	bool aioproxysession::start_async_redirect(bool sw)
	{
		directmode = sw;
		flag = EPOLLIN | flag_default;
		//return pool->reset(this,flag);
		return true;
	}
}}
