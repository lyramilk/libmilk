#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#include "netproxy.h"
#include "netaio.h"
#include "aio.h"
#include "dict.h"
#include "log.h"

#include <pwd.h>

class myhttpproxy:public lyramilk::netio::aioproxysession
{
	virtual bool onrequest(const char* cache, int size,int* bytesused, lyramilk::data::ostream& os)
	{
		*bytesused = 0;
		return combine("127.0.0.1",80);
	}
};



class proxy_server:public myhttpproxy
{
};



int main(int argc,const char* argv[])
{
	lyramilk::netio::aioserver<proxy_server> server;
	server.open(8080);

	lyramilk::data::string username = "teapoy";
	// useradd -s /sbin/nologin username
	struct passwd *pw = getpwnam(username.c_str());
	if(pw){
		if(setgid(pw->pw_gid) == 0 && setuid(pw->pw_uid) == 0){
			lyramilk::klog(lyramilk::log::debug,argv[0]) << D("切换到用户[%s]",username.c_str()) << std::endl;
			lyramilk::klog(lyramilk::log::debug,argv[0]) << D("切换到用户组[%s]",username.c_str()) << std::endl;
		}
	}

	lyramilk::io::aiopoll_safe pool(4);
	pool.add(&server);

	while(true){
		sleep(1);
	}

	return 0;
}
