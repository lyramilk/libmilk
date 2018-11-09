#include "log.h"
#include "ansi_3_64.h"
#include "multilanguage.h"
#include <sstream>
#include <time.h>
#include <iomanip>

#ifdef __linux__
#include <unistd.h>
#endif
#include <stdio.h>
#include <linux/limits.h>

using namespace lyramilk::log;

logss lyramilk::klog;

logb::logb()
{
	proxy = nullptr;
}

logb::~logb()
{
}

void logb::set_proxy(const logb* pr)
{
	proxy = pr;
}

lyramilk::data::string logb::strtime(time_t ti) const
{
	static lyramilk::threading::mutex_rw lock;
	static time_t last_time = 0;
	static lyramilk::data::string last_time_str;

	{
		lyramilk::threading::mutex_sync _(lock.r());
		if(last_time == ti && !last_time_str.empty()){
			return last_time_str;
		}
	}

#ifdef __GNUC__
	tm __t;
	tm *t = localtime_r(&ti,&__t);
#else
	tm __t;
	tm* t = &__t;
	localtime_s(t, &ti);
#endif
	if (t == NULL){
		last_time = 0;
		lyramilk::threading::mutex_sync _(lock.w());
		last_time_str.clear();
		return "";
	}
	last_time = ti;
	lyramilk::data::stringstream ss;
	ss << std::setfill('0') << std::setw(4) << (1900 + t->tm_year) <<"-" << std::setw(2) << 1+t->tm_mon << "-" << std::setw(2) << t->tm_mday << " " << std::setw(2) << t->tm_hour << ":" << std::setw(2) << t->tm_min << ":" << std::setw(2) << t->tm_sec;
	lyramilk::threading::mutex_sync _(lock.w());
	last_time_str = ss.str();
	return last_time_str;
}



void logb::log(time_t ti,type ty,const lyramilk::data::string& usr,const lyramilk::data::string& app,const lyramilk::data::string& module,const lyramilk::data::string& str) const
{
	if(proxy){
		proxy->log(ti,ty,usr,app,module,str);
		return;
	}
	switch(ty){
	  case debug:{
		lyramilk::data::string cache;
		cache.reserve(1024);
		cache.append("\x1b[36m");
		cache.append(strtime(ti));
		cache.append(" [");
		cache.append(module);
		cache.append("] ");
		cache.append(str);
		cache.append("\x1b[0m");
		fwrite(cache.c_str(),cache.size(),1,stdout);
	  }break;
	  case trace:{
		lyramilk::data::string cache;
		cache.reserve(1024);
		cache.append("\x1b[37m");
		cache.append(strtime(ti));
		cache.append(" [");
		cache.append(module);
		cache.append("] ");
		cache.append(str);
		cache.append("\x1b[0m");
		fwrite(cache.c_str(),cache.size(),1,stdout);
	  }break;
	  case warning:{
		lyramilk::data::string cache;
		cache.reserve(1024);
		cache.append("\x1b[33m");
		cache.append(strtime(ti));
		cache.append(" [");
		cache.append(module);
		cache.append("] ");
		cache.append(str);
		cache.append("\x1b[0m");
		fwrite(cache.c_str(),cache.size(),1,stdout);
	  }break;
	  case error:{
		lyramilk::data::string cache;
		cache.reserve(1024);
		cache.append("\x1b[31m");
		cache.append(strtime(ti));
		cache.append(" [");
		cache.append(module);
		cache.append("] ");
		cache.append(str);
		cache.append("\x1b[0m");
		fwrite(cache.c_str(),cache.size(),1,stderr);
	  }break;
	}
}


logbuf::logbuf(logss2& pp):buf(2048),p(pp)
{
	setp(buf.data(),buf.data() + buf.size());
}

logbuf::~logbuf()
{
}

logbuf::int_type logbuf::overflow(int_type _Meta)
{
	sync();
	return sputc(_Meta);
}

int logbuf::sync()
{
	const char* pstr = pbase();
	int len = (int)(pptr() - pbase());

	lyramilk::data::string module = p.module;

#ifdef __linux__
	/**/
	static lyramilk::data::string str;
	static pid_t pid = getpid();
	if(pid == getpid() && str.empty()){
		char buff[PATH_MAX] = {0};
		ssize_t sz = readlink("/proc/self/exe",buff,PATH_MAX);
		if(sz > 0){
			str.assign(buff,sz);
		}
	}

	const char* loginuser = getlogin();

	p.loger.log(time(NULL),p.t,loginuser?loginuser:"unknow_user",str.c_str(),module,lyramilk::data::string(pstr,len));
	/*/
	p.p->log(time(NULL),p.t,"user","app",module,lyramilk::data::string(pstr,len));
	/ **/
#endif
	setp(buf.data(),buf.data() + buf.size());
	return 0;
}



logss2::logss2():db(*this)
{
	init(&db);
	t = trace;
}

logss2::~logss2()
{
}

logss::logss()
{
	p = nullptr;
}

logss::logss(const lyramilk::data::string& m)
{
	prefix = m;
	p = nullptr;
}

logss::logss(const logss& qlog,const lyramilk::data::string& m)
{
	prefix = m;
	p = qlog.p;
}

logss::~logss()
{}

void static logssclean(void* parg)
{
	logss2* plogss2 = (logss2*)parg;
	delete plogss2;
}

static pthread_key_t logss2_key;

static __attribute__ ((constructor)) void __init()
{
	pthread_key_create(&logss2_key,logssclean);
}

logss2& logss::operator()(type ty) const
{
	logss2* plogss2 = (logss2*)pthread_getspecific(logss2_key);
	if(!plogss2){
		plogss2 = new logss2;
		pthread_setspecific(logss2_key,plogss2);
	}
	plogss2->t = ty;
	plogss2->module = prefix;
	plogss2->loger.set_proxy(p);
	return *plogss2;
}

logss2& logss::operator()(const lyramilk::data::string& m) const
{
	logss2* plogss2 = (logss2*)pthread_getspecific(logss2_key);
	if(!plogss2){
		plogss2 = new logss2;
		pthread_setspecific(logss2_key,plogss2);
	}
	if(prefix.empty()){
		plogss2->module = m;
	}else{
		plogss2->module = prefix + "." + m;
	}
	plogss2->loger.set_proxy(p);
	return *plogss2;
}

logss2& logss::operator()(type ty,const lyramilk::data::string& m) const
{
	logss2* plogss2 = (logss2*)pthread_getspecific(logss2_key);
	if(!plogss2){
		plogss2 = new logss2;
		pthread_setspecific(logss2_key,plogss2);
	}
	plogss2->t = ty;
	if(prefix.empty()){
		plogss2->module = m;
	}else{
		plogss2->module = prefix + "." + m;
	}
	plogss2->loger.set_proxy(p);
	return *plogss2;
}

lyramilk::log::logb* logss::rebase(lyramilk::log::logb* ploger)
{
	if(ploger == p) return ploger;
	if(ploger){
		(*this)(lyramilk::log::debug,"lyramilk.log.logss") << lyramilk::kdict("切换日志状态") << std::endl;
	}else{
		(*this)(lyramilk::log::debug,"lyramilk.log.logss") << lyramilk::kdict("恢复默认日志状态") << std::endl;
	}

	logb* old = p;
	p = ploger;
	return old;
}
//		lyramilk::threading::mutex lock;
