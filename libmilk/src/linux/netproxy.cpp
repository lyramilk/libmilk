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


	// aioproxysession_speedy

	const unsigned int flag_default = EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLONESHOT;
	//const unsigned int flag_default = EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLET;
	

	bool aioproxysession_speedy::ssl()
	{
		return false;
	}

	void aioproxysession_speedy::ssl(bool use_ssl)
	{
	}

	bool aioproxysession_speedy::init_ssl(const lyramilk::data::string& certfilename, const lyramilk::data::string& keyfilename)
	{
		lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aioproxysession_speedy.ssl.init_ssl") << lyramilk::kdict("不支持SSL") << std::endl;
		return false;
	}

	ssl_ctx_type aioproxysession_speedy::get_ssl_ctx()
	{
		return nullptr;
	}

	bool aioproxysession_speedy::open(const lyramilk::data::string& host,lyramilk::data::uint16 port,int timeout_msec)
	{
		errno = 0;
		if(fd() >= 0){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aioproxysession_speedy.open") << lyramilk::kdict("打开套接字失败：%s","套接字己经打开。") << std::endl;
			return false;
		}
		hostent* h = gethostbyname(host.c_str());
		if(h == nullptr){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aioproxysession_speedy.open") << lyramilk::kdict("获取%s的IP地址失败：%p,%s",host.c_str(),h,strerror(errno)) << std::endl;
			return false;
		}

		in_addr* inaddr = (in_addr*)h->h_addr;
		if(inaddr == nullptr){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aioproxysession_speedy.open") << lyramilk::kdict("获取%s的IP地址失败：%p,%s",host.c_str(),inaddr,strerror(errno)) << std::endl;
			return false;
		}

		sockaddr_in addr = {0};
		addr.sin_addr.s_addr = inaddr->s_addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);

		return open(addr,timeout_msec);
	}

	bool aioproxysession_speedy::open(const sockaddr_in& saddr,int timeout_msec)
	{
		native_socket_type tmpsock = ::socket(AF_INET,SOCK_STREAM, IPPROTO_IP);
		if(tmpsock < 0){
			lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aioproxysession_speedy.open") << lyramilk::kdict("打开套接字%d失败：%s",tmpsock,strerror(errno)) << std::endl;
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

		lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aioproxysession_speedy.open") << lyramilk::kdict("打开套接字%d失败：%s",tmpsock,strerror(errno)) << std::endl;
		::close(tmpsock);
		return false;
	}

	aioproxysession_speedy::aioproxysession_speedy()
	{
		endpoint = nullptr;
		flag = EPOLLIN | flag_default;
	}
	aioproxysession_speedy::~aioproxysession_speedy()
	{
		if(endpoint){
			endpoint->endpoint = nullptr;
			pool->remove(endpoint);
		}
	}
	bool aioproxysession_speedy::init()
	{
		return true;
	}

	bool aioproxysession_speedy::combine(aioproxysession_speedy* endpoint)
	{
		if(endpoint == nullptr) return false;

		this->endpoint = endpoint;
		endpoint->endpoint = this;


		lyramilk::io::aiopoll_safe* pool = (lyramilk::io::aiopoll_safe*)this->pool;
		if(pool->add_to_thread(get_thread_idx(),endpoint,EPOLLIN | flag_default)){
			return true;
		}

		this->endpoint = nullptr;
		endpoint->endpoint = nullptr;
		return false;
	}

	bool aioproxysession_speedy::notify_in()
	{
		if(endpoint == nullptr) return false;
		// 发生读事件时，由对端的写事件驱动。
		return pool->reset(endpoint,EPOLLOUT| flag_default) && pool->reset(this,flag_default);
	}


	bool aioproxysession_speedy::notify_out()
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

	bool aioproxysession_speedy::notify_hup()
	{
		return false;
	}

	bool aioproxysession_speedy::notify_err()
	{
		lyramilk::klog(lyramilk::log::debug,"lyramilk.netio.aioproxysession_speedy.notify_err") << lyramilk::kdict("发生了ERR事件") << std::endl;
		return false;
	}

	bool aioproxysession_speedy::notify_pri()
	{
		lyramilk::klog(lyramilk::log::debug,"lyramilk.netio.aioproxysession_speedy.notify_pri") << lyramilk::kdict("发生了PRI事件") << std::endl;
		return false;
	}

	// aioproxysession
	aioproxysession::aioproxysession()
	{
		directmode = false;
	}

	aioproxysession::~aioproxysession()
	{
		stop_proxy();
	}

	bool aioproxysession::notify_in()
	{
		if(directmode) return aioproxysession_speedy::notify_in();
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

	bool aioproxysession::notify_out()
	{
		if(directmode) return aioproxysession_speedy::notify_out();
		lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aioproxysession.notify_out") << lyramilk::kdict("%s方法未实现","notify_out") << std::endl;
		return false;
	}

	bool aioproxysession::notify_hup()
	{
		if(directmode) return aioproxysession_speedy::notify_hup();
		return false;
	}

	bool aioproxysession::notify_err()
	{
		if(directmode) return aioproxysession_speedy::notify_err();
		lyramilk::klog(lyramilk::log::debug,"lyramilk.netio.aioproxysession.notify_err") << lyramilk::kdict("发生了ERR事件") << std::endl;
		return false;
	}

	bool aioproxysession::notify_pri()
	{
		if(directmode) return aioproxysession_speedy::notify_pri();
		lyramilk::klog(lyramilk::log::debug,"lyramilk.netio.aioproxysession.notify_pri") << lyramilk::kdict("发生了PRI事件") << std::endl;
		return false;
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

	bool aioproxysession::combine(const lyramilk::data::string& host,lyramilk::data::uint16 port)
	{
		if(endpoint) return false;
		endpoint = lyramilk::netio::aiosession::__tbuilder<aioproxysession_speedy>();

		if(endpoint->open(host,port,200)){
			endpoint->endpoint = this;
			setnodelay(true);
			setkeepalive(20,3);

			endpoint->flag = EPOLLIN | flag_default;

			lyramilk::io::aiopoll_safe* pool = (lyramilk::io::aiopoll_safe*)this->pool;
			if(start_proxy() && pool->add_to_thread(get_thread_idx(),endpoint,-1)){
				return true;
			}
			stop_proxy();
		}
		endpoint->destory();
		endpoint = nullptr;
		return false;
	}

	bool aioproxysession::combine(const sockaddr_in& saddr)
	{
		if(endpoint) return false;
		endpoint = lyramilk::netio::aiosession::__tbuilder<aioproxysession_speedy>();

		if(endpoint->open(saddr,200)){
			endpoint->endpoint = this;
			setnodelay(true);
			setkeepalive(20,3);

			endpoint->flag = EPOLLIN | flag_default;

			lyramilk::io::aiopoll_safe* pool = (lyramilk::io::aiopoll_safe*)this->pool;
			if(start_proxy() && pool->add_to_thread(get_thread_idx(),endpoint,-1)){
				return true;
			}
			stop_proxy();
		}
		endpoint->destory();
		endpoint = nullptr;
		return false;
	}

	bool aioproxysession::combine(aioproxysession_speedy* dest)
	{
		if(endpoint) return false;
		endpoint = dest;
		endpoint->endpoint = this;
		setnodelay(true);
		setkeepalive(20,3);

		endpoint->flag = EPOLLIN | flag_default;

		lyramilk::io::aiopoll_safe* pool = (lyramilk::io::aiopoll_safe*)this->pool;
		if(start_proxy() && pool->add_to_thread(get_thread_idx(),endpoint,-1)){
			return true;
		}
		endpoint = nullptr;
		dest->endpoint = nullptr;
		return false;
	}

	bool aioproxysession::start_proxy()
	{
		directmode = true;
		return true;
	}

	bool aioproxysession::stop_proxy()
	{
		directmode = false;
		return true;
	}


}}
