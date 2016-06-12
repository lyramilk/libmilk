#include "clockdiff.h"
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

namespace lyramilk{ namespace debug {

#ifdef __GNUC__
	static inline long long __rdtsc()
	{
		long long x = 0;
		__asm__ volatile ("rdtsc" : "=A" (x));
		return x;
	}
#endif
	clockdiff::clockdiff()
	{
		mark();
		diff();
		long long d1,d2;
		d = d1 = d2 = 0;
		mark();
		d1 = diff();
		mark();
		d2 = diff();
		d = d1 > d2 ? d2 : d1;
	}

	void clockdiff::mark()
	{
		timestamp = __rdtsc();
	}

	long long clockdiff::diff()
	{
		long long newtimestamp = __rdtsc();
		long long des = newtimestamp - timestamp - d;
		return des;
	}

	lyramilk::data::string clockdiff::diff_str()
	{
		long long newtimestamp = __rdtsc();
		long long des = newtimestamp - timestamp - d;
		return D("耗时%llu个时钟周期。(d=%llu)", des, d);
	}

	long long clocktester::d = 0;

	clocktester::clocktester():outer(std::cout)
	{
		printable = true;
		str = "耗时：%lld";
		timestamp = __rdtsc();
	}
	clocktester::clocktester(lyramilk::data::string msg):outer(std::cout)
	{
		printable = true;
		str = msg;
		timestamp = __rdtsc();
	}
	clocktester::clocktester(std::ostream& os,lyramilk::data::string msg):outer(os)
	{
		printable = true;
		str = msg;
		timestamp = __rdtsc();
	}
	clocktester::~clocktester()
	{
		if(printable){
			long long newtimestamp = __rdtsc();
			long long des = newtimestamp - timestamp - d;
			outer << D(str.c_str(),des) << std::endl;
		}
	}

	void clocktester::cancel()
	{
		printable = false;
	}

	void clocktester::resume()
	{
		printable = true;
	}

	void clocktester::setmsg(lyramilk::data::string msg)
	{
		str = msg;
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