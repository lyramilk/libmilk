#include "testing.h"
#include "multilanguage.h"
#include <sstream>
#ifdef WIN32
#include <windows.h>
#include <time.h>
#endif
#ifdef __GNUC__
	#define D(x...) lyramilk::kdict(x)
#elif defined _MSC_VER
	#define D(...) lyramilk::kdict(__VA_ARGS__)
#endif

namespace lyramilk{ namespace debug
{

	void cpuinfo::bind(int cpucore)
	{
		cpu_set_t mask;
		CPU_SET(cpucore, &mask);
		pthread_setaffinity_np(pthread_self(),sizeof(mask),&mask);
	}

	int cpuinfo::count()
	{
		return get_nprocs();
	}

	void nsecdiff::mark()
	{
		clock_gettime(CLOCK_REALTIME, &timestamp);
	}

	long long nsecdiff::diff()
	{
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		long long s = ts.tv_sec - timestamp.tv_sec;
		long long n = ts.tv_nsec - timestamp.tv_nsec;
		return s * 1000000000 + n;
	}

	timer::timer()
	{
		l = v = time(0);
	}
	timer::~timer()
	{
	}
#ifdef WIN32
	timer::operator bool(){
		v = time(0);
		if(l != v && ((v % 1) == 0)){
			if (InterlockedCompareExchange((volatile unsigned long long*)&l, l, v)){
				return true;
			}
		}
		return false;
	}
#elif defined __GNUC__
	timer::operator bool(){
		v = time(0);
		if (l != v && ((v % 1) == 0)){
			if (__sync_bool_compare_and_swap(&l, l, v)){
				return true;
			}
		}
		return false;
	}
#endif

}}