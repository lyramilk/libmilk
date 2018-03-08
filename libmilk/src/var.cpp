#include "var.h"
#include "multilanguage.h"
#include <vector>
#include <sstream>
#include <algorithm>
#include <stdio.h>
#include <memory.h>

#ifndef null
#define null nullptr
#endif
#ifdef Z_HAVE_JEMALLOC
	#include <jemalloc/jemalloc.h>
#endif


#ifdef _WIN32
	#include <Windows.h>
#elif defined __linux__
	#include <iconv.h>
	#include <assert.h>
	#include <arpa/inet.h>
#endif

std::locale::id id;

namespace lyramilk { namespace data{

const var var::nil;
var::_userdata var::__reserve0;

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
		char c[sizeof(T)];
	}s,d;
	s.v = t;
	for(int i =0;i < sz;++i){
		d.c[i] = s.c[sz - i - 1];
	}
	return d.v;
}

#ifdef _WIN32
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
	static lyramilk::data::string iconv(const lyramilk::data::string& str,const lyramilk::data::string& from,const lyramilk::data::string& to)
	{
		const size_t ds = 4096;
		iconv_t cd = iconv_open(to.c_str(),from.c_str());
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
		return lyramilk::data::string(ret.data(),p2 - (char*)ret.data());
	}

	wstring utf8_unicode(string str)
	{
		string dst = iconv(str,"utf8//ignore","wchar_t");
		if(dst.size()&1){
			dst.push_back(0);
		}
		return wstring((wchar_t*)dst.c_str(),dst.size() >> tempc<sizeof(wchar_t)>::square);
	}

	string unicode_utf8(wstring str)
	{
		string src((char*)str.c_str(),str.size() << tempc<sizeof(wchar_t)>::square);
		string dst = iconv(src,"wchar_t","utf8");
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

void* milk_malloc(size_t size)
{
	return ::malloc(size);
}

void milk_free(void* p, size_t size)
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
}

var::var(const var& v)
{
	t = t_invalid;
	assign(v);
}

var::~var()
{
	clear();
}

var::var(const unsigned char* v)
{
	t = t_invalid;
	assign(v);
}

var::var(const char* v)
{
	t = t_invalid;
	assign(v);
}

var::var(const wchar_t* v)
{
	t = t_invalid;
	assign(v);
}

var::var(const chunk& v)
{
	t = t_invalid;
	assign(v);
}

var::var(const string& v)
{
	t = t_invalid;
	assign(v);
}

var::var(const wstring& v)
{
	t = t_invalid;
	assign(v);
}

var::var(bool v)
{
	t = t_invalid;
	assign(v);
}

var::var(int8 v)
{
	t = t_invalid;
	assign(v);
}

var::var(uint8 v)
{
	t = t_invalid;
	assign(v);
}

var::var(int16 v)
{
	t = t_invalid;
	assign(v);
}

var::var(uint16 v)
{
	t = t_invalid;
	assign(v);
}

var::var(int32 v)
{
	t = t_invalid;
	assign(v);
}

var::var(uint32 v)
{
	t = t_invalid;
	assign(v);
}

var::var(long v)
{
	t = t_invalid;
	assign(v);
}

var::var(unsigned long v)
{
	t = t_invalid;
	assign(v);
}

var::var(int64 v)
{
	t = t_invalid;
	assign(v);
}

var::var(uint64 v)
{
	t = t_invalid;
	assign(v);
}

var::var(double v)
{
	t = t_invalid;
	assign(v);
}

var::var(float v)
{
	t = t_invalid;
	assign(v);
}

var::var(const array& v)
{
	t = t_invalid;
	assign(v);
}

var::var(const map& v)
{
	t = t_invalid;
	assign(v);
}

var::var(const string& n,const void* v)
{
	t = t_invalid;
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
			chunk* bp = reinterpret_cast<chunk*>(&u.bp);
			*bp += (chunk)v;
		}break;
	  case t_str:{
			string* bs = reinterpret_cast<string*>(&u.bs);
			*bs += (string)v;
		}break;
	  case t_wstr:{
			wstring* bw = reinterpret_cast<wstring*>(&u.bw);
			*bw += (wstring)v;
		}break;
	  case t_bool:{
			u.b |= (bool)v;
		}break;
	  case t_int:{
			u.i8 += (int64)v;
		}break;
	  case t_uint:{
			u.u8 += (uint64)v;
		}break;
	  case t_double:{
			u.f8 += (double)v;
		}break;
	  case t_array:{
			array* ba = reinterpret_cast<array*>(&u.ba);
			if(v.t == t_array){
				const array* vba = reinterpret_cast<const array*>(&v.u.ba);
				ba->insert(ba->end(),vba->begin(),vba->end());
			}else{
				ba->push_back(v);
			}
		}break;
	  case t_map:{
			if(v.t == t_map){
				map* bm = reinterpret_cast<map*>(&u.bm);
				const map* vbm = reinterpret_cast<const map*>(&v.u.bm);
				map::const_iterator it = vbm->begin();
				for(;it!=vbm->end();++it){
					bm->insert(*it);
				}
			}else{
				throw type_invalid(lyramilk::kdict("%s：t_map类型不能追加其它类型","lyramilk::data::var::operator +=()"));
			}
		}break;
	  case t_invalid:{
			assign(v);
		}break;
	  default:
		throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator +=()",type_name(t).c_str()));
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
				const chunk* bp = reinterpret_cast<const chunk*>(&u.bp);
				return bp->compare((chunk)v) == 0;
			}
		}break;
	  case t_str:{
			if(v.t == t_bool){
				return ((bool)*this) == v.u.b;
			}else{
				const string* bs = reinterpret_cast<const string*>(&u.bs);
				return bs->compare((string)v) == 0;
			}
		}break;
	  case t_wstr:{
			if(v.t == t_bool){
				return ((bool)*this) == v.u.b;
			}else{
				const wstring* bw = reinterpret_cast<const wstring*>(&u.bw);
				return bw->compare((wstring)v) == 0;
			}
		}break;
	  case t_bool:{
			return u.b == (bool)v;
		}break;
	  case t_int:{
			return u.i8 == (int64)v;
		}break;
	  case t_uint:{
			return u.u8 == (uint64)v;
		}break;
	  case t_double:{
			return u.f8 == (double)v;
		}break;
	  case t_array:{
			if(v.t != t_array){
				return false;
			}else{
				const array* ba = reinterpret_cast<const array*>(&u.ba);
				const array* vba = reinterpret_cast<const array*>(&v.u.ba);
				return *ba == *vba;
			}
		}break;
	  case t_map:{
			if(v.t != t_map){
				return false;
			}else{
				const map* bm = reinterpret_cast<const map*>(&u.bm);
				const map* vbm = reinterpret_cast<const map*>(&v.u.bm);
				if(bm->size() != vbm->size()) return false;
				for(map::const_iterator it1 = vbm->begin(),it2 = bm->begin();it1 != vbm->end() && it2 != bm->end();++it1,++it2){
					if(it1->first != it2->first || it1->second != it2->second) return false;
				}
				return true;
			}
		}break;
	  case t_user:{
			return false;
		}break;
	  case t_invalid:{
			return t_invalid == v.t;
		}break;
	}
	throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator ==()",type_name(t).c_str()));
}

bool var::operator !=(const var& v) const throw(type_invalid)
{
	return !(*this == v);
}

bool var::operator <(const var& v) const throw(type_invalid)
{
	switch(t){
	  case t_bin:{
			const chunk* bp = reinterpret_cast<const chunk*>(&u.bp);
			return bp->compare((chunk)v) < 0;
		}break;
	  case t_str:{
			const string* bs = reinterpret_cast<const string*>(&u.bs);
			return bs->compare((string)v) < 0;
		}break;
	  case t_wstr:{
			const wstring* bw = reinterpret_cast<const wstring*>(&u.bw);
			return bw->compare((wstring)v) < 0;
		}break;
	  case t_array:{
			if(v.t != t_array){
				return t < v.t;
			}else{
				const array* ba = reinterpret_cast<const array*>(&u.ba);
				const array* vba = reinterpret_cast<const array*>(&v.u.ba);
				return *ba < *vba;
			}
		}break;
	  case t_map:{
			if(v.t != t_map){
				return t < v.t;
			}else{
				const map* bm = reinterpret_cast<const map*>(&u.bm);
				const map* vbm = reinterpret_cast<const map*>(&v.u.bm);
				if(bm->size() != vbm->size()) return false;
				for(map::const_iterator it1 = vbm->begin(),it2 = bm->begin();it1 != vbm->end() && it2 != bm->end();++it1,++it2){
					if(it2->first < it1->first) return false;
				}
				return bm->size() < vbm->size();
			}
		}break;
	  case t_bool:{
			return u.b < (bool)v;
		}break;
	  case t_int:{
			return u.i8 < (int64)v;
		}break;
	  case t_uint:{
			return u.u8 < (uint64)v;
		}break;
	  case t_double:{
			return u.f8 < (double)v;
		}break;
	  case t_invalid:{
			return false;
		}break;
	  default:
		throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator <()",type_name(t).c_str()));
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

var& var::at(lyramilk::data::uint64 index) throw(type_invalid)
{
	if(t == t_array){
		array* ba = reinterpret_cast<array*>(&u.ba);
		return ba->at(index);
	}
	throw type_invalid(lyramilk::kdict("%s：错误的索引类型%s[%s]","lyramilk::data::var::at()",type_name(t).c_str(),"t_uint"));
}

var& var::at(const string& index) throw(type_invalid)
{
	if(t == t_map){
		map* bm = reinterpret_cast<map*>(&u.bm);
		return bm->operator[](index);
	}
	throw type_invalid(lyramilk::kdict("%s：错误的索引类型%s[%s]","lyramilk::data::var::at()",type_name(t).c_str(),"t_str"));
}

var& var::at(const wstring& index) throw(type_invalid)
{
	if(t == t_map){
		var str = index;
		map* bm = reinterpret_cast<map*>(&u.bm);
		return bm->operator[](str);
	}
	throw type_invalid(lyramilk::kdict("%s：错误的索引类型%s[%s]","lyramilk::data::var::at()",type_name(t).c_str(),"t_wstr"));
}

const var& var::at(lyramilk::data::uint64 index) const throw(type_invalid)
{
	if(t == t_array){
		const array* ba = reinterpret_cast<const array*>(&u.ba);
		return ba->at(index);
	}
	throw type_invalid(lyramilk::kdict("%s：错误的索引类型%s[%s]","lyramilk::data::var::at()",type_name(t).c_str(),"t_uint"));
}

const var& var::at(const string& index) const throw(type_invalid)
{
	if(t == t_map){
		const map* bm = reinterpret_cast<const map*>(&u.bm);
		map::const_iterator it = bm->find(index);
		if (it != bm->end()) {
			return it->second;
		}
		return lyramilk::data::var::nil;
	}
	throw type_invalid(lyramilk::kdict("%s：错误的索引类型%s[%s]","lyramilk::data::var::at()",type_name(t).c_str(),"t_str"));
}

const var& var::at(const wstring& index) const throw(type_invalid)
{
	if(t == t_map){
		var str = index;
		const map* bm = reinterpret_cast<const map*>(&u.bm);
		map::const_iterator it = bm->find(str.str());
		if (it != bm->end()) {
			return it->second;
		}
		return lyramilk::data::var::nil;
	}
	throw type_invalid(lyramilk::kdict("%s：错误的索引类型%s[%s]","lyramilk::data::var::at()",type_name(t).c_str(),"t_wstr"));
}

var& var::assign(const var& v)
{
	if(this == &v) return *this;
	vt newt = v.t;
	vu oldu = u;
	switch(v.t){
	  case t_bin:
		{
			const chunk* vbp = reinterpret_cast<const chunk*>(&v.u.bp);
			new (u.bp) chunk(*vbp);
			vu newu = u;
			u = oldu;
			clear();
			u = newu;
		}
		break;
	  case t_str:
		{
			const string* vbs = reinterpret_cast<const string*>(&v.u.bs);
			new (u.bs) string(*vbs);
			vu newu = u;
			u = oldu;
			clear();
			u = newu;
		}
		break;
	  case t_wstr:
		{
			const wstring* vbw = reinterpret_cast<const wstring*>(&v.u.bw);
			new (u.bw) wstring(*vbw);
			vu newu = u;
			u = oldu;
			clear();
			u = newu;
		}
		break;
	  case t_bool:
		{
			bool tmp = v.u.b;
			clear();
			u.b = tmp;
		}
		break;
	  case t_int:
		{
			int64 tmp = v.u.i8;
			clear();
			u.i8 = tmp;
		}
		break;
	  case t_uint:
		{
			uint64 tmp = v.u.u8;
			clear();
			u.u8 = tmp;
		}
		break;
	  case t_double:
		{
			double tmp = v.u.f8;
			clear();
			u.f8 = tmp;
		}
		break;
	  case t_array:
		{
			const array* vba = reinterpret_cast<const array*>(&v.u.ba);
			new (u.ba) array(*vba);
			vu newu = u;
			u = oldu;
			clear();
			u = newu;
		}
		break;
	  case t_map:
		{
			const map* vbm = reinterpret_cast<const map*>(&v.u.bm);
			new (u.bm) map(*vbm);
			vu newu = u;
			u = oldu;
			clear();
			u = newu;
			t = newt;
		}
		break;
	  case t_user:
		{
			const _userdata* vbo = reinterpret_cast<const _userdata*>(&v.u.bo);
			new (u.bo) _userdata(*vbo);
			vu newu = u;
			u = oldu;
			clear();
			u = newu;
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
	new (u.bp) chunk(v?v:(const unsigned char*)"");
	return *this;
}

var& var::assign(const char* v)
{
	clear();
	t = t_str;
	new (u.bs) string(v?v:"");
	return *this;
}

var& var::assign(const wchar_t* v)
{
	clear();
	t = t_wstr;
	new (u.bw) wstring(v?v:L"");
	return *this;
}

var& var::assign(const chunk& v)
{
	clear();
	t = t_bin;
	new (u.bp) chunk(v);
	return *this;
}

var& var::assign(const string& v)
{
	clear();
	t = t_str;
	new (u.bs) string(v);
	return *this;
}

var& var::assign(const wstring& v)
{
	clear();
	t = t_wstr;
	new (u.bw) wstring(v);
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
	t = t_int;
	u.i8 = v;
	return *this;
}

var& var::assign(uint8 v)
{
	clear();
	t = t_uint;
	u.u8 = v;
	return *this;
}

var& var::assign(int16 v)
{
	clear();
	t = t_int;
	u.i8 = v;
	return *this;
}

var& var::assign(uint16 v)
{
	clear();
	t = t_uint;
	u.u8 = v;
	return *this;
}

var& var::assign(int32 v)
{
	clear();
	t = t_int;
	u.i8 = v;
	return *this;
}

var& var::assign(uint32 v)
{
	clear();
	t = t_uint;
	u.u8 = v;
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
	t = t_int;
	u.i8 = v;
	return *this;
}

var& var::assign(uint64 v)
{
	clear();
	t = t_uint;
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
	new (u.ba) array(v);
	return *this;
}

var& var::assign(const lyramilk::data::var::map& v)
{
	clear();
	t = t_map;
	new (u.bm) map(v);
	return *this;
}

var& var::assign(const string& n,const void* v)
{
	clear();
	t = t_user;
	_userdata* bo = new (u.bo) _userdata;
	bo->operator[](n) = v;
	return *this;
}

/*var::operator chunk& () throw(type_invalid)
{
	if(t != t_bin) throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator chunk&()",type_name(t).c_str()));
	return *u.p;
}*/

var::operator chunk() const throw(type_invalid)
{
	switch(t){
	  case t_bin:{
			const chunk* bp = reinterpret_cast<const chunk*>(&u.bp);
			return *bp;
		}break;
	  case t_str:{
			const string* bs = reinterpret_cast<const string*>(&u.bs);
			return chunk((const unsigned char*)bs->c_str(),bs->size());
		}break;
	  case t_wstr:{
			const wstring* bw = reinterpret_cast<const wstring*>(&u.bw);
			return chunk((const unsigned char*)bw->c_str(),(bw->size() << (int)tempc<sizeof(wchar_t)>::square));
		}break;
	  case t_bool:{
			return chunk((const unsigned char*)&u.b,sizeof(u.b));
		}break;
	  case t_int:{
			int64 i8 = reverse_order(u.i8);
			return chunk((const unsigned char*)&i8,sizeof(i8));
		}break;
	  case t_uint:{
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
			throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator chunk()",type_name(t).c_str()));
		}break;
	}
	throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator chunk()",type_name(t).c_str()));
}

/*var::operator string& () throw(type_invalid)
{
	if(t != t_str) throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator string&()",type_name(t).c_str()));
	return *u.s;
}*/

var::operator string () const throw(type_invalid)
{
	switch(t){
	  case t_bin:{
			const chunk* bp = reinterpret_cast<const chunk*>(&u.bp);
			return string((const char*)bp->c_str(),bp->size());
		}break;
	  case t_str:{
			const string* bs = reinterpret_cast<const string*>(&u.bs);
			return *bs;
		}break;
	  case t_wstr:{
			const wstring* bw = reinterpret_cast<const wstring*>(&u.bw);
			return string(u2a(*bw));
		}break;
	  case t_bool:{
			return u.b?"true":"false";
		}break;
	  case t_int:{
			char buff[256];
			sprintf(buff,"%lld",u.i8);
			return buff;
		}break;
	  case t_uint:{
			char buff[256];
			sprintf(buff,"%llu",u.u8);
			return buff;
		}break;
	  case t_double:{
			char buff[256];
			sprintf(buff,"%f",u.f8);
			return buff;
		}break;
	  case t_array:{
			const array* ba = reinterpret_cast<const array*>(&u.ba);
			array::const_iterator it = ba->begin();
			string str = "[";
			if(it != ba->end()){
				str += (string)*it;
				for(++it;it!=ba->end();++it){
					str += ",";
					str += (string)*it;
				}
			}
			str += "]";
			return str;
		}break;
	  case t_map:{
			const map* bm = reinterpret_cast<const map*>(&u.bm);

			string str = "{";
			map::const_iterator it = bm->begin();
			if(it != bm->end()){
				str += (string)it->first + ":" + (string)it->second;
				for(++it;it!=bm->end();++it){
					str += "," + (string)it->first + ":" + (string)it->second;
				}
			}
			str += "}";
			return str;
		}break;
	  case t_user:{
			//throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator string()",type_name(t).c_str()));
			return "";
		}break;
	  case t_invalid:{
			//throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator string()",type_name(t).c_str()));
			return "";
		}break;
	}
	throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator string()",type_name(t).c_str()));
}

/*var::operator wstring& () throw(type_invalid)
{
	if(t != t_wstr) throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator wstring&()",type_name(t).c_str()));
	return *u.w;
}*/

var::operator wstring () const throw(type_invalid)
{
	switch(t){
	  case t_bin:{
			const chunk* bp = reinterpret_cast<const chunk*>(&u.bp);
			return wstring((const wchar_t*)bp->c_str(),(bp->size() << (int)tempc<sizeof(wchar_t)>::square));
		}break;
	  case t_str:{
			const string* bs = reinterpret_cast<const string*>(&u.bs);
			return a2u(*bs);
		}break;
	  case t_wstr:{
			const wstring* bw = reinterpret_cast<const wstring*>(&u.bw);
			return *bw;
		}break;
	  case t_bool:{
			return u.b?L"true":L"false";
		}break;
	  case t_int:{
			wchar_t buff[256];
			swprintf(buff,sizeof(buff),L"%lld",u.i8);
			return buff;
		}break;
	  case t_uint:{
			wchar_t buff[256];
			swprintf(buff,sizeof(buff),L"%llu",u.u8);
			return buff;
		}break;
	  case t_double:{
			wchar_t buff[256];
			swprintf(buff,sizeof(buff),L"%f",u.f8);
			return buff;
		}break;
	  case t_array:{
			const array* ba = reinterpret_cast<const array*>(&u.ba);
			array::const_iterator it = ba->begin();
			wstring str = L"[";
			if(it != ba->end()){
				str += (wstring)*it;
				for(++it;it!=ba->end();++it){
					str += L",";
					str += (wstring)*it;
				}
			}
			str += L"]";
			return str;
		}break;
	  case t_map:{
			const map* bm = reinterpret_cast<const map*>(&u.bm);
			wstring str = L"{";
			map::const_iterator it = bm->begin();
			if(it != bm->end()){
				wstring strfirst = var(it->first);
				str += strfirst + L":" + (wstring)it->second;
				for(++it;it!=bm->end();++it){
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
	throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator wstring()",type_name(t).c_str()));
 }

var::operator bool () const throw(type_invalid)
{
	switch(t){
	  case t_bin:{
			const chunk* bp = reinterpret_cast<const chunk*>(&u.bp);
			if(sizeof(bool) > bp->size()) throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator bool()",type_name(t).c_str()));
			bool b;
			memcpy(&b,bp->c_str(),sizeof(b));
			return b;	}break;
	  case t_str:{
			const string* bs = reinterpret_cast<const string*>(&u.bs);
			string w = *bs;
			std::transform(bs->begin(),bs->end(),w.begin(),tolower);
			if(w.compare("true") == 0) return true;
			if(w.compare("false") == 0) return false;
			throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator bool()",type_name(t).c_str()));
		}break;
	  case t_wstr:{
			const wstring* bw = reinterpret_cast<const wstring*>(&u.bw);
			wstring w = *bw;
			std::transform(bw->begin(),bw->end(),w.begin(),towlower);
			if(w.compare(L"true") == 0) return true;
			if(w.compare(L"false") == 0) return false;
			throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator bool()",type_name(t).c_str()));
		}break;
	  case t_bool:
	  case t_int:
	  case t_uint:
	  case t_double:{
			return (int64)*this != 0;
		}break;
	  case t_array:{
			throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator bool()",type_name(t).c_str()));
		}break;
	  case t_map:{
			throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator bool()",type_name(t).c_str()));
		}break;
	  case t_user:{
			throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator bool()",type_name(t).c_str()));
		}break;
	  case t_invalid:{
			throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator bool()",type_name(t).c_str()));
		}break;
	}
	throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator bool()",type_name(t).c_str()));
}

var::operator int8 () const throw(type_invalid)
{
	switch(t){
	  case t_bin:
	  case t_str:
	  case t_wstr:
	  case t_bool:
	  case t_int:
	  case t_uint:
	  case t_double:{
			return (int8)(int64)*this;
		}break;
	  case t_array:
	  case t_map:
	  case t_user:
	  case t_invalid:{
			throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator int8()",type_name(t).c_str()));
		}break;
	}
	throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator int8()",type_name(t).c_str()));
}

var::operator uint8 () const throw(type_invalid)
{
	switch(t){
	  case t_bin:
	  case t_str:
	  case t_wstr:
	  case t_bool:
	  case t_int:
	  case t_uint:
	  case t_double:{
			return (uint8)(int64)*this;
		}break;
	  case t_array:
	  case t_map:
	  case t_user:
	  case t_invalid:{
			throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator uint8()",type_name(t).c_str()));
		}break;
	}
	throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator uint8()",type_name(t).c_str()));
}

var::operator int16 () const throw(type_invalid)
{
	switch(t){
	  case t_bin:
	  case t_str:
	  case t_wstr:
	  case t_bool:
	  case t_int:
	  case t_uint:
	  case t_double:{
			return (int16)(int64)*this;
		}break;
	  case t_array:
	  case t_map:
	  case t_user:
	  case t_invalid:{
			throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator int16()",type_name(t).c_str()));
		}
	}
	throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator int16()",type_name(t).c_str()));
}

var::operator uint16 () const throw(type_invalid)
{
	switch(t){
	  case t_bin:
	  case t_str:
	  case t_wstr:
	  case t_bool:
	  case t_int:
	  case t_uint:
	  case t_double:{
			return (uint16)(int64)*this;
		}break;
	  case t_array:
	  case t_map:
	  case t_user:
	  case t_invalid:{
			throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator uint16()",type_name(t).c_str()));
		}
	}
	throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator uint16()",type_name(t).c_str()));
}

var::operator int32 () const throw(type_invalid)
{
	switch(t){
	  case t_bin:
	  case t_str:
	  case t_wstr:
	  case t_bool:
	  case t_int:
	  case t_uint:
	  case t_double:{
			return (int32)(int64)*this;
		}break;
	  case t_array:
	  case t_map:
	  case t_user:
	  case t_invalid:{
			throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator int32()",type_name(t).c_str()));
		}
	}
	throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator int32()",type_name(t).c_str()));
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
	  case t_int:{
			return u.i8;
		}break;
	  case t_uint:{
			return u.u8;
		}break;
	  case t_bin:{
			const chunk* bp = reinterpret_cast<const chunk*>(&u.bp);
			if(sizeof(int64) > bp->size()) throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator int64()",type_name(t).c_str()));
			int64 i8;
			memcpy(&i8,bp->c_str(),sizeof(i8));
			return i8;
		}break;
	  case t_str:{
			const string* bs = reinterpret_cast<const string*>(&u.bs);
			char* p;
			return strtoll(bs->c_str(),&p,10);
		}break;
	  case t_wstr:{
			const wstring* bw = reinterpret_cast<const wstring*>(&u.bw);
			wchar_t* p;
			return wcstoll(bw->c_str(),&p,10);
		}break;
	  case t_bool:{
			return u.b;
		}break;
	  case t_double:{
			return (int64)u.f8;
		}break;
	  case t_array:
	  case t_map:
	  case t_user:
	  case t_invalid:{
			throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator int64()",type_name(t).c_str()));
		}break;
	}
	throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator int64()",type_name(t).c_str()));
}

var::operator uint64 () const throw(type_invalid)
{
	return (uint64)(int64)*this;
}


var::operator double () const throw(type_invalid)
{
	switch(t){
	  case t_bin:{
			const chunk* bp = reinterpret_cast<const chunk*>(&u.bp);
			if(sizeof(int64) > bp->size()) throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator uint64()",type_name(t).c_str()));
			double f8;
			memcpy(&f8,bp->c_str(),sizeof(f8));
			return f8;
		}break;
	  case t_str:{
			const string* bs = reinterpret_cast<const string*>(&u.bs);
			char* p;
			return strtod(bs->c_str(),&p);
		}break;
	  case t_wstr:{
			const wstring* bw = reinterpret_cast<const wstring*>(&u.bw);
			wchar_t* p;
			return wcstod(bw->c_str(),&p);
		}break;
	  case t_bool:{
			return u.b;
		}break;
	  case t_int:{
			return (double)u.i8;
		}break;
	  case t_uint:{
			return (double)u.u8;
		}break;
	  case t_double:{
			return u.f8;
		}break;
	  case t_array:
	  case t_map:
	  case t_user:
	  case t_invalid:{
			throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator double()",type_name(t).c_str()));
		}break;
	}
	throw type_invalid(lyramilk::kdict("%s：错误的子类型%s","lyramilk::data::var::operator double()",type_name(t).c_str()));
}

var::operator float () const throw(type_invalid)
{
	return (float)(double)*this;
}

var::operator lyramilk::data::var::array& () throw(type_invalid)
{
	array* ba = reinterpret_cast<array*>(&u.ba);
	if(t == t_array) return *ba;
	throw type_invalid(lyramilk::kdict("%s：%s类型无法转换为%s类型","lyramilk::data::var::operator var::array()",type_name(t).c_str(),type_name(t_array).c_str()));
}

var::operator const lyramilk::data::var::array& () const throw(type_invalid)
{
	const array* ba = reinterpret_cast<const array*>(&u.ba);
	if(t == t_array) return *ba;
	throw type_invalid(lyramilk::kdict("%s：%s类型无法转换为%s类型","lyramilk::data::var::operator var::array()",type_name(t).c_str(),type_name(t_array).c_str()));
}

var::operator var::map& ()  throw(type_invalid)
{
	map* bm = reinterpret_cast<map*>(&u.bm);
	if(t == t_map) return *bm;
	throw type_invalid(lyramilk::kdict("%s：%s类型无法转换为%s类型","lyramilk::data::var::operator var::map()",type_name(t).c_str(),type_name(t_map).c_str()));
}

var::operator const lyramilk::data::var::map& () const throw(type_invalid)
{
	const map* bm = reinterpret_cast<const map*>(&u.bm);
	if(t == t_map) return *bm;
	throw type_invalid(lyramilk::kdict("%s：%s类型无法转换为%s类型","lyramilk::data::var::operator var::map()",type_name(t).c_str(),type_name(t_map).c_str()));
}

chunk var::conv(chunk if_not_compat) const
{
	if(type_like(t_str)) return *this;
	return if_not_compat;
}

string var::conv(string if_not_compat) const
{
	if(type_like(t_str)) return *this;
	return if_not_compat;
}

wstring var::conv(wstring if_not_compat) const
{
	if(type_like(t_str)) return *this;
	return if_not_compat;
}

string var::conv(const char* if_not_compat) const
{
	if(type_like(t_str)) return *this;
	return if_not_compat;
}

string var::conv(char* if_not_compat) const
{
	if(type_like(t_str)) return *this;
	return if_not_compat;
}

wstring var::conv(const wchar_t* if_not_compat) const
{
	if(type_like(t_str)) return *this;
	return if_not_compat;
}

wstring var::conv(wchar_t* if_not_compat) const
{
	if(type_like(t_str)) return *this;
	return if_not_compat;
}

chunk var::conv(const unsigned char* if_not_compat) const
{
	if(type_like(t_bin)) return *this;
	return if_not_compat;
}

chunk var::conv(unsigned char* if_not_compat) const
{
	if(type_like(t_bin)) return *this;
	return if_not_compat;
}

bool var::conv(bool if_not_compat) const
{
	if(type_like(t_str)) return *this;
	return if_not_compat;
}

uint64 var::conv(int8 if_not_compat) const
{
	if(type_like(t_str)) return *this;
	return if_not_compat;
}
uint64 var::conv(uint8 if_not_compat) const
{
	if(type_like(t_str)) return *this;
	return if_not_compat;
}
uint64 var::conv(int16 if_not_compat) const
{
	if(type_like(t_str)) return *this;
	return if_not_compat;
}
uint64 var::conv(uint16 if_not_compat) const
{
	if(type_like(t_str)) return *this;
	return if_not_compat;
}
uint64 var::conv(int32 if_not_compat) const
{
	if(type_like(t_str)) return *this;
	return if_not_compat;
}
uint64 var::conv(uint32 if_not_compat) const
{
	if(type_like(t_str)) return *this;
	return if_not_compat;
}

uint64 var::conv(int64 if_not_compat) const
{
	if(type_like(t_str)) return *this;
	return if_not_compat;
}

uint64 var::conv(uint64 if_not_compat) const
{
	if(type_like(t_str)) return *this;
	return if_not_compat;
}

uint64 var::conv(long if_not_compat) const
{
	if(type_like(t_str)) return *this;
	return if_not_compat;
}

uint64 var::conv(unsigned long if_not_compat) const
{
	if(type_like(t_str)) return *this;
	return if_not_compat;
}

double var::conv(double if_not_compat) const
{
	if(type_like(t_str)) return *this;
	return if_not_compat;
}

var::array& var::conv(var::array& if_not_compat)
{
	if(!type_like(t_array)) return if_not_compat;
	return *this;
}

var::map& var::conv(var::map& if_not_compat)
{
	if(!type_like(t_map)) return if_not_compat;
	return *this;
}


void lyramilk::data::var::userdata(string v,const void* p) throw(type_invalid)
{
	if(t != t_user)	throw type_invalid(lyramilk::kdict("%s：%s类型无法赋予用户数据","lyramilk::data::var::operator var::map()",type_name(t).c_str()));
	_userdata* bo = reinterpret_cast<_userdata*>(&u.bo);
	bo->operator[](v) = p;
}

const void* lyramilk::data::var::userdata(string v) const
{
	if(t != t_user)	return null;
	const _userdata* bo = reinterpret_cast<const _userdata*>(&u.bo);
	_userdata::const_iterator it = bo->find(v);
	if(it == bo->end()) return null;
	return it->second;
}

const void* lyramilk::data::var::userdata() const
{
	if(t != t_user)	return null;
	const _userdata* bo = reinterpret_cast<const _userdata*>(&u.bo);
	_userdata::const_iterator it = bo->begin();
	if(it == bo->end()) return null;
	return it->second;
}

var& var::operator[](const char* index) throw(type_invalid)
{
	return at(index);
}

var& var::operator[](const wchar_t* index) throw(type_invalid)
{
	return at(index);
}

var& var::operator[](const string& index) throw(type_invalid)
{
	return at(index);
}

var& var::operator[](const wstring& index) throw(type_invalid)
{
	return at(index);
}

var& var::operator[](bool index) throw(type_invalid)
{
	return at(index);
}

var& var::operator[](int8 index) throw(type_invalid)
{
	return at(index);
}

var& var::operator[](uint8 index) throw(type_invalid)
{
	return at(index);
}

var& var::operator[](int16 index) throw(type_invalid)
{
	return at(index);
}

var& var::operator[](uint16 index) throw(type_invalid)
{
	return at(index);
}

var& var::operator[](int32 index) throw(type_invalid)
{
	return at(index);
}

var& var::operator[](uint32 index) throw(type_invalid)
{
	return at(index);
}

var& var::operator[](long index) throw(type_invalid)
{
	return at(index);
}

var& var::operator[](int64 index) throw(type_invalid)
{
	return at((lyramilk::data::uint32)index);
}

var& var::operator[](uint64 index) throw(type_invalid)
{
	return at((lyramilk::data::uint32)index);
}

var& var::operator[](double index) throw(type_invalid)
{
	return at((lyramilk::data::uint32)index);
}

const var& var::operator[](const char* index) const throw(type_invalid)
{
	return at(index);
}

const var& var::operator[](const wchar_t* index) const throw(type_invalid)
{
	return at(index);
}

const var& var::operator[](const string& index) const throw(type_invalid)
{
	return at(index);
}

const var& var::operator[](const wstring& index) const throw(type_invalid)
{
	return at(index);
}

const var& var::operator[](bool index) const throw(type_invalid)
{
	return at(index);
}

const var& var::operator[](int8 index) const throw(type_invalid)
{
	return at(index);
}

const var& var::operator[](uint8 index) const throw(type_invalid)
{
	return at(index);
}

const var& var::operator[](int16 index) const throw(type_invalid)
{
	return at(index);
}

const var& var::operator[](uint16 index) const throw(type_invalid)
{
	return at(index);
}

const var& var::operator[](int32 index) const throw(type_invalid)
{
	return at(index);
}

const var& var::operator[](uint32 index) const throw(type_invalid)
{
	return at(index);
}

const var& var::operator[](long index) const throw(type_invalid)
{
	return at(index);
}

const var& var::operator[](int64 index) const throw(type_invalid)
{
	return at((lyramilk::data::uint32)index);
}

const var& var::operator[](uint64 index) const throw(type_invalid)
{
	return at((lyramilk::data::uint32)index);
}

const var& var::operator[](double index) const throw(type_invalid)
{
	return at((lyramilk::data::uint32)index);
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
	  case t_int:
		return "t_int";
	  case t_uint:
		return "t_uint";
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
	switch(nt){
	  case t_bin:{
			chunk t = *this;
			clear();
			new (u.bp) chunk(t);
		}break;
	  case t_str:{
			string t = *this;
			clear();
			new (u.bs) string(t);
		}break;
	  case t_wstr:{
			wstring t = *this;
			clear();
			new (u.bw) wstring(t);
		}break;
	  case t_bool:{
			bool t = *this;
			clear();
			u.b = t;
		}break;
	  case t_int:{
			lyramilk::data::int64 t = *this;
			clear();
			u.i8 = t;
		}break;
	  case t_uint:{
			lyramilk::data::uint64 t = *this;
			clear();
			u.u8 = t;
		}break;
	  case t_double:{
			double t = *this;
			clear();
			u.f8 = t;
		}break;
	  case t_array:{
			if(type_like(t_array)){
				array t = *this;
				clear();
				new (u.ba) array(t);
			}else{
				clear();
				new (u.ba) array();
			}
		}break;
	  case t_map:{
			if(type_like(t_map)){
				map t = *this;
				clear();
				new (u.bw) map(t);
			}else{
				clear();
				new (u.bw) map();
			}
		}break;
	  case t_user:{
			if(t != nt){
				clear();
			}
		}break;
	  case t_invalid:
		clear();
		break;
	  default:
		throw type_invalid(lyramilk::kdict("%s：错误的新类型%d","lyramilk::data::var::operator var::type()",nt));
	}
	t = nt;
	return *this;
}

bool var::type_like(vt nt) const
{
	if((nt == t_bin || nt == t_str || nt == t_wstr) && (t == t_bin || t == t_str || t == t_wstr ||   t == t_int ||  t == t_uint ||  t == t_double)){
		return true;
	}

	if((nt == t_int || nt == t_uint || nt == t_double) &&
		(t == t_int ||  t == t_uint ||  t == t_double)){
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
		const array* ba = reinterpret_cast<const array*>(&u.ba);
		return ba->size();
	}
	if(t == t_map){
		const map* bm = reinterpret_cast<const map*>(&u.bm);
		return bm->size();
	}
	return 0;
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
	  case t_int:
	  case t_uint:
	  case t_double:
	  case t_invalid:
		break;
	  case t_bin:
		{
			const chunk* bp = reinterpret_cast<const chunk*>(&u.bp);
			bp->~chunk();
		}
		break;
	  case t_str:
		{
			const string* bs = reinterpret_cast<const string*>(&u.bs);
			bs->~string();
		}
		break;
	  case t_wstr:
		{
			const wstring* bw = reinterpret_cast<const wstring*>(&u.bw);
			bw->~wstring();
		}
		break;
	  case t_array:
		{
			const array* ba = reinterpret_cast<const array*>(&u.ba);
			ba->~array();
		}
		break;
	  case t_map:
		{
			const map* bm = reinterpret_cast<const map*>(&u.bm);
			bm->~map();
		}
		break;
	  case t_user:
		{
			const _userdata* bo = reinterpret_cast<const _userdata*>(&u.bo);
			bo->~_userdata();
		}
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
	if((t&0x7f) != t) throw type_invalid(lyramilk::kdict("%s：不支持的类型%d","lyramilk::data::var::serialize()",(t&0x7f)));
	unsigned char m;
	m = g_igendian<<7;
	m |= t&0x7f;

	switch(t){
	  case t_bin:{
			const chunk* bp = reinterpret_cast<const chunk*>(&u.bp);
			string_size_type size = (string_size_type)bp->size();
			write(os,m);
			write(os,size);
			os.write((const char*)bp->c_str(),size);
			return true;
		}break;
	  case t_str:{
			const string* bs = reinterpret_cast<const string*>(&u.bs);
			string utf8str = a2t(*bs);
			string_size_type size = (string_size_type)utf8str.size();
			write(os,m);
			write(os,size);
			os.write(utf8str.c_str(),size);
			return true;
		}break;
	  case t_wstr:{
			const wstring* bw = reinterpret_cast<const wstring*>(&u.bw);
			string utf8str = u2t(*bw);
			string_size_type size = (string_size_type)utf8str.size();
			write(os,m);
			write(os,size);
			os.write(utf8str.c_str(),size);
			return true;
		}break;
	  case t_bool:
	  case t_int:
	  case t_uint:{
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
			const array* ba = reinterpret_cast<const array*>(&u.ba);
			array_size_type size = (array_size_type)ba->size();
			write(os,m);
			write(os,size);
			array::const_iterator it = ba->begin();
			for(;it != ba->end();++it){
				if(!it->_serialize(os)) return false;
			}
			return true;
		}break;
	  case t_map:{
			const map* bm = reinterpret_cast<const map*>(&u.bm);
			array_size_type size = (array_size_type)bm->size();
			write(os,m);
			write(os,size);
			map::const_iterator it = bm->begin();
			for(;it != bm->end();++it){
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
	throw type_invalid(lyramilk::kdict("%s：不支持的类型%d","lyramilk::data::var::serialize()",(t&0x7f)));
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
	  case t_int:
	  case t_uint:{
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
			array* ba = reinterpret_cast<array*>(&u.ba);
			for(array_size_type i=0;i<size;++i){
				var d;
				if(!d._deserialize(is)){
					clear();
					return false;
				}
				ba->push_back(d);
			}
			t = ts;
			return true;
		}break;
	  case t_map:{
			array_size_type size = 0;
			if(!read(is,size)) return false;
			if(r) size = reverse_order(size);
			type(t_map);
			map* bm = reinterpret_cast<map*>(&u.bm);
			for(array_size_type i=0;i<size;++i){
				var key;
				var value;
				if(!(key._deserialize(is) && value._deserialize(is))){
					clear();
					return false;
				}
				bm->operator[](key) = value;
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
	if((p->type() & (t_map | t_array)) == 0) throw type_invalid(lyramilk::kdict("%s 路径：根元素不是t_map或t_array(%s)","lyramilk::data::var::path()",varpath.c_str()));
*/
	for(;it!=fields.end();++it){
		if(p->type() == t_array){
			string& str = *it;
			if(str.find_first_not_of("0123456789") != str.npos){
				throw type_invalid(lyramilk::kdict("%s 路径：t_array类型只能接收纯整数的字符串形式(%s)","lyramilk::data::var::path()",varpath.c_str()));
			}
			unsigned int index = atoi(str.c_str());
			array* ba = reinterpret_cast<array*>(&p->u.ba);
			if(ba->size() == index){
				ba->push_back(nil);
			}else if(ba->size() < index + 1){
				throw type_invalid(lyramilk::kdict("%s 路径：t_array类型越界(%s)","lyramilk::data::var::path()",varpath.c_str()));
			}
			p = &ba->at(index);
		}else if(p->type() == t_map){
			map* bm = reinterpret_cast<map*>(&p->u.bm);
			p = &bm->operator[](*it);
		}else if(p->type() == t_invalid){
			string& str = *it;
			if(str.find_first_not_of("0123456789") == str.npos){
				unsigned int index = atoi(str.c_str());
				p->type(t_array);

				array* ba = reinterpret_cast<array*>(&p->u.ba);
				if(ba->size() == index){
					ba->push_back(nil);
					p = &ba->back();
				}else if(ba->size() < index - 1){
					p = &ba->operator[](index);
				}
			}else{
				p->type(t_map);
				map* bm = reinterpret_cast<map*>(&p->u.bm);
				p = &bm->operator[](str);
			}
		}else{
			throw type_invalid(lyramilk::kdict("%s 路径：%s","lyramilk::data::var::path()",varpath.c_str()));
		}
	}
	return *p;
}

const var& var::path(string varpath) const throw(type_invalid)
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
	const var* p = this;
	/*
	if(p->type() == t_invalid){
		p->type(t_map);
	}
	if((p->type() & (t_map | t_array)) == 0) throw type_invalid(lyramilk::kdict("%s 路径：根元素不是t_map或t_array(%s)","lyramilk::data::var::path()",varpath.c_str()));
*/
	for(;it!=fields.end();++it){
		if(p->type() == t_array){
			string& str = *it;
			if(str.find_first_not_of("0123456789") != str.npos){
				throw type_invalid(lyramilk::kdict("%s 路径：t_array类型只能接收纯整数的字符串形式(%s)","lyramilk::data::var::path()",varpath.c_str()));
			}
			unsigned int index = atoi(str.c_str());
			const array* ba = reinterpret_cast<const array*>(&u.ba);
			if(ba->size() <= index){
				return lyramilk::data::var::nil;
			}
			p = &ba->at(index);
		}else if(p->type() == t_map){
			const map* bm = reinterpret_cast<const map*>(&u.bm);
			map::const_iterator it_map = bm->find(*it);
			if(it_map == bm->end()){
				return lyramilk::data::var::nil;
			}
			p = &it_map->second;
		}else{
			return lyramilk::data::var::nil;
		}
	}
	return *p;
}

template < >
lyramilk::data::chunk& lyramilk::data::var::as<lyramilk::data::chunk&>() throw(type_invalid)
{
	if(t != t_bin) throw type_invalid(lyramilk::kdict("as取引用时无法转换类型"));
	chunk* bp = reinterpret_cast<chunk*>(&u.bp);
	return *bp;
}

template < >
lyramilk::data::string& lyramilk::data::var::as<lyramilk::data::string&>() throw(type_invalid)
{
	if(t != t_str) throw type_invalid(lyramilk::kdict("as取引用时无法转换类型"));
	string* bs = reinterpret_cast<string*>(&u.bs);
	return *bs;
}

template < >
lyramilk::data::wstring& lyramilk::data::var::as<lyramilk::data::wstring&>() throw(type_invalid)
{
	if(t != t_wstr) throw type_invalid(lyramilk::kdict("as取引用时无法转换类型"));
	wstring* bw = reinterpret_cast<wstring*>(&u.bw);
	return *bw;
}

}}	//namespace

std::ostream& operator << (std::ostream& os,const lyramilk::data::var& t)
{
	return os << t.str();
}

std::istream& operator >> (std::istream& is, lyramilk::data::var& t)
{
	char buff[4096];
	lyramilk::data::string str;
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



#ifdef Z_HAVE_UNORDEREDMAP
namespace std{
#elif defined Z_HAVE_TR1_UNORDEREDMAP
namespace std{namespace tr1{
#endif

#if (defined Z_HAVE_UNORDEREDMAP) || (defined Z_HAVE_TR1_UNORDEREDMAP)
	template <>
	std::size_t hash<lyramilk::data::string>::operator()(lyramilk::data::string d) const
	{
		const static std::size_t seed = 0xee6b27eb;
		const static std::size_t m = 0xc6a4a7935bd1e995ULL;
		const static int r = 47;

		const std::size_t* p = (const std::size_t*)d.c_str();
		std::size_t l = d.size();

		const std::size_t* end = p + (l/8);
		std::size_t h = seed ^ (l * m);

		while(p != end)
		{
			std::size_t k = *p++;

			k *= m; 
			k ^= k >> r; 
			k *= m; 

			h ^= k;
			h *= m; 
		}

		const unsigned char * p2 = (const unsigned char*)p;

		switch(l & 7)
		{
		  case 7: h ^= std::size_t(p2[6]) << 48;
		  case 6: h ^= std::size_t(p2[5]) << 40;
		  case 5: h ^= std::size_t(p2[4]) << 32;
		  case 4: h ^= std::size_t(p2[3]) << 24;
		  case 3: h ^= std::size_t(p2[2]) << 16;
		  case 2: h ^= std::size_t(p2[1]) << 8;
		  case 1: h ^= std::size_t(p2[0]);
			h *= m;
		};

		h ^= h >> r;
		h *= m;
		h ^= h >> r;

		return h;
	}
#endif
#ifdef Z_HAVE_UNORDEREDMAP
}
#elif defined Z_HAVE_TR1_UNORDEREDMAP
}}
#endif
