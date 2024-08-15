#include <libmilk/scriptengine.h>
#include <libmilk/log.h>
#include <libmilk/dict.h>
#include <libmilk/netaio.h>
#include <libmilk/netproxy.h>
#include <libmilk/setproctitle.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <sys/wait.h>


namespace lyramilk{ namespace teapoy{ namespace native
{
	lyramilk::log::logss static klog;
	lyramilk::log::logss static log(klog,"teapoy.native.rproxy");
	const char RPROXY_MAGIC[] = "28f988dd-1dea-4df9-a6fd-c2fb8bfb2cab";

	class rproxy_aioserver;


	// 服务端业务会话
	class rproxyserver_aiosession:public lyramilk::netio::aioproxysession
	{
	  public:
		unsigned int thread_idx;

		rproxy_aioserver* pubserver;
		rproxyserver_aiosession()
		{
			flag = EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLONESHOT;
		}

	  	virtual ~rproxyserver_aiosession();

		virtual bool oninit(lyramilk::data::ostream& os);
		virtual bool onrequest(const char* cache, int size,int* bytesused, lyramilk::data::ostream& os);

	};

	// 服务端业务端口
	class rproxy_aioserver:public lyramilk::netio::aioserver<rproxyserver_aiosession>
	{
		std::set<lyramilk::netio::socket*> masters;
		lyramilk::threading::mutex_rw lock;
		lyramilk::threading::mutex_os session_lock;

		std::set<rproxyserver_aiosession*> sessions;

	  public:
		virtual ~rproxy_aioserver()
		{
		}

		virtual lyramilk::netio::aiosession* create()
		{
			rproxyserver_aiosession* client_session = lyramilk::netio::aiosession::__tbuilder<rproxyserver_aiosession>();
			client_session->pubserver = this;
			return client_session;
		}

		virtual bool add_master(lyramilk::netio::socket* mgr)
		{
			lyramilk::threading::mutex_sync _(lock.w());
			masters.insert(mgr);
			return true;
		}

		virtual bool remove_master(lyramilk::netio::socket* mgr)
		{
			lyramilk::threading::mutex_sync _(lock.w());
			std::set<lyramilk::netio::socket*>::iterator it = masters.find(mgr);
			if(it!=masters.end()){
				masters.erase(it);
				return true;
			}
			return false;
		}

		virtual bool add_session(rproxyserver_aiosession* session)
		{
			lyramilk::threading::mutex_sync _(session_lock);
			sessions.insert(session);
			return true;
		}

		virtual bool remove_session(rproxyserver_aiosession* session)
		{
			lyramilk::threading::mutex_sync _(session_lock);
			std::set<rproxyserver_aiosession*>::iterator it = sessions.find(session);
			if(it!=sessions.end()){
				sessions.erase(it);
				return true;
			}
			return false;
		}
		virtual bool accept_new_session(rproxyserver_aiosession* p)
		{
			if(!add_session(p)) return false;


			lyramilk::threading::mutex_sync _(lock.r());
			std::set<lyramilk::netio::socket*>::iterator it = masters.begin();
			if(it == masters.end()){
				remove_session(p);
				return false;
			}

			lyramilk::netio::socket* mgr = *it;
			unsigned long long lptr = (unsigned long long)reinterpret_cast<void*>(p);
			mgr->write((const char*)&lptr,sizeof(lptr));
			return true;
		}
	};


	//	rproxyserver_aiosession
	bool rproxyserver_aiosession::oninit(lyramilk::data::ostream& os)
	{
		thread_idx = pool->get_thread_idx();
		return pubserver->accept_new_session(this);
	}

	rproxyserver_aiosession::~rproxyserver_aiosession()
	{
		pubserver->remove_session(this);
	}


	bool rproxyserver_aiosession::onrequest(const char* cache, int size,int* bytesused, lyramilk::data::ostream& os)
	{
		*bytesused = size;
		//*bytesused = 0;
		return true;
	}

	//	管理端会话
	class rproxyserver_aiosession_master:public lyramilk::netio::aioproxysession
	{
		bool is_upstream_master;
	  public:
		rproxy_aioserver* pubserver;


		rproxyserver_aiosession_master()
		{
			is_upstream_master = false;
		}

	  	virtual ~rproxyserver_aiosession_master()
		{
			if(is_upstream_master){
				pubserver->remove_master(this);
			}else{
			}
		}

		virtual bool onrequest(const char* cache, int size,int* bytesused, lyramilk::data::ostream& os)
		{
			if((unsigned int)size >= sizeof(RPROXY_MAGIC) && memcmp(cache,RPROXY_MAGIC,sizeof(RPROXY_MAGIC)) == 0){
				// 管理会话
				is_upstream_master = true;
				*bytesused = sizeof(RPROXY_MAGIC);
				pubserver->add_master(this);
				return true;
			}

			//业务会话
			if(size >= 8){
				*bytesused = 8;
				unsigned long long lptr = *(unsigned long long*)cache;
				rproxyserver_aiosession* client_session = reinterpret_cast<rproxyserver_aiosession*>(lptr);
				if(pubserver->remove_session(client_session)){
					pool->detach(client_session);
					errno = 0;
					async_redirect_to(client_session);
				}else{
					log(lyramilk::log::error,"onrequest") << lyramilk::kdict("remove失败") << std::endl;
					return false;
				}
			}else if(size >= 4){
				*bytesused = 4;
				//"PING"
			}

			return true;
		}

	};

	// 管理端服务
	class rproxy_aioserver_master:public lyramilk::netio::aioserver<rproxyserver_aiosession_master>
	{
		rproxy_aioserver* pubserver;
		lyramilk::data::string upstream_host;
		unsigned short upstream_port;
	  public:
		virtual bool attach(const lyramilk::data::string& host,unsigned short port,rproxy_aioserver* srv)
		{
			upstream_host = host;
			upstream_port = port;
			pubserver = srv;
			return pubserver != nullptr;
		}
		virtual lyramilk::netio::aiosession* create()
		{
			rproxyserver_aiosession_master* master = lyramilk::netio::aiosession::__tbuilder<rproxyserver_aiosession_master>();
			master->pubserver = pubserver;
			return master;
		}
	};



	// 服务端转发会话
	class rproxy_aioclient:public lyramilk::netio::aioproxysession_connector
	{
		lyramilk::data::string server_host;
		unsigned short server_port;
	  public:
		rproxy_aioclient()
		{}
	  	virtual ~rproxy_aioclient()
		{}

		virtual bool open(const lyramilk::data::string& host,unsigned short port)
		{
			server_host = host;
			server_port = port;

			if(fd() >= 0){
				log(lyramilk::log::error,"open") << lyramilk::kdict("打开套接字(%s:%u)失败：%s",host.c_str(),port,"套接字己经打开。") << std::endl;
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
					lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aiolistener.open") << lyramilk::kdict("打开套接字(%s:%u)失败2：%s",host.c_str(),port,strerror(herrno)) << std::endl;
					return false;
				}
				inaddr = (in_addr*)h.h_addr;
				if(inaddr == nullptr){
					lyramilk::klog(lyramilk::log::error,"lyramilk.netio.aiolistener.open") << lyramilk::kdict("打开套接字(%s:%u)失败3：%s",host.c_str(),port,strerror(herrno)) << std::endl;
					return false;
				}
			}

			int tmpsock = ::socket(AF_INET,SOCK_STREAM, IPPROTO_IP);
			if(tmpsock < 0){
				log(lyramilk::log::error,"open") << lyramilk::kdict("打开套接字(%s:%u)失败：%s",host.c_str(),port,strerror(errno)) << std::endl;
				return false;
			}

			sockaddr_in addr = {0};
			addr.sin_addr.s_addr = inaddr->s_addr;
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);

			if(0 == ::connect(tmpsock,(const sockaddr*)&addr,sizeof(addr))){
				this->fd(tmpsock);
				return true;
			}
			log(lyramilk::log::error,"open") << lyramilk::kdict("打开套接字(%s:%u)失败：%s",host.c_str(),port,strerror(errno)) << std::endl;
			::close(tmpsock);
			return false;
		}
	};

	class rproxy_aiocmdclient:public lyramilk::netio::client
	//class rproxy_aiocmdclient:public lyramilk::netio::aioproxysession_speedy_async
	{
	  public:
		lyramilk::io::aiopoll_safe* pool;
		lyramilk::data::string server_host;
		unsigned short server_port;
		lyramilk::data::string upstream_host;
		unsigned short upstream_port;
	  public:
		virtual bool init(const lyramilk::data::string& host,unsigned short port,const lyramilk::data::string& uhost,unsigned short uport)
		{
			server_host = host;
			server_port = port;
			upstream_host = uhost;
			upstream_port = uport;
			return true;
		}
		virtual bool reopen()
		{
			if(!isalive()){
				if(fd() >= 0){
					close();
					log(lyramilk::log::error,"rproxy.middle.open") << lyramilk::kdict("打开套接字(%s:%u)失败：%s",server_host.c_str(),server_port,"套接字己经打开。") << std::endl;
				}
			}

			hostent h;
			in_addr* inaddr;
			char __buff[8192];

			{
				hostent* phe = nullptr;
				int herrno;
				gethostbyname_r(server_host.c_str(),&h,__buff,sizeof(__buff),&phe,&herrno);
				if(phe == nullptr){
					lyramilk::klog(lyramilk::log::error,"rproxy.middle.open") << lyramilk::kdict("打开套接字(%s:%u)失败2：%s",server_host.c_str(),server_port,strerror(herrno)) << std::endl;
					return false;
				}
				inaddr = (in_addr*)h.h_addr;
				if(inaddr == nullptr){
					lyramilk::klog(lyramilk::log::error,"rproxy.middle.open") << lyramilk::kdict("打开套接字(%s:%u)失败3：%s",server_host.c_str(),server_port,strerror(herrno)) << std::endl;
					return false;
				}
			}

			int tmpsock = ::socket(AF_INET,SOCK_STREAM, IPPROTO_IP);
			if(tmpsock < 0){
				log(lyramilk::log::error,"rproxy.middle.open") << lyramilk::kdict("打开套接字(%s:%u)失败：%s",server_host.c_str(),server_port,strerror(errno)) << std::endl;
				return false;
			}

			sockaddr_in addr = {0};
			addr.sin_addr.s_addr = inaddr->s_addr;
			addr.sin_family = AF_INET;
			addr.sin_port = htons(server_port);


			if(0 == ::connect(tmpsock,(const sockaddr*)&addr,sizeof(addr))){
				this->fd(tmpsock);
				return true;
			}
			log(lyramilk::log::error,"rproxy.middle.open") << lyramilk::kdict("打开套接字(%s:%u)失败：%s",server_host.c_str(),server_port,strerror(errno)) << std::endl;
			::close(tmpsock);
			return false;
		}





		static void* thread_task(void* _p)
		{
			rproxy_aiocmdclient* ins = (rproxy_aiocmdclient*)_p;


			while(!ins->reopen()){
				sleep(2);
			}

			lyramilk::netio::netaddress nad = ins->dest();

			while(true){
				while(!ins->reopen()){
					sleep(2);
				}

				ins->write(RPROXY_MAGIC,sizeof(RPROXY_MAGIC));

				while(true){
					if(!ins->check_read(10000)){
						if(ins->write("PING",4) != 4){
							ins->close();
							log(lyramilk::log::error,"rproxy_aiocmdclient.thread_task") << lyramilk::kdict("check_read:连接失败 %s:%u %s",nad.host().c_str(),nad.port(),strerror(errno)) << std::endl;
							break;
						}
						continue;
					}

					char cache[8];
					int size = ins->read(cache,sizeof(cache));
					if(size!=8){
						log(lyramilk::log::error,"rproxy_aiocmdclient.thread_task") << lyramilk::kdict("连接失败1:size=%d,err=%s",size,strerror(errno)) << std::endl;
						ins->close();
						break;
					}
					rproxy_aioclient* c = rproxy_aioclient::__tbuilder<rproxy_aioclient>();

					//lyramilk::netio::aioproxysession_speedy_async* c = rproxy_aioclient::__tbuilder<lyramilk::netio::aioproxysession_speedy_async>();

					if(!c->open(ins->server_host.c_str(),ins->server_port)){
						log(lyramilk::log::error,"rproxy_aiocmdclient.thread_task") << lyramilk::kdict("连接失败2:size=%d,err=%s",size,strerror(errno)) << std::endl;
						delete c;
						continue;
					}
					c->setnodelay(true);
					c->setkeepalive(20,3);
					c->setnoblock(true);

					c->write(cache,size);
					rproxy_aioclient* uc = rproxy_aioclient::__tbuilder<rproxy_aioclient>();
					//lyramilk::netio::aioproxysession_speedy_async* uc = rproxy_aioclient::__tbuilder<lyramilk::netio::aioproxysession_speedy_async>();
					if(!uc->open(ins->upstream_host.c_str(),ins->upstream_port)){
						log(lyramilk::log::error,"rproxy_aiocmdclient.thread_task") << lyramilk::kdict("连接失败3:size=%d,err=%s",size,strerror(errno)) << std::endl;
						delete uc;
						delete c;
						continue;
					}
					uc->setnodelay(true);
					uc->setkeepalive(20,3);
					uc->setnoblock(true);
					

					c->endpoint = uc;
					uc->endpoint = c;

					log(lyramilk::log::debug,"rproxy_aiocmdclient.thread_task") << lyramilk::kdict("连接成功:%s:%u --> %s:%u",ins->server_host.c_str(),ins->server_port,ins->upstream_host.c_str(),ins->upstream_port) << std::endl;

					ins->pool->add(uc,-1);
					ins->pool->add_to_thread(uc->get_thread_idx(),c,-1);
				}
			}

			pthread_exit(0);
			return nullptr;
		}
	};

}}}




void useage(lyramilk::data::string selfname)
{
	std::cout << "useage:" << selfname << " [optional]" << std::endl;
	std::cout << "\t-h <host>\t" << "管理地址" << std::endl;
	std::cout << "\t-p <port>\t" << "管理端口" << std::endl;
	std::cout << "\t-m <host>\t" << "目标地址" << std::endl;
	std::cout << "\t-a <port>\t" << "目标端口" << std::endl;
	std::cout << "\t-t <0 or 1>\t" << "类型：0 管理端，1 工作端" << std::endl;
	std::cout << "\t-d \t" << " 后台运行" << std::endl;
}



int main(int argc,char* argv[])
{
	lyramilk::init_setproctitle(argc,(const char**&)argv);
	
	bool setdaemon = false;
	bool isdaemon = false;
	signal(SIGPIPE, SIG_IGN);

	lyramilk::data::string man_host;
	lyramilk::data::int64 man_port = -1;
	lyramilk::data::string mid_host;
	lyramilk::data::int64 mid_port = -1;
	lyramilk::data::int64 type = -1;
	{
		lyramilk::data::string selfname = argv[0];
		int oc;
		while((oc = getopt(argc, argv, "h:p:m:a:t:d?")) != -1){
			switch(oc)
			{
			  case 'h':
				man_host = optarg;
				break;
			  case 'p':
				man_port = strtoll(optarg,NULL,10);
				break;
			  case 'm':
				mid_host = optarg;
				break;
			  case 'a':
				mid_port = strtoll(optarg,NULL,10);
				break;
			  case 't':
				type = strtoll(optarg,NULL,10);
				break;
			  case 'd':
				setdaemon = true;
				break;
			  case '?':
			  default:
				useage(selfname);
				return 0;
			}
		}
		if(man_port == -1 || mid_port == -1 || type == -1){
			useage(selfname);
			return 0;
		}
	}
	if(type == 1){
		lyramilk::setproctitle("rproxy [guard]");
	}

	
	if(setdaemon){
		::daemon(1,0);
		int pid = 0;
		do{
			pid = fork();
			if(pid == 0){
				if(type == 1){
					lyramilk::setproctitle("rproxy [workder]");
				}else{
					lyramilk::setproctitle("rproxy [service]");
				}
				isdaemon = true;
				break;
			}
			sleep(1);
		}while(waitpid(pid,NULL,0));
	}else{
		isdaemon = getppid() == 1;
		if(type == 1){
			lyramilk::setproctitle("rproxy [*workder]");
		}else{
			lyramilk::setproctitle("rproxy [*service]");
		}
	}
	
	
	lyramilk::io::aiopoll_safe af(8);
	af.active();
	if(type == 0){
		// 管理端
		static lyramilk::teapoy::native::rproxy_aioserver ins;
		static lyramilk::teapoy::native::rproxy_aioserver_master master;
		
		if(man_host.empty()){
			if(!master.open(man_port)){
			lyramilk::teapoy::native::log(lyramilk::log::error,"rproxy.init") << __LINE__ << std::endl;
				return 0 - __LINE__;
			}
			lyramilk::teapoy::native::log(lyramilk::log::debug,"rproxy.init") << lyramilk::kdict("管理端监听:%u",man_port) << std::endl;
			if(!master.attach("0.0.0.0",man_port,&ins)){
			lyramilk::teapoy::native::log(lyramilk::log::error,"rproxy.init") << __LINE__ << std::endl;
				return 0 - __LINE__;
			}
		}else{
			if(!master.open(man_host.c_str(),man_port)){
			lyramilk::teapoy::native::log(lyramilk::log::error,"rproxy.init") << __LINE__ << std::endl;
				return 0 - __LINE__;
			}
			lyramilk::teapoy::native::log(lyramilk::log::debug,"rproxy.init") << lyramilk::kdict("管理端监听:%s:%u",man_host.c_str(),man_port) << std::endl;
			if(!master.attach(man_host.c_str(),man_port,&ins)){
			lyramilk::teapoy::native::log(lyramilk::log::error,"rproxy.init") << __LINE__ << std::endl;
				return 0 - __LINE__;
			}
		}
		
		if(mid_host.empty()){
			if(!ins.open(mid_port)){
			lyramilk::teapoy::native::log(lyramilk::log::error,"rproxy.init") << __LINE__ << std::endl;
				return 0 - __LINE__;
			}
			lyramilk::teapoy::native::log(lyramilk::log::debug,"rproxy.init") << lyramilk::kdict("工作端监听:%u",mid_port) << std::endl;
		}else{
			if(!ins.open(mid_host.c_str(),mid_port)){
			lyramilk::teapoy::native::log(lyramilk::log::error,"rproxy.init") << __LINE__ << std::endl;
				return 0 - __LINE__;
			}
			lyramilk::teapoy::native::log(lyramilk::log::debug,"rproxy.init") << lyramilk::kdict("工作端监听:%s:%u",mid_host.c_str(),mid_port) << std::endl;
		}
		if(!af.add(&ins)){
			lyramilk::teapoy::native::log(lyramilk::log::error,"rproxy.init") << __LINE__ << std::endl;
			return 0 - __LINE__;
		}
		if(!af.add(&master)){
			lyramilk::teapoy::native::log(lyramilk::log::error,"rproxy.init") << __LINE__ << std::endl;
			return 0 - __LINE__;
		}
	}else if(type == 1){
		// 工作端
		static lyramilk::teapoy::native::rproxy_aiocmdclient ins;
		if(!ins.init(man_host.c_str(),man_port,mid_host.c_str(),mid_port)){
			lyramilk::teapoy::native::log(lyramilk::log::error,"rproxy.init") << __LINE__ << std::endl;
			return 0 - __LINE__;
		}
		ins.pool = &af;
		pthread_t id_1;
		pthread_create(&id_1,NULL,lyramilk::teapoy::native::rproxy_aiocmdclient::thread_task,&ins);
		pthread_detach(id_1);
	}

	while(true){
		sleep(10);
	}
	std::cout << "helo world" << std::endl;
	return 0;
}


