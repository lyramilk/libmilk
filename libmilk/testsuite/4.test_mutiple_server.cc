#include <iostream>
#include "netaio.h"
#include "var.h"
#include "log.h"
#include <signal.h>

#define MACRO_ES2

//#undef MACRO_ES2


#ifdef MACRO_ES2
class server_session:public lyramilk::netio::aiosession_async
#else
class server_session:public lyramilk::netio::aiosession_sync
#endif
{
  protected:
	lyramilk::log::logss log;
  public:
	bool oninit()
	{
		return true;
	}

	//void onfinally()

	bool onrequest(const char* cache,int size,std::ostream& os)
	{
		/*
std::cout << "请求" << std::endl;
std::cout.write(cache,size);
std::cout << std::endl;*/
		lyramilk::data::string q;

#ifdef MACRO_ES2
		q = "<html><head><title>HTTP测试</title></head><body>HTTP测试</body></html>";
#else
		if(ssl()){
			q = "<html><head><title>HTTPS测试</title></head><body>HTTPS测试</body></html>";
		}else{
			q = "<html><head><title>HTTP测试</title></head><body>HTTP测试</body></html>";
		}
#endif
/*
std::cout << "响应正文长度" << q.size() << std::endl;
std::cout << "HTTP/1.1 200 OK\r\nServer:libmilk/1.0\r\nContent-Type:text/html;charset=utf-8\r\nContent-Length:";
std::cout << q.size();
std::cout << "\r\n\r\n";
std::cout << q;
std::cout << std::endl;*/
		os << "HTTP/1.1 200 OK\r\nServer:libmilk/1.0\r\nContent-Type:text/html;charset=utf-8\r\nContent-Length:";
		os << q.size() + 1;
		os << "\r\n\r\n";
		os << q;
		os.put('X');
		return false;
	}

	//void write(const unsigned char* cache,int size)

	server_session() :log(lyramilk::klog,"server.session")
	{
	}
	virtual ~server_session()
	{
	}
};

int main(int argc,const char* argv[])
{
	signal(SIGPIPE, SIG_IGN);
#ifdef MACRO_ES2
	lyramilk::io::aiopoll aip;
	lyramilk::netio::aioserver<server_session> ais2;
	ais2.open("127.0.0.1",80);
	aip.add(&ais2);

	lyramilk::netio::aioserver<server_session> ais1;
	ais1.open(443);
	ais1.init_ssl("/root/ssl-keygen/test.crt","/root/ssl-keygen/test.key");
	aip.add(&ais1);

	aip.active(100);
#else
	lyramilk::io::aiopoll aip;
	lyramilk::netio::aioserver<server_session> ais2;
	ais2.open(80);
	aip.add(&ais2);

	lyramilk::netio::aioserver<server_session> ais1;
	ais1.open(443);
	ais1.init_ssl("/root/ssl-keygen/test.crt","/root/ssl-keygen/test.key");
	aip.add(&ais1);

	aip.active(4);
#endif

	int i = 0;
	std::cin >> i;
	return 0;
}
