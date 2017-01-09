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
	tm *t = localtime(&ti);
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



void logb::log(time_t ti,type ty,lyramilk::data::string usr,lyramilk::data::string app,lyramilk::data::string module,lyramilk::data::string str) const
{
	//lyramilk::threading::mutex_sync _(lock);
	switch(ty){
	  case debug:{
		lyramilk::data::string cache;
		cache.reserve(1024);
		cache.append("\x1b[36m");
		cache.append(strtime(ti));
		cache.append(" [");
		cache.append(module);
		cache.append("] ");
		cache.append("\x1b[0m");
		cache.append(str);
		//(std::cout << cache).flush();
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
		cache.append("\x1b[0m");
		cache.append(str);
		//(std::cout << cache).flush();
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
		cache.append("\x1b[0m");
		cache.append(str);
		//(std::cout << cache).flush();
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
		cache.append("\x1b[0m");
		cache.append(str);
		//(std::cerr << cache).flush();
		fwrite(cache.c_str(),cache.size(),1,stderr);
	  }break;
	}
}


logbuf::logbuf(logss& pp):buf(2048),p(pp)
{
	setp(buf.data(),buf.data() + buf.size());
}

logbuf::~logbuf()
{
}

std::streamsize logbuf::sputn (const char_type* s, std::streamsize  n)
{
	return std::basic_streambuf<char>::sputn(s,n);
}

logbuf::int_type logbuf::sputc (char_type c)
{
	return std::basic_streambuf<char>::sputc(c);
}

std::streamsize logbuf::xsputn (const char_type* s, std::streamsize  n)
{
	return std::basic_streambuf<char>::xsputn(s,n);
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
	if(!module.empty()){
		if(!p.module_suffix.empty()){
			module += '.' + p.module_suffix;
		}
	}else{
		module = p.module_suffix;
	}

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

	p.p->log(time(NULL),p.t,getlogin(),str.c_str(),module,lyramilk::data::string(pstr,len));
	/*/
	p.p->log(time(NULL),p.t,"user","app",module,lyramilk::data::string(pstr,len));
	/ **/
#endif
	p.t = trace;
	p.module_suffix.clear();
	setp(buf.data(),buf.data() + buf.size());
	return 0;
}

logss::logss():db(*this)
{
	t = trace;
	p = &loger;
	init(&db);
}

logss::logss(lyramilk::data::string m):db(*this)
{
	module = m;
	t = trace;
	p = &loger;
	init(&db);
}

logss::logss(const logss& qlog,lyramilk::data::string m):db(*this)
{
	module = qlog.module.c_str();
	if(qlog.module.empty()){
		module += m;
	}else if(!m.empty()){
		module += '.' + m;
	}
	t = qlog.t;
	p = &loger;
	p->set_proxy(qlog.p);
	init(&db);
}

logss::~logss()
{}

logss& logss::operator()(type ty)
{
	t = ty;
	return *this;
}

logss& logss::operator()(lyramilk::data::string m)
{
	module_suffix = m;
	return *this;
}

logss& logss::operator()(type ty,lyramilk::data::string m)
{
	module_suffix = m;
	t = ty;
	return *this;
}

lyramilk::log::logb* logss::rebase(lyramilk::log::logb* ploger)
{
	p->set_proxy(ploger);
	(*this)(lyramilk::log::debug,"lyramilk.log.logss") << lyramilk::kdict("恢复默认日志状态") << std::endl;

	logb* old = p;
	p = ploger;
	return old;
}
//		lyramilk::threading::mutex lock;
