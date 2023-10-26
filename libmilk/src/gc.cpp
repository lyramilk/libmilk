#include "gc.h"
#ifdef Z_HAVE_JEMALLOC
	#include <jemalloc/jemalloc.h>
#else
	#include <malloc.h>
#endif

#ifdef _WIN32
	#include <windows.h>
#else
	#define IsBadReadPtr(ptr,len) (ptr == nullptr)
#endif

using namespace lyramilk;

//int objcount = 0;

obj::obj()
{
	_rc = 0;
}

obj::~obj()
{
}

/*
#ifdef _MSC_VER
void obj::add_ref()
{
	InterlockedAdd(&_rc, 1);
}

void obj::sub_ref()
{
	InterlockedAdd(&_rc, -1);
}
#elif defined __GNUC__
	void obj::add_ref()
	{
		__sync_add_and_fetch(&_rc, 1);
	}

	void obj::sub_ref()
	{
		__sync_sub_and_fetch(&_rc, 1);
	}
#endif
*/
void obj::add_ref()
{
	++_rc;
}

void obj::sub_ref()
{
	--_rc;
}

int obj::payload() const
{
	return _rc;
}

bool obj::verify() const
{
	return !IsBadReadPtr(this,sizeof(_rc));
}

bool obj::try_del()
{
	//if(!__sync_bool_compare_and_swap(&_rc,0,0))return false;
	if(_rc > 0) return false;
	ondestory();
	return true;
}

void obj::ondestory()
{
	delete this;
}

void* obj::operator new(size_t sz)
{
	return malloc(sz);
}

void* obj::operator new[](size_t sz)
{
	return malloc(sz);
}

void obj::operator delete(void* sz)
{
	free(sz);
}

void obj::operator delete[](void* sz)
{
	free(sz);
}
