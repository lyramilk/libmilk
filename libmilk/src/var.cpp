#include "var.h"
#include "multilanguage.h"
#include <vector>
#include <sstream>
#include <algorithm>

#ifndef null
#define null nullptr
#endif

std::locale::id id;

using namespace lyramilk::data;

const var var::nil;

template <int T>
struct tempc;

template <>
struct tempc<2>
{
	typedef int16 t;
	typedef uint16 ut;
	enum {
		square = 1
	};
};

template <>
struct tempc<4>
{
	typedef int32 t;
	typedef uint32 ut;
	enum {
		square = 2
	};
};

template <>
struct tempc<8>
{
	typedef int64 t;
	typedef uint64 ut;
	enum {
		square = 3
	};
};

template <typename T>
T reverse_order(T t)
{
	const int sz = sizeof(T);
	union{
		T v;
		char c[0];
	}s,d;
	s.v = t;
	for(int i =0;i < sz;++i){
		d.c[i] = s.c[sz - i - 1];
	}
	return d.v;
}

#ifdef _WIN32
	#include <Windows.h>

	class u2a
	{
		string p;
	  public:
		u2a(const wstring& wstr)
		{
			p.resize((wstr.length() + 1) << 1);
			char* _ = (char*)p.c_str();
			int i = WideCharToMultiByte(CP_ACP,0,wstr.c_str(),(int)wstr.length(),_,(int)p.capacity(),null,null);
			p.erase(p.begin() + i,p.end());
		}
		/*
		operator const char*()
		{
			return p.data();
		}*/
		operator string()
		{
			return p;
		}
	};

	class a2u
	{
		wstring p;
	  public:
		a2u(const string& str)
		{
			p.resize(str.length() + 1);
			wchar_t* _ = (wchar_t*)p.c_str();
			int i = MultiByteToWideChar(CP_ACP,0,str.c_str(),(int)str.length(),_,(int)p.capacity());
			p.erase(p.begin() + i,p.end());
		}
		/*
		operator const wchar_t*()
		{
			return p.data();
		}*/
		operator wstring()
		{
			return p;
		}
	};

	class u2t
	{
		string p;
	  public:
		u2t(const wstring& wstr)
		{
			int len = WideCharToMultiByte(CP_UTF8,0,wstr.c_str(),(int)wstr.length(),null,0,null,null);
			p.resize(len);
			char* _ = (char*)p.c_str();
			int i = WideCharToMultiByte(CP_UTF8,0,wstr.c_str(),(int)wstr.length(),_,(int)p.capacity(),null,null);
			p.erase(p.begin() + i,p.end());
		}
		/*
		operator const char*()
		{
			return p.c_str();
		}*/
		operator string()
		{
			return p;
		}
	};
	class t2u
	{
		wstring p;
	  public:
		t2u(const string& str)
		{
			int len = MultiByteToWideChar(CP_UTF8,0,str.c_str(),(int)str.length(),null,0);
			p.resize(len);
			wchar_t* _ = (wchar_t*)p.c_str();
			int i = MultiByteToWideChar(CP_UTF8,0,str.c_str(),(int)str.length(),_,(int)p.capacity());
			p.erase(p.begin() + i,p.end());
		}
		/*
		operator const wchar_t*()
		{
			return p.data();
		}*/
		operator wstring()
		{
			return wstring(p.data());
		}
	};

	#define t2a(x) (u2a(t2u(x)))
	#define a2t(x) (u2t(a2u(x)))

#elif defined __linux__
	#include <iconv.h>
	#include <assert.h>
	#include <arpa/inet.h>


	string iconv(string str,string from,string to)
	{
		const size_t ds = 64;

		iconv_t cd = iconv_open(from.c_str(),to.c_str());
		if(cd == 0){
			assert(cd);
			return "";
		}

		char* p1 = (char*)str.c_str();
		size_t p1s = str.size();

		std::vector<char> ret;
		char* p2 = ret.data();
		size_t p2s = ret.size();

		int rc = -1;
		do{
			size_t pos = p2 - ret.data();
			ret.insert(ret.end(),ds,0);
			p2 = ret.data() + pos;
			p2s = ret.size() - pos;
			rc = ::iconv(cd,&p1,&p1s,&p2,&p2s);
		}while(rc == -1 && p2s < ds);

		iconv_close(cd);
		if(rc == -1){
			return "";
		}
		return string(ret.data(),p2 - (char*)ret.data());
	}


	wstring utf8_unicode(string str)
	{
		string dst = iconv(str,"wchar_t","utf8//ignore");
		if(dst.size()&1){
			dst.push_back(0);
		}
		return wstring((wchar_t*)dst.c_str(),dst.size() >> tempc<sizeof(wchar_t)>::square);
	}



	string unicode_utf8(wstring str)
	{
		string src((char*)str.c_str(),str.size() << tempc<sizeof(wchar_t)>::square);
		string dst = iconv(src,"utf8","wchar_t");
		return dst;
	}


	class u2a
	{
		string p;
	  public:
		u2a(const wstring& wstr)
		{
			p = unicode_utf8(wstr);
		}
		
		operator string()
		{
			return p;
		}
	};

	class a2u
	{
		wstring p;
	  public:
		a2u(const string& str)
		{
			p = utf8_unicode(str);
		}
		
		operator wstring()
		{
			return p;
		}
	};

	#define t2a(x) (x)
	#define a2t(x) (x)

	#define t2u(x) a2u(x)
	#define u2t(x) u2a(x)
#endif


void* lyramilk::data::milk_malloc(size_t size)
{
	return ::malloc(size);
}

void lyramilk::data::milk_free(void* p, size_t size)
{
	::free(p);
}


var::type_invalid::type_invalid(string msg)
{
	p = msg;
}

var::type_invalid::~type_invalid() throw()
{
}

const char* var::type_invalid::what() const throw()
{
	return p.c_str();
}

var::var()
{
	t = t_invalid;
	u.a = null;
}

var::var(const var& v)
{
	t = t_invalid;
	u.a = null;
	assign(v);
}

var::~var()
{
	clear();
}

var::var(const unsigned char* v)
{
	t = t_invalid;
	u.a = null;
	assign(v);
}

var::var(const char* v)
{
	t = t_invalid;
	u.a = null;
	assign(v);
}

var::var(const wchar_t* v)
{
	t = t_invalid;
	u.a = null;
	assign(v);
}

var::var(const chunk& v)
{
	t = t_invalid;
	u.a = null;
	assign(v);
}

var::var(const string& v)
{
	t = t_invalid;
	u.a = null;
	assign(v);
}

var::var(const wstring& v)
{
	t = t_invalid;
	u.a = null;
	assign(v);
}

var::var(bool v)
{
	t = t_invalid;
	u.a = null;
	assign(v);
}

var::var(int8 v)
{
	t = t_invalid;
	u.a = null;
	assign(v);
}

var::var(uint8 v)
{
	t = t_invalid;
	u.a = null;
	assign(v);
}

var::var(int16 v)
{
	t = t_invalid;
	u.a = null;
	assign(v);
}

var::var(uint16 v)
{
	t = t_invalid;
	u.a = null;
	assign(v);
}

var::var(int32 v)
{
	t = t_invalid;
	u.a = null;
	assign(v);
}

var::var(uint32 v)
{
	t = t_invalid;
	u.a = null;
	assign(v);
}

var::var(long v)
{
	t = t_invalid;
	u.a = null;
	assign(v);
}

var::var(unsigned long v)
{
	t = t_invalid;
	u.a = null;
	assign(v);
}

var::var(int64 v)
{
	t = t_invalid;
	u.a = null;
	assign(v);
}

var::var(uint64 v)
{
	t = t_invalid;
	u.a = null;
	assign(v);
}

var::var(double v)
{
	t = t_invalid;
	u.a = null;
	assign(v);
}

var::var(float v)
{
	t = t_invalid;
	u.a = null;
	assign(v);
}

var::var(const array& v)
{
	t = t_invalid;
	u.a = null;
	assign(v);
}

var::var(const map& v)
{
	t = t_invalid;
	u.a = null;
	assign(v);
}

var::var(const string& n,const void* v)
{
	t = t_invalid;
	u.a = null;
	assign(n,v);
}

var var::operator +(const var& v) const throw(type_invalid)
{
	return var(*this) += var(v);
}

var& var::operator +=(const var& v) throw(type_invalid)
{
	switch(t){
	  case t_bin:{
			*u.p += (chunk)v;
		}break;
	  case t_str:{
			*u.s += (string)v;
		}break;
	  case t_wstr:{
			*u.w += (wstring)v;
		}break;
	  case t_bool:{
			u.b |= (bool)v;
		}break;
	  case t_int8:{
			u.i += (int8)v;
		}break;
	  case t_uint8:{
			u.u += (uint8)v;
		}break;
	  case t_int16:{
			u.i2 += (int16)v;
		}break;
	  case t_uint16:{
			u.u2 += (uint16)v;
		}break;
	  case t_int32:{
			u.i4 += (int32)v;
		}break;
	  case t_uint32:{
			u.u4 += (uint32)v;
		}break;
	  case t_int64:{
			u.i8 += (int64)v;
		}break;
	  case t_uint64:{
			u.u8 += (uint64)v;
		}break;
	  case t_double:{
			u.f8 += (double)v;
		}break;
	  case t_array:{
			if(v.t == t_array){
				u.a->insert(u.a->end(),v.u.a->begin(),v.u.a->end());
			}else if(v.t == t_map){
				array av = v;
				*this += av;
			}else{
				u.a->push_back(v);
			}
		}break;
	  case t_map:{
			if(v.t == t_map){
				map::iterator it = v.u.m->begin();
				for(;it!=v.u.m->end();++it){
					u.m->insert(*it);
				}
				u.a->insert(u.a->end(),v.u.a->begin(),v.u.a->end());
			}else{
				throw type_invalid(lyramilk::kdict("%s 错误：t_map类型不能追加其它类型","lyramilk::data::var::operator +=()"));
			}
		}break;
	  case t_invalid:{
			assign(v);
		}break;
	  default:
		throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator +=()",type_name(t).c_str()));
	}
	return *this;
}

bool var::operator ==(const var& v) const throw(type_invalid)
{
	switch(t){
	  case t_bin:{
			if(v.t == t_bool){
				return ((bool)*this) == v.u.b;
			}else{
				return u.p->compare((chunk)v) == 0;
			}
		}break;
	  case t_str:{
			if(v.t == t_bool){
				return ((bool)*this) == v.u.b;
			}else{
				return u.s->compare((string)v) == 0;
			}
		}break;
	  case t_wstr:{
			if(v.t == t_bool){
				return ((bool)*this) == v.u.b;
			}else{
				return u.w->compare((wstring)v) == 0;
			}
		}break;
	  case t_bool:{
			return u.b == (bool)v;
		}break;
	  case t_int8:{
			return u.i == (int8)v;
		}break;
	  case t_uint8:{
			return u.u == (uint8)v;
		}break;
	  case t_int16:{
			return u.i2 == (int16)v;
		}break;
	  case t_uint16:{
			return u.u2 == (uint16)v;
		}break;
	  case t_int32:{
			return u.i4 == (int32)v;
		}break;
	  case t_uint32:{
			return u.u4 == (uint32)v;
		}break;
	  case t_int64:{
			return u.i8 == (int64)v;
		}break;
	  case t_uint64:{
			return u.u8 == (uint64)v;
		}break;
	  case t_double:{
			return u.f8 == (double)v;
		}break;
	  case t_array:{
			if(v.t != t_array){
				if(u.a->size() == 1 && u.a->at(0) == v) return true;
				return false;
			}else{
				array::size_type size = u.a->size();
				if(size != v.u.a->size()) return false;
				for(array::size_type i=0;i<size;++i){
					if(u.a->at(i) != v.u.a->at(i)) return false;
				}
				return true;
			}
		}break;
	  case t_map:{
			return false;
		}break;
	  case t_user:{
			return false;
		}break;
	  case t_invalid:{
			return t_invalid == v.t;
		}break;
	}
	throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator ==()",type_name(t).c_str()));
}

bool var::operator !=(const var& v) const throw(type_invalid)
{
	return !(*this == v);
}

bool var::operator <(const var& v) const throw(type_invalid)
{
	switch(t){
	  case t_bin:{
			if(v.t == t_bool){
				return ((bool)*this) == v.u.b;
			}else{
				return u.p->compare((chunk)v) < 0;
			}
		}break;
	  case t_str:{
			if(v.t == t_bool){
				return ((bool)*this) == v.u.b;
			}else{
				return u.s->compare((string)v) < 0;
			}
		}break;
	  case t_wstr:{
			if(v.t == t_bool){
				return ((bool)*this) == v.u.b;
			}else{
				return u.w->compare((wstring)v) < 0;
			}
		}break;
	  case t_array:{
			if(v.t != t_array){
				if(!u.a->empty()) return u.a->at(0) < v;
				return false;
			}else{
				array::iterator it1 = u.a->begin();
				array::iterator it2 = v.u.a->begin();

				for(;it1!=u.a->end() && it2!=v.u.a->end();++it1,++it2){
					if((*it1) != (*it2)){
						return *it1 < *it2;
					}
				}
				//如果到这里还没返回，说明前面一直相等，此时检查数组容量，如果右值容量大，则左小于右。
				if(it2 != v.u.a->end()) return true;
				return false;
			}
		}break;
	  case t_bool:{
			return u.b < (bool)v;
		}break;
	  case t_int8:{
			return u.i < (int32)v;
		}break;
	  case t_uint8:{
			return u.u < (uint32)v;
		}break;
	  case t_int16:{
			return u.i2 < (int32)v;
		}break;
	  case t_uint16:{
			return u.u2 < (uint32)v;
		}break;
	  case t_int32:{
			return u.i4 < (int32)v;
		}break;
	  case t_uint32:{
			return u.u4 < (uint32)v;
		}break;
	  case t_int64:{
			return u.i8 < (int64)v;
		}break;
	  case t_uint64:{
			return u.u8 < (uint64)v;
		}break;
	  case t_double:{
			return u.f8 < (double)v;
		}break;
	  case t_invalid:{
			return false;
		}break;
	  default:
		throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator <()",type_name(t).c_str()));
	}
}

bool var::operator >(const var& v) const throw(type_invalid)
{
	if(t == t_invalid){
		return false;
	}
	if(v == *this) return false;
	return v < *this;
}

var& var::operator =(const var& v)
{
	return assign(v);
}

var& var::assign(const var& v)
{
	if(this == &v) return *this;
	vt newt = v.t;
	switch(v.t){
	  case t_bin:
		if(v.u.p){
			chunk* p = new chunk(*v.u.p);
			clear();
			u.p = p;
			if(u.p == null) newt = t_invalid;
		}else{
			clear();
			newt = t_invalid;
		}
		break;
	  case t_str:
		if(v.u.s){
			string* p = new string(*v.u.s);
			clear();
			u.s = p;
			if(u.s == null) newt = t_invalid;
		}else{
			clear();
			newt = t_invalid;
		}
		break;
	  case t_wstr:
		if(v.u.w){
			wstring* p = new wstring(*v.u.w);
			clear();
			u.w = p;
			if(u.w == null) newt = t_invalid;
		}else{
			clear();
			newt = t_invalid;
		}
		break;
	  case t_array:
		if(v.u.a){
			array* p = new array(*v.u.a);
			clear();
			u.a = p;
			if(u.a == null) newt = t_invalid;
		}else{
			clear();
			newt = t_invalid;
		}
		break;
	  case t_map:
		if(v.u.m){
			map* p = new map(*v.u.m);
			clear();
			u.m = p;
			if(u.m == null) newt = t_invalid;
		}else{
			clear();
			newt = t_invalid;
		}
		break;
	  case t_user:
		if(v.u.o){
			_userdata* p = new _userdata;
			*p = *v.u.o;
			clear();
			u.o = p;
			if(u.o == null) newt = t_invalid;
		}else{
			clear();
			newt = t_invalid;
		}
		break;
	  case t_invalid:
		clear();
		break;
	  default:
		uint64 tu = v.u.u8;
		clear();
		u.u8 = tu;
	}
	t = newt;
	return *this;
}

var& var::assign(const unsigned char* v)
{
	clear();
	t = t_str;
	u.p = new chunk(v?v:(const unsigned char*)"");
	if(u.p == null) t = t_invalid;
	return *this;
}

var& var::assign(const char* v)
{
	clear();
	t = t_str;
	u.s = new string(v?v:"");
	if(u.s == null) t = t_invalid;
	return *this;
}

var& var::assign(const wchar_t* v)
{
	clear();
	t = t_wstr;
	u.w = new wstring(v?v:L"");
	if(u.w == null) t = t_invalid;
	return *this;
}

var& var::assign(const chunk& v)
{
	clear();
	t = t_bin;
	u.p = new chunk(v);
	if(u.p == null) t = t_invalid;
	return *this;
}

var& var::assign(const string& v)
{
	clear();
	t = t_str;
	u.s = new string(v);
	if(u.s == null) t = t_invalid;
	return *this;
}

var& var::assign(const wstring& v)
{
	clear();
	t = t_wstr;
	u.w = new wstring(v);
	if(u.w == null) t = t_invalid;
	return *this;
}

var& var::assign(bool v)
{
	clear();
	t = t_bool;
	u.b = v;
	return *this;
}

var& var::assign(int8 v)
{
	clear();
	t = t_int8;
	u.i = v;
	return *this;
}

var& var::assign(uint8 v)
{
	clear();
	t = t_uint8;
	u.u = v;
	return *this;
}

var& var::assign(int16 v)
{
	clear();
	t = t_int16;
	u.i2 = v;
	return *this;
}

var& var::assign(uint16 v)
{
	clear();
	t = t_uint16;
	u.u2 = v;
	return *this;
}

var& var::assign(int32 v)
{
	clear();
	t = t_int32;
	u.i4 = v;
	return *this;
}

var& var::assign(uint32 v)
{
	clear();
	t = t_uint32;
	u.u4 = v;
	return *this;
}

var& var::assign(long v)
{
	assign((tempc<sizeof(long)>::t)v);
	return *this;
}

var& var::assign(unsigned long v)
{
	assign((tempc<sizeof(unsigned long)>::ut)v);
	return *this;
}

var& var::assign(int64 v)
{
	clear();
	t = t_int64;
	u.i8 = v;
	return *this;
}

var& var::assign(uint64 v)
{
	clear();
	t = t_uint64;
	u.u8 = v;
	return *this;
}

var& var::assign(double v)
{
	clear();
	t = t_double;
	u.f8 = v;
	return *this;
}

var& var::assign(float v)
{
	assign((double)v);
	return *this;
}

var& var::assign(const array& v)
{
	clear();
	t = t_array;
	u.a = new array();
	if(u.a == null){
		t = t_invalid;
		return *this;
	}
	*u.a = v;
	return *this;
}

var& var::assign(const lyramilk::data::var::map& v)
{
	clear();
	t = t_map;
	u.m = new map();
	if(u.m == null){
		t = t_invalid;
		return *this;
	}
	*u.m = v;
	return *this;
}

var& var::assign(const string& n,const void* v)
{
	clear();
	t = t_user;
	u.o = new _userdata;
	if(u.o == null){
		t = t_invalid;
		return *this;
	}
	u.o->operator[](n) = v;
	return *this;
}

/*var::operator chunk& () throw(type_invalid)
{
	if(t != t_bin) throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator chunk&()",type_name(t).c_str()));
	return *u.p;
}*/

var::operator chunk() const throw(type_invalid)
{
	switch(t){
	  case t_bin:{
			return *u.p;
		}break;
	  case t_str:{
			return chunk((const unsigned char*)u.s->c_str(),u.s->size());
		}break;
	  case t_wstr:{
			return chunk((const unsigned char*)u.w->c_str(),(u.w->size() << (int)tempc<sizeof(wchar_t)>::square));
		}break;
	  case t_bool:{
			return chunk((const unsigned char*)&u.b,sizeof(u.b));
		}break;
	  case t_int8:{
			return chunk((const unsigned char*)&u.i,sizeof(u.i));
		}break;
	  case t_uint8:{
			return chunk((const unsigned char*)&u.u,sizeof(u.u));
		}break;
	  case t_int16:{
			int16 i2 = reverse_order(u.i2);
			return chunk((const unsigned char*)&i2,sizeof(i2));
		}break;
	  case t_uint16:{
			uint16 u2 = reverse_order(u.u2);
			return chunk((const unsigned char*)&u2,sizeof(u2));
		}break;
	  case t_int32:{
			int32 i4 = reverse_order(u.i4);
			return chunk((const unsigned char*)&i4,sizeof(i4));
		}break;
	  case t_uint32:{
			uint32 u4 = reverse_order(u.u4);
			return chunk((const unsigned char*)&u4,sizeof(u4));
		}break;
	  case t_int64:{
			int64 i8 = reverse_order(u.i8);
			return chunk((const unsigned char*)&i8,sizeof(i8));
		}break;
	  case t_uint64:{
			uint64 u8 = reverse_order(u.u8);
			return chunk((const unsigned char*)&u8,sizeof(u8));
		}break;
	  case t_double:{
			return chunk((const unsigned char*)&u.f8,sizeof(u.f8));
		}break;
	  case t_array:
	  case t_map:
	  case t_user:
	  case t_invalid:{
			throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator chunk()",type_name(t).c_str()));
		}break;
	}
	throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator chunk()",type_name(t).c_str()));
}

/*var::operator string& () throw(type_invalid)
{
	if(t != t_str) throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator string&()",type_name(t).c_str()));
	return *u.s;
}*/

var::operator string () const throw(type_invalid)
{
	switch(t){
	  case t_bin:{
			return string((const char*)u.p->c_str(),u.p->size());
		}break;
	  case t_str:{
			return *u.s;
		}break;
	  case t_wstr:{
			return string(u2a(*u.w));
		}break;
	  case t_bool:{
			return u.b?"true":"false";
		}break;
	  case t_int8:{
			stringstream ss;
			ss << (unsigned int)u.i;
			return ss.str();
		}break;
	  case t_uint8:{
			stringstream ss;
			ss << (unsigned int)u.u;
			return ss.str();
		}break;
	  case t_int16:{
			stringstream ss;
			ss << (unsigned int)u.i2;
			return ss.str();
		}break;
	  case t_uint16:{
			stringstream ss;
			ss << (unsigned int)u.u2;
			return ss.str();
		}break;
	  case t_int32:{
			stringstream ss;
			ss << u.i4;
			return ss.str();
		}break;
	  case t_uint32:{
			stringstream ss;
			ss << u.u4;
			return ss.str();
		}break;
	  case t_int64:{
			stringstream ss;
			ss << u.i8;
			return ss.str();
		}break;
	  case t_uint64:{
			stringstream ss;
			ss << u.u8;
			return ss.str();
		}break;
	  case t_double:{
			stringstream ss;
			ss << u.f8;
			return ss.str();
		}break;
	  case t_array:{
			//if(u.a->size() == 1) return u.a->at(0);
			array::const_iterator it = u.a->begin();
			string str = "[";
			if(it != u.a->end()){
				str += (string)*it;
				for(++it;it!=u.a->end();++it){
					str += ",";
					str += (string)*it;
				}
			}
			str += "]";
			return str;
		}break;
	  case t_map:{
			string str = "{";
			map::const_iterator it = u.m->begin();
			if(it != u.m->end()){
				str += (string)it->first + ":" + (string)it->second;
				for(++it;it!=u.m->end();++it){
					str += "," + (string)it->first + ":" + (string)it->second;
				}
			}
			str += "}";
			return str;
		}break;
	  case t_user:{
			//throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator string()",type_name(t).c_str()));
			return "";
		}break;
	  case t_invalid:{
			//throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator string()",type_name(t).c_str()));
			return "";
		}break;
	}
	throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator string()",type_name(t).c_str()));
}

/*var::operator wstring& () throw(type_invalid)
{
	if(t != t_wstr) throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator wstring&()",type_name(t).c_str()));
	return *u.w;
}*/

var::operator wstring () const throw(type_invalid)
{
	switch(t){
	  case t_bin:{
			return wstring((const wchar_t*)u.p->c_str(),(u.p->size() << (int)tempc<sizeof(wchar_t)>::square));
		}break;
	  case t_str:{
			return a2u(*u.s);
		}break;
	  case t_wstr:{
			return *u.w;
		}break;
	  case t_bool:{
			return u.b?L"true":L"false";
		}break;
	  case t_int8:{
			wstringstream ss;
			ss << (unsigned int)u.i;
			return ss.str();
		}break;
	  case t_uint8:{
			wstringstream ss;
			ss << (unsigned int)u.u;
			return ss.str();
		}break;
	  case t_int16:{
			wstringstream ss;
			ss << (unsigned int)u.i2;
			return ss.str();
		}break;
	  case t_uint16:{
			wstringstream ss;
			ss << (unsigned int)u.u2;
			return ss.str();
		}break;
	  case t_int32:{
			wstringstream ss;
			ss << u.i4;
			return ss.str();
		}break;
	  case t_uint32:{
			wstringstream ss;
			ss << u.u4;
			return ss.str();
		}break;
	  case t_int64:{
			wstringstream ss;
			ss << u.i8;
			return ss.str();
		}break;
	  case t_uint64:{
			wstringstream ss;
			ss << u.u8;
			return ss.str();
		}break;
	  case t_double:{
			wstringstream ss;
			ss << u.f8;
			return ss.str();
		}break;
	  case t_array:{
			//if(u.a->size() == 1) return u.a->at(0);
			array::const_iterator it = u.a->begin();
			wstring str = L"[";
			if(it != u.a->end()){
				str += (wstring)*it;
				for(++it;it!=u.a->end();++it){
					str += L",";
					str += (wstring)*it;
				}
			}
			str += L"]";
			return str;
		}break;
	  case t_map:{
			wstring str = L"{";
			map::const_iterator it = u.m->begin();
			if(it != u.m->end()){
				wstring strfirst = var(it->first);
				str += strfirst + L":" + (wstring)it->second;
				for(++it;it!=u.m->end();++it){
					wstring strfirst = var(it->first);
					str += L"," + strfirst + L":" + (wstring)it->second;
				}
			}
			str += L"}";
			return str;
		}break;
	  case t_user:{
			return L"";
		}break;
	  case t_invalid:{
			return L"";
		}break;
	}
	throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator wstring()",type_name(t).c_str()));
 }

var::operator bool () const throw(type_invalid)
{
	switch(t){
	  case t_bin:{
			throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator bool()",type_name(t).c_str()));
		}break;
	  case t_str:{
			string w = *u.s;
			std::transform(u.s->begin(),u.s->end(),w.begin(),tolower);
			if(w.compare("true") == 0) return true;
			if(w.compare("false") == 0) return false;
			throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator bool()",type_name(t).c_str()));
		}break;
	  case t_wstr:{
			wstring w = *u.w;
			std::transform(u.w->begin(),u.w->end(),w.begin(),towlower);
			if(w.compare(L"true") == 0) return true;
			if(w.compare(L"false") == 0) return false;
			throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator bool()",type_name(t).c_str()));
		}break;
	  case t_bool:
	  case t_int8:
	  case t_uint8:
	  case t_int16:
	  case t_uint16:
	  case t_int32:
	  case t_uint32:
	  case t_int64:
	  case t_uint64:
	  case t_double:{
			return (int64)*this != 0;
		}break;
	  case t_array:{
			throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator bool()",type_name(t).c_str()));
		}break;
	  case t_map:{
			throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator bool()",type_name(t).c_str()));
		}break;
	  case t_user:{
			throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator bool()",type_name(t).c_str()));
		}break;
	  case t_invalid:{
			throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator bool()",type_name(t).c_str()));
		}break;
	}
	throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator bool()",type_name(t).c_str()));
}

var::operator int8 () const throw(type_invalid)
{
	switch(t){
	  case t_bin:
	  case t_str:
	  case t_wstr:
	  case t_bool:
	  case t_int8:
	  case t_uint8:
	  case t_int16:
	  case t_uint16:
	  case t_int32:
	  case t_uint32:
	  case t_int64:
	  case t_uint64:
	  case t_double:{
			return (int8)(int64)*this;
		}break;
	  case t_array:
	  case t_map:
	  case t_user:
	  case t_invalid:{
			throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator int8()",type_name(t).c_str()));
		}break;
	}
	throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator int8()",type_name(t).c_str()));
}

var::operator uint8 () const throw(type_invalid)
{
	switch(t){
	  case t_bin:
	  case t_str:
	  case t_wstr:
	  case t_bool:
	  case t_int8:
	  case t_uint8:
	  case t_int16:
	  case t_uint16:
	  case t_int32:
	  case t_uint32:
	  case t_int64:
	  case t_uint64:
	  case t_double:{
			return (uint8)(int64)*this;
		}break;
	  case t_array:
	  case t_map:
	  case t_user:
	  case t_invalid:{
			throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator uint8()",type_name(t).c_str()));
		}break;
	}
	throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator uint8()",type_name(t).c_str()));
}

var::operator int16 () const throw(type_invalid)
{
	switch(t){
	  case t_bin:
	  case t_str:
	  case t_wstr:
	  case t_bool:
	  case t_int8:
	  case t_uint8:
	  case t_int16:
	  case t_uint16:
	  case t_int32:
	  case t_uint32:
	  case t_int64:
	  case t_uint64:
	  case t_double:{
			return (int16)(int64)*this;
		}break;
	  case t_array:
	  case t_map:
	  case t_user:
	  case t_invalid:{
			throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator int16()",type_name(t).c_str()));
		}
	}
	throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator int16()",type_name(t).c_str()));
}

var::operator uint16 () const throw(type_invalid)
{
	switch(t){
	  case t_bin:
	  case t_str:
	  case t_wstr:
	  case t_bool:
	  case t_int8:
	  case t_uint8:
	  case t_int16:
	  case t_uint16:
	  case t_int32:
	  case t_uint32:
	  case t_int64:
	  case t_uint64:
	  case t_double:{
			return (uint16)(int64)*this;
		}break;
	  case t_array:
	  case t_map:
	  case t_user:
	  case t_invalid:{
			throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator uint16()",type_name(t).c_str()));
		}
	}
	throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator uint16()",type_name(t).c_str()));
}

var::operator int32 () const throw(type_invalid)
{
	switch(t){
	  case t_bin:
	  case t_str:
	  case t_wstr:
	  case t_bool:
	  case t_int8:
	  case t_uint8:
	  case t_int16:
	  case t_uint16:
	  case t_int32:
	  case t_uint32:
	  case t_int64:
	  case t_uint64:
	  case t_double:{
			return (int32)(int64)*this;
		}break;
	  case t_array:
	  case t_map:
	  case t_user:
	  case t_invalid:{
			throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator int32()",type_name(t).c_str()));
		}
	}
	throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator int32()",type_name(t).c_str()));
}

var::operator uint32 () const throw(type_invalid)
{
	return (uint32)(int32)*this;
}

var::operator long () const throw(type_invalid)
{
	return (tempc<sizeof(long)>::t)*this;
}

var::operator unsigned long () const throw(type_invalid)
{
	return (tempc<sizeof(unsigned long)>::ut)*this;
}

var::operator int64 () const throw(type_invalid)
{
	switch(t){
	  case t_bin:{
			int64 value = 0;
			string str((const char*)u.p->c_str(),u.p->size());
			istringstream is(str);
			is >> value;
			return value;
		}break;
	  case t_str:{
			int64 value = 0;
			istringstream is(*u.s);
			is >> value;
			return value;
		}break;
	  case t_wstr:{
			int64 value = 0;
			wistringstream is(*u.w);
			is >> value;
			return value;
		}break;
	  case t_bool:{
			return u.b;
		}break;
	  case t_int8:{
			return u.i;
		}break;
	  case t_uint8:{
			return u.u;
		}break;
	  case t_int16:{
			return u.i2;
		}break;
	  case t_uint16:{
			return u.u2;
		}break;
	  case t_int32:{
			return u.i4;
		}break;
	  case t_uint32:{
			return u.u4;
		}break;
	  case t_int64:{
			return u.i8;
		}break;
	  case t_uint64:{
			return u.u8;
		}break;
	  case t_double:{
			return (int64)u.f8;
		}break;
	  case t_array:
	  case t_map:
	  case t_user:
	  case t_invalid:{
			throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator int64()",type_name(t).c_str()));
		}break;
	}
	throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator int64()",type_name(t).c_str()));
}

var::operator uint64 () const throw(type_invalid)
{
	return (uint64)(int64)*this;
}


var::operator double () const throw(type_invalid)
{
	switch(t){
	  case t_bin:{
			double value = 0.0f;
			string str((const char*)u.p->c_str(),u.p->size());
			istringstream is(str);
			is >> value;
			return value;
		}break;
	  case t_str:{
			double value = 0.0f;
			istringstream is(*u.s);
			is >> value;
			return value;
		}break;
	  case t_wstr:{
			double value = 0.0f;
			wistringstream is(*u.w);
			is >> value;
			return value;
		}break;
	  case t_bool:{
			return u.b;
		}break;
	  case t_int8:{
			return u.i;
		}break;
	  case t_uint8:{
			return u.u;
		}break;
	  case t_int16:{
			return u.i2;
		}break;
	  case t_uint16:{
			return u.u2;
		}break;
	  case t_int32:{
			return u.i4;
		}break;
	  case t_uint32:{
			return u.u4;
		}break;
	  case t_int64:{
			return (double)u.i8;
		}break;
	  case t_uint64:{
			return (double)u.u8;
		}break;
	  case t_double:{
			return u.f8;
		}break;
	  case t_array:
	  case t_map:
	  case t_user:
	  case t_invalid:{
			throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator double()",type_name(t).c_str()));
		}break;
	}
	throw type_invalid(lyramilk::kdict("%s 错误：错误的子类型%s","lyramilk::data::var::operator double()",type_name(t).c_str()));
}

var::operator float () const throw(type_invalid)
{
	return (float)(double)*this;
}

var::operator lyramilk::data::var::array& () throw(type_invalid)
{
	if(t == t_array) return *u.a;
	throw type_invalid(lyramilk::kdict("%s 错误：%s类型无法转换为%s类型","lyramilk::data::var::operator var::array()",type_name(t).c_str(),type_name(t_array).c_str()));
}

var::operator const lyramilk::data::var::array& () const throw(type_invalid)
{
	if(t == t_array) return *u.a;
	throw type_invalid(lyramilk::kdict("%s 错误：%s类型无法转换为%s类型","lyramilk::data::var::operator var::array()",type_name(t).c_str(),type_name(t_array).c_str()));
}

var::operator var::map& ()  throw(type_invalid)
{
	if(t == t_map) return *u.m;
	throw type_invalid(lyramilk::kdict("%s 错误：%s类型无法转换为%s类型","lyramilk::data::var::operator var::map()",type_name(t).c_str(),type_name(t_map).c_str()));
}

var::operator const lyramilk::data::var::map& () const throw(type_invalid)
{
	if(t == t_map) return *u.m;
	throw type_invalid(lyramilk::kdict("%s 错误：%s类型无法转换为%s类型","lyramilk::data::var::operator var::map()",type_name(t).c_str(),type_name(t_map).c_str()));
}

void lyramilk::data::var::userdata(string v,const void* p) throw(type_invalid)
{
	if(t != t_user)	throw type_invalid(lyramilk::kdict("%s 错误：%s类型无法赋予用户数据","lyramilk::data::var::operator var::map()",type_name(t).c_str()));
	u.o->operator[](v) = p;
}

const void* lyramilk::data::var::userdata(string v) const
{
	if(t != t_user)	return null;
	_userdata::iterator it = u.o->find(v);
	if(it == u.o->end()) return null;
	return it->second;
}

const void* lyramilk::data::var::userdata() const
{
	if(t != t_user)	return null;
	_userdata::iterator it = u.o->begin();
	if(it == u.o->end()) return null;
	return it->second;
}

var& var::operator[](const char* index)
{
	return at(index);
}

var& var::operator[](const wchar_t* index)
{
	return at(index);
}

var& var::operator[](const string& index)
{
	return at(index);
}

var& var::operator[](const wstring& index)
{
	return at(index);
}

var& var::operator[](bool index)
{
	return at(index);
}

var& var::operator[](int8 index)
{
	return at(index);
}

var& var::operator[](uint8 index)
{
	return at(index);
}

var& var::operator[](int16 index)
{
	return at(index);
}

var& var::operator[](uint16 index)
{
	return at(index);
}

var& var::operator[](int32 index)
{
	return at(index);
}

var& var::operator[](uint32 index)
{
	return at(index);
}

var& var::operator[](long index)
{
	return at(index);
}

var& var::operator[](int64 index)
{
	return at((lyramilk::data::uint32)index);
}

var& var::operator[](uint64 index)
{
	return at((lyramilk::data::uint32)index);
}

var& var::operator[](double index)
{
	return at((lyramilk::data::uint32)index);
}

const char* var::c_str() const throw(type_invalid)
{
	return ((string)(*this)).c_str();
}

const wchar_t* var::c_wstr() const throw(type_invalid)
{
	return ((wstring)(*this)).c_str();
}

var::vt var::type() const
{
	return t;
}

string var::type_name(vt nt)
{
	switch(nt){
	  case t_bin:
		return "t_bin";
	  case t_str:
		return "t_str";
	  case t_wstr:
		return "t_wstr";
	  case t_bool:
		return "t_bool";
	  case t_int8:
		return "t_int8";
	  case t_uint8:
		return "t_uint8";
	  case t_int16:
		return "t_int16";
	  case t_uint16:
		return "t_uint16";
	  case t_int32:
		return "t_int32";
	  case t_uint32:
		return "t_uint32";
	  case t_int64:
		return "t_int64";
	  case t_uint64:
		return "t_uint64";
	  case t_double:
		return "t_double";
	  case t_array:
		return "t_array";
	  case t_map:
		return "t_map";
	  case t_user:
		return "t_user";
	  case t_invalid:
		return "t_invalid";
	  default:
		return "t_unknow ";
	}
}

string var::type_name() const
{
	return type_name(t);
}

var& var::type(var::vt nt) throw(type_invalid)
{
	if(t == nt) return *this;
	string* s = u.s;
	wstring* w = u.w;
	array* a = u.a;
	map* m = u.m;
	chunk* p = u.p;
	switch(nt){
	  case t_bin:{
			u.p = new chunk((const chunk)*this);
			if(u.p == null) t = t_invalid;
		}break;
	  case t_str:{
			u.s = new string((const string)*this);
			if(u.s == null) t = t_invalid;
		}break;
	  case t_wstr:{
			u.w = new wstring((const wstring)*this);
			if(u.w == null) t = t_invalid;
		}break;
	  case t_bool:{
			u.b = *this;
		}break;
	  case t_int8:{
			u.i = *this;
		}break;
	  case t_uint8:{
			u.u = *this;
		}break;
	  case t_int16:{
			u.i2 = *this;
		}break;
	  case t_uint16:{
			u.u2 = *this;
		}break;
	  case t_int32:{
			u.i4 = *this;
		}break;
	  case t_uint32:{
			u.u4 = *this;
		}break;
	  case t_int64:{
			u.i8 = *this;
		}break;
	  case t_uint64:{
			u.u8 = *this;
		}break;
	  case t_double:{
			u.f8 = *this;
		}break;
	  case t_array:{
			array* na = new array();
			if(type_compat(t_array)){
				*na = *this;
			}
			u.a = na;
		}break;
	  case t_map:{
			map* nm = new map();
			if(type_compat(t_map)){
				*nm = *this;
			}
			u.m = nm;
		}break;
	  case t_user:{
			if(t != nt) throw type_invalid(lyramilk::kdict("%s 错误：错误的新类型%d","lyramilk::data::var::operator var::type()",nt));
		}break;
	  case t_invalid:
		break;
	  default:
		throw type_invalid(lyramilk::kdict("%s 错误：错误的新类型%d","lyramilk::data::var::operator var::type()",nt));
	}

	switch(t){
	  case t_bin:
		delete p;
		break;
	  case t_str:
		delete s;
		break;
	  case t_wstr:
		delete w;
		break;
	  case t_bool:
	  case t_int8:
	  case t_uint8:
	  case t_int16:
	  case t_uint16:
	  case t_int32:
	  case t_uint32:
	  case t_int64:
	  case t_uint64:
	  case t_double:
	  case t_array:
		delete a;
		break;
	  case t_map:
		delete m;
		break;
	  case t_user:
		break;
	  case t_invalid:
		break;
	}

	t = nt;
	return *this;
}

bool var::type_compat(vt nt)
{
	if((nt == t_bin || nt == t_str || nt == t_wstr) && (t == t_bin || t == t_str || t == t_wstr || t == t_int8 ||  t == t_uint8 ||  t == t_int16 ||  t == t_uint16 ||  t == t_int32 ||  t == t_uint32 ||  t == t_int64 ||  t == t_uint64 ||  t == t_double)){
		return true;
	}

	if((nt == t_int8 || nt == t_uint8 || nt == t_int16 || nt == t_uint16 || nt == t_int32 || nt == t_uint32 || nt == t_int64 || nt == t_uint64 || nt == t_double) &&
		(t == t_int8 ||  t == t_uint8 ||  t == t_int16 ||  t == t_uint16 ||  t == t_int32 ||  t == t_uint32 ||  t == t_int64 ||  t == t_uint64 ||  t == t_double)){
		return true;
	}

	if(nt == t_user && t == t_user){
		return true;
	}
	if(nt != t_invalid && nt == t) return true;
	return false;
}

var::array::size_type var::size() const throw(type_invalid)
{
	if(t == t_array){
		return u.a->size();
	}
	if(t == t_map){
		return u.m->size();
	}
	if(t == t_bin){
		return u.p->size();
	}
	if(t == t_str){
		return u.s->size();
	}
	if(t == t_wstr){
		return u.w->size();
	}
	throw type_invalid(lyramilk::kdict("%s 错误：不可获取size的子类型","lyramilk::data::var::size()"));
}

lyramilk::data::string var::str() const
{
	return *this;
}

void var::clear()
{
	if(t == t_invalid){
		return;
	}

	switch(t){
	  case t_bool:
	  case t_int8:
	  case t_uint8:
	  case t_int16:
	  case t_uint16:
	  case t_int32:
	  case t_uint32:
	  case t_int64:
	  case t_uint64:
	  case t_double:
	  case t_invalid:
		break;
	  case t_bin:
		delete u.p;
		u.p = null;
		break;
	  case t_str:
		delete u.s;
		u.s = null;
		break;
	  case t_wstr:
		delete u.w;
		u.w = null;
		break;
	  case t_array:
		delete u.a;
		u.a = null;
		break;
	  case t_map:
		delete u.m;
		u.m = null;
		break;
	  case t_user:
		delete u.o;
		u.o = null;
		break;
	}
	t = t_invalid;
}

/* 序列化相关 */

bool is_igendian()
{
	union{
		short s;
		char c[1];
	}u;
	u.s = 1;
	return u.c[0] == 0;
}

const bool g_igendian = is_igendian();

typedef unsigned short array_size_type;
typedef unsigned int string_size_type;

template <typename T>
void write(ostream& os,T& t)
{
	os.write((const char*)&t,sizeof(T));
}

template <typename T>
bool read(istream& is,T& t)
{
	is.read((char*)&t,sizeof(T));
	return is.gcount() == sizeof(T);
}

bool var::_serialize(ostream& os) const throw(type_invalid)
{
	if((t&0x7f) != t) throw type_invalid(lyramilk::kdict("%s 错误：不支持的类型%d","lyramilk::data::var::serialize()",(t&0x7f)));
	unsigned char m;
	m = g_igendian<<7;
	m |= t&0x7f;

	switch(t){
	  case t_bin:{
			chunk& binstr = *u.p;
			string_size_type size = (string_size_type)binstr.size();
			write(os,m);
			write(os,size);
			os.write((const char*)binstr.c_str(),size);
			return true;
		}break;
	  case t_str:{
			string& localstr = *u.s;
			string utf8str = a2t(localstr);
			string_size_type size = (string_size_type)utf8str.size();
			write(os,m);
			write(os,size);
			os.write(utf8str.c_str(),size);
			return true;
		}break;
	  case t_wstr:{
			string utf8str = u2t(*u.w);
			string_size_type size = (string_size_type)utf8str.size();
			write(os,m);
			write(os,size);
			os.write(utf8str.c_str(),size);
			return true;
		}break;
	  case t_bool:
	  case t_int8:
	  case t_uint8:{
			write(os,m);
			write(os,u.u);
			return true;
		}break;
	  case t_int16:
	  case t_uint16:{
			write(os,m);
			write(os,u.u2);
			return true;
		}break;
	  case t_int32:
	  case t_uint32:{
			write(os,m);
			write(os,u.u4);
			return true;
		}break;
	  case t_int64:
	  case t_uint64:{
			write(os,m);
			write(os,u.u8);
			return true;
		}break;
	  case t_double:{
			write(os,m);
			write(os,u.f8);
			return true;
		}break;
	  case t_array:{
			array_size_type size = (array_size_type)u.a->size();
			write(os,m);
			write(os,size);
			array::iterator it = u.a->begin();
			for(;it != u.a->end();++it){
				if(!it->_serialize(os)) return false;
			}
			return true;
		}break;
	  case t_map:{
			array_size_type size = (array_size_type)u.m->size();
			write(os,m);
			write(os,size);
			map::iterator it = u.m->begin();
			for(;it != u.m->end();++it){
				{
					string utf8str = a2t(it->first);
					string_size_type size = (string_size_type)utf8str.size();
					unsigned char mstr = (g_igendian<<7)|(t_str&0x7f);
					write(os,mstr);
					write(os,size);
					os.write((const char*)utf8str.c_str(),size);
				}
				if(!it->second._serialize(os)) return false;
			}
			return true;
		}break;
	  case t_user:
		return true;
	  case t_invalid:
		return false;
	}
	throw type_invalid(lyramilk::kdict("%s 错误：不支持的类型%d","lyramilk::data::var::serialize()",(t&0x7f)));
}

bool var::_deserialize(istream& is)
{
	unsigned char m;
	if(!read(is,m)) return false;
	vt ts = (vt)(m&0x7f);
	bool r = (g_igendian?1:0) != (m>>7);

	switch(ts){
	  case t_bin:{
			string_size_type size = 0;
			if(!read(is,size)) return false;
			if(r) size = reverse_order(size);
			chunk binstr;
			binstr.resize(size);
			is.read((char*)binstr.c_str(),size);
			if(is.gcount() == size){
				assign(binstr);
				return true;
			}
			return false;
		}break;
	  case t_str:{
			string_size_type size = 0;
			if(!read(is,size)) return false;
			if(r) size = reverse_order(size);
			string utf8str;
			utf8str.resize(size);
			is.read((char*)utf8str.c_str(),size);
			if(is.gcount() == size){
				string localstr = t2a(utf8str);
				assign(localstr);
				return true;
			}
			return false;
		}break;
	  case t_wstr:{
			string_size_type size = 0;
			if(!read(is,size)) return false;
			if(r) size = reverse_order(size);
			string utf8str;
			utf8str.resize(size);
			is.read((char*)utf8str.c_str(),size);
			if(is.gcount() == size){
				string localstr = t2a(utf8str);
				assign(wstring(a2u(localstr)));
				return true;
			}
			return false;
		}break;
	  case t_bool:
	  case t_int8:
	  case t_uint8:{
			if(!read(is,u.u)) return false;
			t = ts;
			return true;
		}break;
	  case t_int16:
	  case t_uint16:{
			if(!read(is,u.u2)) return false;
			if(r) u.u2 = reverse_order(u.u2);
			t = ts;
			return true;
		}break;
	  case t_int32:
	  case t_uint32:{
			if(!read(is,u.u4)) return false;
			if(r) u.u4 = reverse_order(u.u4);
			t = ts;
			return true;
		}break;
	  case t_int64:
	  case t_uint64:{
			if(!read(is,u.u8)) return false;
			if(r) u.u8 = reverse_order(u.u8);
			t = ts;
			return true;
		}break;
	  case t_double:{
			if(!read(is,u.f8)) return false;
			t = ts;
			return true;
		}break;
	  case t_array:{
			array_size_type size = 0;
			if(!read(is,size)) return false;
			if(r) size = reverse_order(size);
			type(t_array);
			for(array_size_type i=0;i<size;++i){
				var d;
				if(!d._deserialize(is)){
					clear();
					return false;
				}
				u.a->push_back(d);
			}
			t = ts;
			return true;
		}break;
	  case t_map:{
			array_size_type size = 0;
			if(!read(is,size)) return false;
			if(r) size = reverse_order(size);
			type(t_map);
			for(array_size_type i=0;i<size;++i){
				var key;
				var value;
				if(!(key._deserialize(is) && value._deserialize(is))){
					clear();
					return false;
				}
				u.m->operator[](key) = value;
			}
			t = ts;
			return true;
		}break;
	  case t_user:
		return true;
	  case t_invalid:
		return false;
	}
	return false;
}

bool var::serialize(ostream& os) const throw(type_invalid)
{
	ostream::streamoff bpos = os.tellp();
	int32 size = 0;
	write(os,size);
	bool ret = _serialize(os);
	if(ret){
		ostream::streamoff epos = os.tellp();
		os.seekp(bpos,ostream::beg);
		size = (int32)(epos - bpos - sizeof(size));
		if(size > 0){
			size = htonl(size);
			write(os,size);
			os.seekp(epos,ostream::beg);
			if(os.good()) return true;
		}
	}
	os.clear();
	os.seekp(bpos,ostream::beg);
	return false;
}

bool var::deserialize(istream& is)
{
	istream::streamoff bpos = is.tellg();
	is.seekg(0,istream::end);
	istream::streamoff epos = is.tellg();
	is.seekg(bpos,istream::beg);

	int32 objsize = 0;
	if(read(is,objsize)){
		objsize = ntohl(objsize);
		int32 realsize = (int32)(epos - bpos - sizeof(objsize));
		if(objsize <= realsize){
			if(_deserialize(is)){
				return true;
			}
		}
	}

	is.clear();
	is.seekg(bpos,istream::beg);
	return false;
}

void var::dump(ostream& os) const
{
	os << str() << std::endl;
}

std::vector<string> static split(string data,string sep)
{
	std::vector<string> lines;
	std::size_t posb = 0;
	do{
		std::size_t poscrlf = data.find(sep,posb);
		if(poscrlf == data.npos){
			string newline = data.substr(posb);
			posb = poscrlf;
			lines.push_back(newline);
		}else{
			string newline = data.substr(posb,poscrlf - posb);
			posb = poscrlf + sep.size();
			lines.push_back(newline);
		}
	}while(posb != data.npos);
	return lines;
}

std::vector<string> static pathof(string varpath) throw(var::type_invalid)
{
	std::vector<string> ret;
	std::vector<string> v = split(varpath,"/");
	std::vector<string>::iterator it = v.begin();
	if(it==v.end()) return ret;
	while(it!=v.end()){
		if(it->compare(".") == 0 || it->empty()){
			++it;
			continue;
		}
		ret.push_back(*it);
		break;
	}
	for(++it;it!=v.end();++it){
		if(it->compare(".") == 0 || it->empty()) continue;
		if(it->compare("..") == 0 && !ret.empty()){
			ret.pop_back();
			continue;
		}
		ret.push_back(*it);
	}
	return ret;
}


var& var::path(string varpath) throw(type_invalid)
{
	std::vector<string> fields = pathof(varpath);

	//如果前面的回退无法清除，说明想要的值越过了根，不允许。
	if(!fields.empty() && fields.at(0).compare("..") == 0){
		throw type_invalid(lyramilk::kdict("%s 路径错误，试图越过了根结点：%s","lyramilk::data::var::path()",varpath.c_str()));
	}

	//如果路径表达式为空或路径表达式只表达一个目录，则直接返回根。
	if(fields.size() == 0){
		return *this;
	}

	//此时的fields中包含且仅包含枝节点。
	std::vector<string>::iterator it = fields.begin();
	//如果var是空的
	var* p = this;
	/*
	if(p->type() == t_invalid){
		p->type(t_map);
	}
	if((p->type() & (t_map | t_array)) == 0) throw type_invalid(lyramilk::kdict("%s 路径错误：根元素不是t_map或t_array(%s)","lyramilk::data::var::path()",varpath.c_str()));
*/
	for(;it!=fields.end();++it){
		if(p->type() == t_array){
			string& str = *it;
			if(str.find_first_not_of("0123456789") != str.npos){
				throw type_invalid(lyramilk::kdict("%s 路径错误：t_array类型只能接收纯整数的字符串形式(%s)","lyramilk::data::var::path()",varpath.c_str()));
			}
			unsigned int index = atoi(str.c_str());
			if(p->u.a->size() == index){
				p->u.a->push_back(nil);
			}else if(p->u.a->size() < index + 1){
				throw type_invalid(lyramilk::kdict("%s 路径错误：t_array类型越界(%s)","lyramilk::data::var::path()",varpath.c_str()));
			}
			p = &p->u.a->at(index);
		}else if(p->type() == t_map){
			p = &p->u.m->operator[](*it);
		}else if(p->type() == t_invalid){
			string& str = *it;
			if(str.find_first_not_of("0123456789") == str.npos){
				unsigned int index = atoi(str.c_str());
				p->type(t_array);
				if(p->u.a->size() == index){
					p->u.a->push_back(nil);
					p = &p->u.a->back();
				}else if(p->u.a->size() < index - 1){
					p = &p->u.a->operator[](index);
				}
			}else{
				p->type(t_map);
				p = &p->u.m->operator[](str);
			}
		}else{
			throw type_invalid(lyramilk::kdict("%s 路径错误：%s","lyramilk::data::var::path()",varpath.c_str()));
		}
	}
	return *p;
}

std::ostream& operator << (std::ostream& os,const lyramilk::data::var& t)
{
	return os << t.str();
}

std::istream& operator >> (std::istream& is, lyramilk::data::var& t)
{
	char buff[4096];
	string str;
	while(is){
		is.read(buff,4096);
		str.append(buff,(unsigned int)is.gcount());
	}
	t = str;

	return is;
}

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
