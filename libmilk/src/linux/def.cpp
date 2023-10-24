#include "def.h"
#include "hash.h"
#include <algorithm>

#ifdef Z_HAVE_JEMALLOC
	#include <jemalloc/jemalloc.h>
#else
	#include <malloc.h>
#endif

std::locale::id id;

namespace lyramilk { namespace data{
	void* milk_malloc(size_t size)
	{
		return ::malloc(size);
	}

	void milk_free(void* p, size_t size)
	{
		::free(p);
	}

	type_invalid::type_invalid(const string& msg)
	{
		p = msg;
	}

	type_invalid::~type_invalid() throw()
	{
	}

	const char* type_invalid::what() const throw()
	{
		return p.c_str();
	}


	size_t case_insensitive_hash::operator()(const lyramilk::data::string& a) const
	{
		lyramilk::data::string sa(a.size(),0);
		transform(a.begin(), a.end(), sa.begin(), tolower);
		return lyramilk::cryptology::hash64::fnv(sa.c_str(),sa.size());
	}

	bool case_insensitive_equare::operator()(const lyramilk::data::string& a,const lyramilk::data::string& b) const
	{
		lyramilk::data::string sa(a.size(),0);
		transform(a.begin(), a.end(), sa.begin(), tolower);

		lyramilk::data::string sb(b.size(),0);
		transform(b.begin(), b.end(), sb.begin(), tolower);

		return sa == sb;
	}

	bool case_insensitive_less::operator()(const lyramilk::data::string& a,const lyramilk::data::string& b) const
	{
		lyramilk::data::string sa(a.size(),0);
		transform(a.begin(), a.end(), sa.begin(), tolower);

		lyramilk::data::string sb(b.size(),0);
		transform(b.begin(), b.end(), sb.begin(), tolower);
		return sa < sb;
	}
}}	//namespace

lyramilk::data::bostream& operator << (lyramilk::data::bostream& os, char c)
{
	return os << (unsigned char)c;
}

lyramilk::data::bostream& operator << (lyramilk::data::bostream& os, signed char c)
{
	return os << (unsigned char)c;
}

lyramilk::data::bostream& operator << (lyramilk::data::bostream& os, const char* c)
{
	return os << (const unsigned char*)c;
}

lyramilk::data::bostream& operator << (lyramilk::data::bostream& os, const signed char* c)
{
	return os << (const unsigned char*)c;
}


