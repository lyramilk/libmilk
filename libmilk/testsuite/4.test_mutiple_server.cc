#include <iostream>
#include "netaio.h"
#include "var.h"
#include "log.h"

class server_session:public lyramilk::netio::aiosession
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
std::cout.write(cache,size);
std::cout << std::endl;
		lyramilk::data::string s = "HTTP/1.1 200 OK\r\nServer:libmilk/1.0\r\nContent-Type:text/html;charset=utf-8\r\nContent-Length:89\r\n\r\n";
		lyramilk::data::string q;
		if(ssl()){
			q = "<html><head><title>lyramilk的个人主页SSL</title></head><body>HTTPS测试</body></html>";
		}else{
			q = "<html><head><title>lyramilk的个人主页SSL</title></head><body>HTTP测试</body></html>";
		}
		s += q;
std::cout << "正文长度" << q.size() << std::endl;
std::cout.write(s.c_str(),s.size());
std::cout << std::endl;

		os.write(s.c_str(),s.size());
		return true;
	}

	//void write(const unsigned char* cache,int size)

	server_session() :log(lyramilk::klog,"iseal.rpc.server.session")
	{
	}
	virtual ~server_session()
	{
	}
};

int main(int argc,const char* argv[])
{
	lyramilk::netio::aiopoll aip;

	lyramilk::netio::aioserver<server_session> ais1;
	ais1.open(443);
	ais1.init_ssl("/root/ssl-keygen/test.crt","/root/ssl-keygen/test.key");
	aip.add(&ais1);

	lyramilk::netio::aioserver<server_session> ais2;
	ais2.open(8080);
	aip.add(&ais2);

	aip.active(4);

	int i = 0;
	std::cin >> i;
	return 0;
}
