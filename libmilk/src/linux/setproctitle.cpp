#include "setproctitle.h"
#include "var.h"
#include <string.h>
#include <stdarg.h>

extern char **environ;

namespace lyramilk
{
	static lyramilk::data::strings argvs;
	static lyramilk::data::strings environs;
	static std::vector<const char*> environ_ptrs;
	static std::vector<const char*> argv_ptrs;

	static char* ptitle = nullptr;
	static std::size_t ptitlelen = 0;

	void init_setproctitle(int argc,const char** &argv)
	{
		const char* lastargv = nullptr;
		for (int i = 0; i < argc; i++) {
			if (lastargv == nullptr || lastargv + 1 == argv[i])
			lastargv = argv[i] + strlen(argv[i]);
		}
		for (int i = 0; environ[i] != nullptr; i++) {
			if (lastargv + 1 == environ[i])
			lastargv = environ[i] + strlen(environ[i]);
		}
		ptitle = (char*)argv[0];
		ptitlelen = lastargv - argv[0] - 1;


		char ** envir = environ;
		while(*envir){
			environs.push_back(*envir);
			++envir;
		}

		lyramilk::data::strings::iterator it = environs.begin();
		for(;it!=environs.end();++it){
			environ_ptrs.push_back(it->c_str());
		}


		argvs.resize(argc);
		for(int i=0;i<argc;++i){
			argvs[i] = argv[i];
		}


		for(int i=0;i<argc;++i){
			argv_ptrs.push_back(argvs[i].c_str());
		}

		argv = (const char**)argv_ptrs.data();
	}

	bool setproctitle(const char *fmt, ...)
	{
		if(ptitle != nullptr && ptitlelen != 0){
			memset(ptitle,0,ptitlelen);

			char buff[1024];

			va_list va;
			int cnt;
			va_start(va, fmt);
			cnt = vsnprintf(buff,1024, fmt, va);
			va_end(va);
			if(cnt < 1024){
				strncpy(ptitle,buff,ptitlelen);
				return true;
			}
		}

		return false;
	}
}
