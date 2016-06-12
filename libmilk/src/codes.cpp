#include "codes.h"
#include "log.h"
#include "multilanguage.h"

#include <cassert>
#include <algorithm>


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

	string iconv(string str,string from,string to)
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

	bool codes::define(string codingname,getter ctr)
	{
		transform(codingname.begin(), codingname.end(), codingname.begin(), tolower);
		std::pair<builder_type::iterator,bool> it = builder.insert(std::make_pair(codingname,ctr));
		return it.second;
	}

	bool codes::undefine(string codingname)
	{
		transform(codingname.begin(), codingname.end(), codingname.begin(), tolower);
		builder_type::iterator it = builder.find(codingname);
		if(it == builder.end()) return false;
		builder.erase(it);
		return true;
	}

	string codes::encode(string src,string codingname) throw (lyramilk::exception)
	{
		transform(codingname.begin(), codingname.end(), codingname.begin(), tolower);
		builder_type::iterator it = builder.find(codingname);
		if(it == builder.end()){
			throw lyramilk::exception(D("不可识别的编码：%s",codingname.c_str()));
		}
		return it->second()->encode(src);
	}
	string codes::decode(string src,string codingname) throw (lyramilk::exception)
	{
		transform(codingname.begin(), codingname.end(), codingname.begin(), tolower);
		builder_type::iterator it = builder.find(codingname);
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
		virtual string decode(string str)
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
		virtual string encode(string str)
		{
			std::vector<char> buf(str.size());
			while(true){
				int i = LZ4_compress_default(str.c_str(),buf.data(),str.size(),buf.size());
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
	virtual string decode(string str)
	{
		return iconv(str,name,"utf8");
	}
	virtual string encode(string str)
	{
		return iconv(str,"utf8",name);
	}

	static coding* getter()
	{
		static coding_t _mm;
		return &_mm;
	}
};

char c_gbk[] = "gbk";
char c_gb2312[] = "gb2312";

static __attribute__ ((constructor)) void __init()
{
	codes::instance()->define("gbk",coding_t<c_gbk>::getter);
	codes::instance()->define("gb2312",coding_t<c_gb2312>::getter);
}
#else
bool __init()
{
	return true;
}

bool r = __init();
#endif

}}
