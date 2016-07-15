#include "codes.h"
#include "log.h"
#include "multilanguage.h"

#include <cassert>
#include <algorithm>
#include <cctype>
#include <memory.h>


#ifdef LZ4_FOUND
	#include <lz4.h>
#endif

#ifdef __linux__
	#include <iconv.h>
#endif

namespace lyramilk{ namespace data
{
	coding_exception::coding_exception(string msg):lyramilk::exception(msg)
	{}

#ifdef __linux__

	string iconv(const string& str,const string& from,const string& to)
	{
		const size_t ds = 65536;

		iconv_t cd = iconv_open(from.c_str(),to.c_str());
		if(cd == 0){
			assert(cd);
			return "";
		}

		size_t p1s = str.size();
		char* p1 = (char*)str.c_str();

		vector<char> ret;
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
#endif

	coding::~coding()
	{}

	codes* codes::instance()
	{
		static codes _mm;
		return &_mm;
	}

	bool codes::define(const string& codingname,getter ctr)
	{
		string str(codingname.size(),0);
		transform(codingname.begin(), codingname.end(), str.begin(), tolower);
		std::pair<builder_type::iterator,bool> it = builder.insert(std::make_pair(str,ctr));
		return it.second;
	}

	bool codes::undefine(const string& codingname)
	{
		string str(codingname.size(),0);
		transform(codingname.begin(), codingname.end(), str.begin(), tolower);
		builder_type::iterator it = builder.find(str);
		if(it == builder.end()) return false;
		builder.erase(it);
		return true;
	}

	string codes::encode(const string& src,const string& codingname) throw (lyramilk::exception)
	{
		string str(codingname.size(),0);
		transform(codingname.begin(), codingname.end(), str.begin(), tolower);
		builder_type::iterator it = builder.find(str);
		if(it == builder.end()){
			throw lyramilk::exception(D("不可识别的编码：%s",codingname.c_str()));
		}
		return it->second()->encode(src);
	}
	string codes::decode(const string& src,const string& codingname) throw (lyramilk::exception)
	{
		string str(codingname.size(),0);
		transform(codingname.begin(), codingname.end(), str.begin(), tolower);
		builder_type::iterator it = builder.find(str);
		if(it == builder.end()){
			throw lyramilk::exception(D("不可识别的编码：%s",codingname.c_str()));
		}
		return it->second()->decode(src);
	}

	lyramilk::data::var::array codes::supports()
	{
		lyramilk::data::var::array r;
		builder_type::iterator it = builder.begin();
		for(;it!=builder.end();++it){
			r.push_back(it->first);
		}
		return r;
	}

#ifdef LZ4_FOUND
	/*************** coding_lz4 ******************/
	class coding_lz4:public coding
	{
	  public:
		virtual string decode(const string& str)
		{
			std::vector<char> buf(str.size());
			while(true){
				int i = LZ4_decompress_safe(str.c_str(),buf.data(),str.size(),buf.size());
				if(i > 0){
					buf.erase(buf.begin()+i,buf.end());
					break;
				}
				buf.resize(buf.size()*2);
			}
			return string(buf.data(),buf.size());
		}
		virtual string encode(const string& str)
		{
			std::vector<char> buf(str.size());
			while(true){
				int i = LZ4_compress_fast(str.c_str(),buf.data(),str.size(),buf.size(),3);
				if(i > 0){
					buf.erase(buf.begin()+i,buf.end());
					break;
				}
				buf.resize(buf.size()*2);
			}
			return string(buf.data(),buf.size());
		}

		static coding* getter()
		{
			static coding_lz4 _mm;
			return &_mm;
		}

		static __attribute__ ((constructor)) void __init()
		{
			codes::instance()->define("lz4",coding_lz4::getter);
		}
	};
#endif

#ifdef __GNUC__

template <char* name>
class coding_t:public coding
{
  public:
	virtual string decode(const string& str)
	{
		return iconv(str,name,"utf8");
	}
	virtual string encode(const string& str)
	{
		return iconv(str,"utf8",name);
	}

	static coding* getter()
	{
		static coding_t _mm;
		return &_mm;
	}
};

class coding_url:public coding
{
	char mc[255];
  public:

	coding_url()
	{
		memset(mc,0,255);
		for(int i=0;i<128;++i){
			char c = (char)(i&0xff);
			if (c >= '0' && c <= '9'){
				mc[i] = c - '0';
			}else if (c >= 'a' && c <= 'f'){
				mc[i] = c - 'a' + 10;
			}else if (c >= 'A' && c <= 'F'){
				mc[i] = c - 'A' + 10;
			}
		}
	}
	virtual ~coding_url()
	{}

	virtual string decode(const string& str)
	{
		string dst(str.size(),0);
		char* pdst = (char*)dst.c_str();
		char* pout = pdst;
		const char* p = str.c_str();
		const char* eof = p + str.size();
		for(;p<eof;++p){
			const char& c = *p;
			if(c == '%'){
				char ca = mc[(unsigned char)p[1]];
				char cb = mc[(unsigned char)p[2]];
				char cr = (ca << 4) | cb;
				*pout++ = cr;
				p+=2;
			}else if(c == '+'){
				*pout++ = ' ';
			}else{
				*pout++ = c;
			}
		}
		dst.erase(dst.begin() + (pout - pdst),dst.end());
		return dst;
	}

	virtual string encode(const string& str)
	{
		const char tb[] = "0123456789ABCDEF";
		string dst(str.size() * 3,0);
		char* pdst = (char*)dst.c_str();
		char* pout = pdst;
		const char* p = str.c_str();
		const char* eof = p + str.size();

		for(;p < eof;++p){
			const char& c = *p;
			if(isalpha(c) || isdigit(c)){
				*pout++ = c;
			}else if(c == ' '){
				*pout++ = '+';
			}else{
				*pout++ = '%';
				*pout++ = tb[(c >> 4)&0xf];
				*pout++ = tb[c&0xf];
			}
		}

		dst.erase(dst.begin() + (pout - pdst),dst.end());
		return dst;
	}
	static coding* getter()
	{
		static coding_url _mm;
		return &_mm;
	}
};

char c_gbk[] = "gbk";
char c_gb2312[] = "gb2312";
char c_big5[] = "big5";
char c_utf16[] = "utf16le";
char c_utf32[] = "utf32";

static __attribute__ ((constructor)) void __init()
{
	codes::instance()->define(c_gbk,coding_t<c_gbk>::getter);
	codes::instance()->define(c_gb2312,coding_t<c_gb2312>::getter);
	codes::instance()->define(c_big5,coding_t<c_big5>::getter);
	codes::instance()->define(c_utf16,coding_t<c_utf16>::getter);
	codes::instance()->define("utf-16",coding_t<c_utf16>::getter);
	codes::instance()->define("utf16",coding_t<c_utf16>::getter);
	codes::instance()->define(c_utf32,coding_t<c_utf32>::getter);
	codes::instance()->define("utf-32",coding_t<c_utf32>::getter);
	codes::instance()->define("wchar_t",coding_t<c_utf32>::getter);
	codes::instance()->define("url",coding_url::getter);
}
#else
bool __init()
{
	return true;
}

bool r = __init();
#endif

}}
