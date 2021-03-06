﻿#include "inotify.h"
#include "dict.h"
#include "log.h"
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

namespace lyramilk{namespace data
{
/*
	static void trace_event(lyramilk::data::string filename,unsigned int event)
	{
		lyramilk::data::stringstream ss;
		ss << filename << ":";
		if(event & IN_ACCESS){
			ss <<"IN_ACCESS,";
		}
		if(event & IN_MODIFY){
			ss <<"IN_MODIFY,";
		}
		if(event & IN_ATTRIB){
			ss <<"IN_ATTRIB,";
		}
		if(event & IN_CLOSE_WRITE){
			ss <<"IN_CLOSE_WRITE,";
		}
		if(event & IN_CLOSE_NOWRITE){
			ss <<"IN_CLOSE_NOWRITE,";
		}
		if(event & IN_OPEN){
			ss <<"IN_OPEN,";
		}
		if(event & IN_MOVED_FROM){
			ss <<"IN_MOVED_FROM,";
		}
		if(event & IN_MOVED_TO){
			ss <<"IN_MOVED_TO,";
		}
		if(event & IN_CREATE){
			ss <<"IN_CREATE,";
		}
		if(event & IN_DELETE_SELF){
			ss <<"IN_DELETE_SELF,";
		}
		if(event & IN_MOVE_SELF){
			ss <<"IN_MOVE_SELF,";
		}

		if(event & IN_UNMOUNT){
			ss <<"IN_UNMOUNT,";
		}
		if(event & IN_Q_OVERFLOW){
			ss <<"IN_Q_OVERFLOW,";
		}
		if(event & IN_IGNORED){
			ss <<"IN_IGNORED,";
		}
		if(event & IN_ONLYDIR){
			ss <<"IN_ONLYDIR,";
		}
		if(event & IN_DONT_FOLLOW){
			ss <<"IN_DONT_FOLLOW,";
		}
		if(event & IN_EXCL_UNLINK){
			ss <<"IN_EXCL_UNLINK,";
		}
		if(event & IN_MASK_ADD){
			ss <<"IN_MASK_ADD,";
		}
		if(event & IN_ISDIR){
			ss <<"IN_ISDIR,";
		}
		if(event & IN_ONESHOT){
			ss <<"IN_ONESHOT,";
		}

		COUT << ss.str() << std::endl;
	}
*/

	bool inotify::notify_in()
	{
		char buff[4096];
		/*
		if(::read(fd,&u,sizeof(u)) != sizeof(u)){
			lyramilk::klog(lyramilk::log::error,"lyramilk.io.notify.onevent") << lyramilk::kdict("读取事件时发生错误%s",strerror(errno)) << std::endl;
			return true;
		}
		*/

		int r = ::read(fd,buff,sizeof(buff));
		if(r == -1){
			lyramilk::klog(lyramilk::log::error,"lyramilk.io.notify.onevent") << lyramilk::kdict("读取事件时发生错误%s",strerror(errno)) << std::endl;
			return pool->reset(this,flag());
			return true;
		}

		char* p = buff;
		for(int i=0;p<buff + r;++i){
			inotify_event* ie = (inotify_event*)p;
			p = ie->name + ie->len;
			notify_event(ie);
		}

		return pool->reset(this,flag());
	}

	bool inotify::notify_out()
	{
		TODO();
	}

	bool inotify::notify_hup()
	{
		TODO();
	}

	bool inotify::notify_err()
	{
		TODO();
	}

	bool inotify::notify_pri()
	{
		TODO();
	}

	lyramilk::io::native_filedescriptor_type inotify::getfd()
	{
		return fd;
	}

	void inotify::ondestory()
	{
	}

	int inotify::flag()
	{
		return EPOLLIN | EPOLLONESHOT;
	}

	inotify::inotify()
	{
		fd = inotify_init();
	}

	inotify::~inotify()
	{
		::close(fd);
	}
	
	void inotify::notify_event(inotify_event* ie)
	{
		lyramilk::data::string filename;
		lyramilk::data::string dirname;
		std::map<lyramilk::io::native_filedescriptor_type,lyramilk::data::string>::iterator it = wm.find(ie->wd);
		if(it!=wm.end()){
			dirname = it->second;
		}
		if(ie->len > 0){
			filename = ie->name;
		}

		uint32 event = ie->mask;

//COUT << "char=" << (int)'~' << ",len=" << ie->len << "_" << filename_raw.size() << "," << (int)filename_raw[filename_raw.size()] << std::endl;
		//if(!filename_raw.empty() && (filename_raw[0] == '.' || filename_raw[filename_raw.size() - 1] == '~')) return;
		//trace_event(filename,event);

		if(event & (IN_CREATE|IN_MOVED_TO)){
			notify_add(dirname,filename);
			return;
		}
		if(event & (IN_MODIFY)){
			notify_modify(dirname,filename);
			return;
		}
		if(event & (IN_DELETE|IN_DELETE_SELF|IN_MOVED_FROM|IN_MOVE_SELF)){
			if(access((dirname + "/" + filename).c_str(),F_OK) == 0){
				notify_modify(dirname,filename);
			}else{
				notify_remove(dirname,filename);
			}
			return;
		}
		notify_other(ie);
	}

	void inotify::notify_add(const lyramilk::data::string& dirname,const lyramilk::data::string& filename)
	{
		//COUT << "添加文件" << filename << ",请扫描" << std::endl;
	}

	void inotify::notify_modify(const lyramilk::data::string& dirname,const lyramilk::data::string& filename)
	{
		//COUT << "修改文件" << filename << ",请扫描" << std::endl;
	}

	void inotify::notify_remove(const lyramilk::data::string& dirname,const lyramilk::data::string& filename)
	{
		//COUT << "删除文件" << filename << ",请扫描" << std::endl;
	}
	
	void inotify::notify_other(inotify_event* ie)
	{
		
	}

	lyramilk::io::native_filedescriptor_type inotify::add(const lyramilk::data::string& spathdirname)
	{
		lyramilk::data::string pathdirname = spathdirname;
		struct stat st = {0};
		if(stat(pathdirname.c_str(),&st) == -1/* && errno != ENOENT*/){
			if(errno == ENOENT){
				lyramilk::klog(lyramilk::log::warning,"lyramilk.io.inotify.add") << lyramilk::kdict("监视的目标[%s]不存在，自动监视其父目录。",pathdirname.c_str()) << std::endl;
			}else{
				lyramilk::klog(lyramilk::log::error,"lyramilk.io.inotify.add") << lyramilk::kdict("添加监视时出错：%s",strerror(errno)) << std::endl;
				return 0;
			}
		}

		if((st.st_mode&S_IFDIR) == 0){
			lyramilk::data::string modifyedstr;
			std::size_t pos = pathdirname.rfind("/");
			if(pos == pathdirname.npos){
				modifyedstr = ".";
			}else{
				modifyedstr = pathdirname.substr(0,pos);
			}
			//lyramilk::klog(lyramilk::log::warning,"lyramilk.io.inotify.add") << lyramilk::kdict("监视的目标[%s]不是一个目录，自动改为监视[%s]",pathdirname.c_str(),modifyedstr.c_str()) << std::endl;
			pathdirname = modifyedstr;
		}

		lyramilk::io::native_filedescriptor_type wfd = inotify_add_watch(fd,pathdirname.c_str(),IN_CREATE|IN_MODIFY|IN_MOVED_FROM|IN_MOVED_TO|IN_DELETE|IN_DELETE_SELF|IN_MOVE_SELF);

		std::map<lyramilk::io::native_filedescriptor_type,lyramilk::data::string>::iterator it = wm.find(wfd);
		if(it!=wm.end()){
			//lyramilk::klog(lyramilk::log::warning,"lyramilk.io.inotify.add") << lyramilk::kdict("己监视目录[%s]，不需要重复监视。",pathdirname.c_str()) << std::endl;
			return 0;
		}else{
			lyramilk::klog(lyramilk::log::debug,"lyramilk.io.inotify.add") << lyramilk::kdict("成功监视目录[%s]。",pathdirname.c_str()) << std::endl;
		}
		wm[wfd] = pathdirname;
		return wfd;
	}

	lyramilk::io::native_filedescriptor_type inotify::add(const lyramilk::data::string& filename,lyramilk::io::uint32 inotify_mask)
	{
		return inotify_add_watch(fd,filename.c_str(),inotify_mask);
	}

	void inotify::remove(lyramilk::io::native_filedescriptor_type childfd)
	{
		wm.erase(childfd);
		inotify_rm_watch(fd,childfd);
	}


	inotify_file::inotify_file(const lyramilk::data::string& spathdirname)
	{
		lyramilk::data::string pathdirname = spathdirname;
		fd = inotify_init();

		struct stat st = {0};
		if(stat(pathdirname.c_str(),&st) == -1/* && errno != ENOENT*/){
			if(errno == ENOENT){
				lyramilk::klog(lyramilk::log::warning,"lyramilk.io.inotify_file.add") << lyramilk::kdict("监视的目标[%s]不存在，自动监视其父目录。",pathdirname.c_str()) << std::endl;
			}else{
				lyramilk::klog(lyramilk::log::error,"lyramilk.io.inotify_file.add") << lyramilk::kdict("添加监视时出错：%s",strerror(errno)) << std::endl;
				return;
			}
		}

		if((st.st_mode&S_IFDIR) == 0){
			lyramilk::data::string modifyedstr;
			std::size_t pos = pathdirname.rfind("/");
			if(pos == pathdirname.npos){
				modifyedstr = ".";
				filename = pathdirname;
			}else{
				modifyedstr = pathdirname.substr(0,pos);
				filename = pathdirname.substr(pos + 1);
			}
			//lyramilk::klog(lyramilk::log::warning,"lyramilk.io.inotify_file.add") << lyramilk::kdict("监视的目标[%s]不是一个目录，自动改为监视[%s]",pathdirname.c_str(),modifyedstr.c_str()) << std::endl;
			pathdirname = modifyedstr;
		}

		wfd = inotify_add_watch(fd,pathdirname.c_str(),IN_CREATE|IN_MODIFY|IN_MOVED_FROM|IN_MOVED_TO|IN_DELETE|IN_DELETE_SELF|IN_MOVE_SELF);
		lyramilk::klog(lyramilk::log::debug,"lyramilk.io.inotify_file.add") << lyramilk::kdict("成功监视目录[%s]。",pathdirname.c_str()) << std::endl;
		dirname = pathdirname;
	}

	inotify_file::~inotify_file()
	{
		::close(fd);
	}

	inotify_file::status inotify_file::check()
	{
		status t = s_keep;
		pollfd pfd;
		pfd.fd = fd;
		pfd.events = POLLIN;
		pfd.revents = 0;
		if(::poll(&pfd,1,0) != 1){
			return t;
		}
		if(pfd.revents != POLLIN){
			return t;
		}

		char buff[4096];
		int r = ::read(fd,buff,sizeof(buff));
		if(r == -1){
			lyramilk::klog(lyramilk::log::error,"lyramilk.io.notify.onevent") << lyramilk::kdict("读取事件时发生错误%s",strerror(errno)) << std::endl;
			return t;
		}

		char* p = buff;
		for(int i=0;p<buff + r;++i){
			inotify_event* ie = (inotify_event*)p;
			p = ie->name + ie->len;
			

			lyramilk::data::string dstfilename;
			if(ie->len > 0){
				dstfilename = ie->name;
			}

			if(dstfilename != filename){
				continue;
			}

			uint32 event = ie->mask;

			if(event & (IN_CREATE|IN_MOVED_TO)){
				t = s_add;
			}
			if(event & (IN_MODIFY)){
				t = s_modify;
			}
			if(event & (IN_DELETE|IN_DELETE_SELF|IN_MOVED_FROM|IN_MOVE_SELF)){
				if(access((dirname + "/" + filename).c_str(),F_OK) == 0){
					t = s_modify;
				}else{
					t = s_remove;
				}
			}
		}
		return t;
	}
}}