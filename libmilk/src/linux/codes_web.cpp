#include "codes.h"
#include "log.h"
#include "dict.h"

#include <memory.h>

namespace lyramilk{ namespace data
{

static char mc_url[] = "0123456789ABCDEF";
static char mc_deurl[255];
static char mc_base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static char mc_urlbase64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
static char mc_debase64[255];

////////////////////////////////////	URL
class coding_url:public lyramilk::data::coding
{
  public:

	coding_url()
	{
	}
	virtual ~coding_url()
	{}

	virtual lyramilk::data::string decode(const lyramilk::data::string& str)
	{
		lyramilk::data::string dst(str.size(),0);
		char* pdst = (char*)dst.c_str();
		char* pout = pdst;
		const char* p = str.c_str();
		const char* eof = p + str.size();
		for(;p<eof;++p){
			const char& c = *p;
			if(c == '%'){
				if(p[1] == 'u'){
					char ca = mc_deurl[(unsigned char)p[2]]&0xf;
					char cb = mc_deurl[(unsigned char)p[3]]&0xf;
					char cc = mc_deurl[(unsigned char)p[4]]&0xf;
					char cd = mc_deurl[(unsigned char)p[5]]&0xf;
					wchar_t wc = (ca<<12) | (cb<<8) | (cc<<4) | (cd<<0);

					if(wc < 0x80){
						*pout++ = (unsigned char)wc;
					}else if(wc < 0x800){
						*pout++ = (unsigned char)((wc>>6)&0x1f) | 0xc0;
						*pout++ = (unsigned char)((wc>>0)&0x3f) | 0x80;
					}else if(wc < 0x10000){
						*pout++ = (unsigned char)((wc>>12)&0xf) | 0xe0;
						*pout++ = (unsigned char)((wc>>6)&0x3f) | 0x80;
						*pout++ = (unsigned char)((wc>>0)&0x3f) | 0x80;
					}else if(wc < 0x200000){
						*pout++ = (unsigned char)((wc>>18)&0x7) | 0xf0;
						*pout++ = (unsigned char)((wc>>12)&0x3f)| 0x80;
						*pout++ = (unsigned char)((wc>>6)&0x3f) | 0x80;
						*pout++ = (unsigned char)((wc>>0)&0x3f) | 0x80;
					}else if(wc < 0x4000000){
						*pout++ = (unsigned char)((wc>>24)&0x3) | 0xf8;
						*pout++ = (unsigned char)((wc>>18)&0x3f)| 0x80;
						*pout++ = (unsigned char)((wc>>12)&0x3f)| 0x80;
						*pout++ = (unsigned char)((wc>>6)&0x3f) | 0x80;
						*pout++ = (unsigned char)((wc>>0)&0x3f) | 0x80;
					}else{
						*pout++ = (unsigned char)((wc>>30)&0x1) | 0xfc;
						*pout++ = (unsigned char)((wc>>24)&0x3f)| 0x80;
						*pout++ = (unsigned char)((wc>>18)&0x3f)| 0x80;
						*pout++ = (unsigned char)((wc>>12)&0x3f)| 0x80;
						*pout++ = (unsigned char)((wc>>6)&0x3f) | 0x80;
						*pout++ = (unsigned char)((wc>>0)&0x3f) | 0x80;
					}
					p+=5;
				}else{
					char ca = mc_deurl[(unsigned char)p[1]];
					char cb = mc_deurl[(unsigned char)p[2]];
					char cr = (ca << 4) | cb;
					*pout++ = cr;
					p+=2;
				}
			}else if(c == '+'){
				//因为rfc3986中不会出现+，所以按htm4来做，把+解析成空格。
				*pout++ = ' ';
			}else{
				*pout++ = c;
			}
		}
		dst.erase(dst.begin() + (pout - pdst),dst.end());
		return dst;
	}

	virtual lyramilk::data::string encode(const lyramilk::data::string& str)
	{
		lyramilk::data::string dst(str.size() * 3,0);
		char* pdst = (char*)dst.c_str();
		char* pout = pdst;
		const char* p = str.c_str();
		const char* eof = p + str.size();
		for(;p < eof;++p){
			const char& c = *p;
			if(isalpha(c) || isdigit(c)){
				*pout++ = c;
			/*}else if(c == ' '){
				因为html4和rfc3986的冲突，不把空格编码成+，而是编码成%20。这样无论html4还是rfc9386都可以解。
				*pout++ = '+';*/
			}else if(c == '$' || c == '-' || c == '_' || c == '.' || c == '!' || c == '*' || c == '\''){
				*pout++ = c;
			}else if(c == ';' || c == '/' || c == '?' || c == ':' || c == '@' || c == '=' || c == '&'){
				*pout++ = c;
			}else{
				*pout++ = '%';
				*pout++ = mc_url[(c >> 4)&0xf];
				*pout++ = mc_url[c&0xf];
			}
		}

		dst.erase(dst.begin() + (pout - pdst),dst.end());
		return dst;
	}
	static lyramilk::data::coding* getter()
	{
		static coding_url _mm;
		return &_mm;
	}
};
////////////////////////////////////	URL component
class coding_urlcomponent:public lyramilk::data::coding
{
  public:

	coding_urlcomponent()
	{
	}
	virtual ~coding_urlcomponent()
	{}

	virtual lyramilk::data::string decode(const lyramilk::data::string& str)
	{
		lyramilk::data::string dst(str.size(),0);
		char* pdst = (char*)dst.c_str();
		char* pout = pdst;
		const char* p = str.c_str();
		const char* eof = p + str.size();
		for(;p<eof;++p){
			const char& c = *p;
			if(c == '%'){
				if(p[1] == 'u'){
					char ca = mc_deurl[(unsigned char)p[2]]&0xf;
					char cb = mc_deurl[(unsigned char)p[3]]&0xf;
					char cc = mc_deurl[(unsigned char)p[4]]&0xf;
					char cd = mc_deurl[(unsigned char)p[5]]&0xf;
					wchar_t wc = (ca<<12) | (cb<<8) | (cc<<4) | (cd<<0);

					if(wc < 0x80){
						*pout++ = (unsigned char)wc;
					}else if(wc < 0x800){
						*pout++ = (unsigned char)((wc>>6)&0x1f) | 0xc0;
						*pout++ = (unsigned char)((wc>>0)&0x3f) | 0x80;
					}else if(wc < 0x10000){
						*pout++ = (unsigned char)((wc>>12)&0xf) | 0xe0;
						*pout++ = (unsigned char)((wc>>6)&0x3f) | 0x80;
						*pout++ = (unsigned char)((wc>>0)&0x3f) | 0x80;
					}else if(wc < 0x200000){
						*pout++ = (unsigned char)((wc>>18)&0x7) | 0xf0;
						*pout++ = (unsigned char)((wc>>12)&0x3f)| 0x80;
						*pout++ = (unsigned char)((wc>>6)&0x3f) | 0x80;
						*pout++ = (unsigned char)((wc>>0)&0x3f) | 0x80;
					}else if(wc < 0x4000000){
						*pout++ = (unsigned char)((wc>>24)&0x3) | 0xf8;
						*pout++ = (unsigned char)((wc>>18)&0x3f)| 0x80;
						*pout++ = (unsigned char)((wc>>12)&0x3f)| 0x80;
						*pout++ = (unsigned char)((wc>>6)&0x3f) | 0x80;
						*pout++ = (unsigned char)((wc>>0)&0x3f) | 0x80;
					}else{
						*pout++ = (unsigned char)((wc>>30)&0x1) | 0xfc;
						*pout++ = (unsigned char)((wc>>24)&0x3f)| 0x80;
						*pout++ = (unsigned char)((wc>>18)&0x3f)| 0x80;
						*pout++ = (unsigned char)((wc>>12)&0x3f)| 0x80;
						*pout++ = (unsigned char)((wc>>6)&0x3f) | 0x80;
						*pout++ = (unsigned char)((wc>>0)&0x3f) | 0x80;
					}
					p+=5;
				}else{
					char ca = mc_deurl[(unsigned char)p[1]];
					char cb = mc_deurl[(unsigned char)p[2]];
					char cr = (ca << 4) | cb;
					*pout++ = cr;
					p+=2;
				}
			}else if(c == '+'){
				*pout++ = ' ';
			}else{
				*pout++ = c;
			}
		}
		dst.erase(dst.begin() + (pout - pdst),dst.end());
		return dst;
	}

	virtual lyramilk::data::string encode(const lyramilk::data::string& str)
	{
		lyramilk::data::string dst(str.size() * 3,0);
		char* pdst = (char*)dst.c_str();
		char* pout = pdst;
		const char* p = str.c_str();
		const char* eof = p + str.size();
		for(;p < eof;++p){
			const char& c = *p;
			if(isalpha(c) || isdigit(c)){
				*pout++ = c;
			/*}else if(c == ' '){
				*pout++ = '+';*/
			}else if(c == '$' || c == '-' || c == '_' || c == '.' || c == '!' || c == '*' || c == '\''){
				*pout++ = c;
			}else{
				*pout++ = '%';
				*pout++ = mc_url[(c >> 4)&0xf];
				*pout++ = mc_url[c&0xf];
			}
		}

		dst.erase(dst.begin() + (pout - pdst),dst.end());
		return dst;
	}
	static lyramilk::data::coding* getter()
	{
		static coding_urlcomponent _mm;
		return &_mm;
	}
};
////////////////////////////////////	js
class coding_js:public lyramilk::data::coding
{
  public:

	coding_js()
	{
	}
	virtual ~coding_js()
	{}

	virtual lyramilk::data::string decode(const lyramilk::data::string& str)
	{
		lyramilk::data::string dst(str.size(),0);
		char* pdst = (char*)dst.c_str();
		char* pout = pdst;
		const char* p = str.c_str();
		const char* eof = p + str.size();
		for(;p<eof;++p){
			const char& c = *p;
			if(c == '%'){
				if(p[1] == 'u'){
					char ca = mc_deurl[(unsigned char)p[2]]&0xf;
					char cb = mc_deurl[(unsigned char)p[3]]&0xf;
					char cc = mc_deurl[(unsigned char)p[4]]&0xf;
					char cd = mc_deurl[(unsigned char)p[5]]&0xf;
					wchar_t wc = (ca<<12) | (cb<<8) | (cc<<4) | (cd<<0);

					if(wc < 0x80){
						*pout++ = (unsigned char)wc;
					}else if(wc < 0x800){
						*pout++ = (unsigned char)((wc>>6)&0x1f) | 0xc0;
						*pout++ = (unsigned char)((wc>>0)&0x3f) | 0x80;
					}else if(wc < 0x10000){
						*pout++ = (unsigned char)((wc>>12)&0xf) | 0xe0;
						*pout++ = (unsigned char)((wc>>6)&0x3f) | 0x80;
						*pout++ = (unsigned char)((wc>>0)&0x3f) | 0x80;
					}else if(wc < 0x200000){
						*pout++ = (unsigned char)((wc>>18)&0x7) | 0xf0;
						*pout++ = (unsigned char)((wc>>12)&0x3f) | 0x80;
						*pout++ = (unsigned char)((wc>>6)&0x3f) | 0x80;
						*pout++ = (unsigned char)((wc>>0)&0x3f) | 0x80;
					}else if(wc < 0x4000000){
						*pout++ = (unsigned char)((wc>>24)&0x3) | 0xf8;
						*pout++ = (unsigned char)((wc>>18)&0x3f) | 0x80;
						*pout++ = (unsigned char)((wc>>12)&0x3f) | 0x80;
						*pout++ = (unsigned char)((wc>>6)&0x3f) | 0x80;
						*pout++ = (unsigned char)((wc>>0)&0x3f) | 0x80;
					}else{
						*pout++ = (unsigned char)((wc>>30)&0x1) | 0xfc;
						*pout++ = (unsigned char)((wc>>24)&0x3f) | 0x80;
						*pout++ = (unsigned char)((wc>>18)&0x3f) | 0x80;
						*pout++ = (unsigned char)((wc>>12)&0x3f) | 0x80;
						*pout++ = (unsigned char)((wc>>6)&0x3f) | 0x80;
						*pout++ = (unsigned char)((wc>>0)&0x3f) | 0x80;
					}
					p+=5;
				}else{
					char ca = mc_deurl[(unsigned char)p[1]];
					char cb = mc_deurl[(unsigned char)p[2]];
					char cr = (ca << 4) | cb;
					*pout++ = cr;
					p+=2;
				}
			}else{
				*pout++ = c;
			}
		}
		dst.erase(dst.begin() + (pout - pdst),dst.end());
		return dst;
	}

	virtual lyramilk::data::string encode(const lyramilk::data::string& str)
	{
		lyramilk::data::string ustr = iconv(str,"utf8","utf16le");
		const char tb[] = "0123456789ABCDEF";
		lyramilk::data::string dst(ustr.size() * 6,0);
		char* pdst = (char*)dst.c_str();
		char* pout = pdst;
		const unsigned short* p = (const unsigned short*)ustr.c_str();
		const unsigned short* eof = p + (ustr.size() >> 1);

		for(;p < eof;++p){
			const unsigned short& c = *p;

			if(c&0xff00){
				*pout++ = '%';
				*pout++ = 'u';

				*pout++ = tb[(c >> 12)&0xf];
				*pout++ = tb[(c >> 8)&0xf];
				*pout++ = tb[(c >> 4)&0xf];
				*pout++ = tb[(c >> 0)&0xf];
			}else if(isalpha(c) || isdigit(c)){
				*pout++ = c;
			}else{
				*pout++ = '%';
				*pout++ = tb[(c >> 4)&0xf];
				*pout++ = tb[c&0xf];
			}
		}
		dst.erase(dst.begin() + (pout - pdst),dst.end());
		return dst;

	}
	static lyramilk::data::coding* getter()
	{
		static coding_js _mm;
		return &_mm;
	}
};

////////////////////////////////////	Base64
class coding_b64:public lyramilk::data::coding
{
  public:
	virtual lyramilk::data::string decode(const lyramilk::data::string& str)
	{
		lyramilk::data::string ret;
		ret.reserve(str.size());
		unsigned int len = str.size();
		if(len == 0) return "";
		unsigned int m = len&0x3;
		if(!m)m=4;
		len -= m;
		const unsigned char* b = (const unsigned char*)str.c_str();
		const unsigned char* e = b + len;

		union{
			unsigned int i;
			unsigned char c[3];
		}u;
		for(const unsigned char* p = b;p<e;p+=4){
			unsigned int i1 = mc_debase64[p[0]];
			unsigned int i2 = mc_debase64[p[1]];
			unsigned int i3 = mc_debase64[p[2]];
			unsigned int i4 = mc_debase64[p[3]];
			if((i1 | i2 | i3 | i4)&0x80) throw lyramilk::data::coding_exception(D("解码%s时发生错误：不该出现该字符","base64"));
			u.i = i1<<18;
			u.i |= i2<<12;
			u.i |= i3<<6;
			u.i |= i4;

			ret.push_back(u.c[2]);
			ret.push_back(u.c[1]);
			ret.push_back(u.c[0]);
		}
		if(m == 4){
			const unsigned char* p = e;
			unsigned int i1 = mc_debase64[p[0]];
			unsigned int i2 = mc_debase64[p[1]];
			unsigned int i3 = mc_debase64[p[2]];
			unsigned int i4 = mc_debase64[p[3]];
			if((i1 | i2 | i3 | i4)&0x80) throw lyramilk::data::coding_exception(D("解码%s时发生错误：不该出现该字符","base64"));
			u.i = i1<<18;
			u.i |= i2<<12;
			u.i |= i3<<6;
			u.i |= i4;
			ret.push_back(u.c[2]);
			ret.push_back(u.c[1]);
			ret.push_back(u.c[0]);
		}else if(m == 3){
			const unsigned char* p = e;
			unsigned int i1 = mc_debase64[p[0]];
			unsigned int i2 = mc_debase64[p[1]];
			unsigned int i3 = mc_debase64[p[2]];
			if((i1 | i2 | i3)&0x80) throw lyramilk::data::coding_exception(D("解码%s时发生错误：不该出现该字符","base64"));
			u.i = i1<<18;
			u.i |= i2<<12;
			u.i |= i3<<6;
			ret.push_back(u.c[2]);
			ret.push_back(u.c[1]);
			ret.push_back(u.c[0]);
		}else if(m == 2){
			const unsigned char* p = e;
			unsigned int i1 = mc_debase64[p[0]];
			unsigned int i2 = mc_debase64[p[1]];
			if((i1 | i2)&0x80) throw lyramilk::data::coding_exception(D("解码%s时发生错误：不该出现该字符","base64"));
			u.i = i1<<18;
			u.i |= i2<<12;
			ret.push_back(u.c[2]);
			ret.push_back(u.c[1]);
		}else if(m == 1){
			const unsigned char* p = e;
			unsigned int i1 = mc_debase64[p[0]];
			if(i1&0x80) throw lyramilk::data::coding_exception(D("解码%s时发生错误：不该出现该字符","base64"));
			u.i = i1<<18;
			ret.push_back(u.c[2]);
		}
		return ret;
	}
	virtual lyramilk::data::string encode(const lyramilk::data::string& str)
	{
		lyramilk::data::string ret;
		ret.reserve(str.size()*3+1);
		unsigned int len = str.size();
		if(len == 0) return "";
		unsigned int m = len%3;
		len -= m;
		const unsigned char* b = (const unsigned char*)str.c_str();
		const unsigned char* e = b + len;
		for(const unsigned char* p = b;p<e;p+=3){
			unsigned int i1 = p[0] >> 2;
			unsigned int i2 = ((p[0] & 3) << 4) | (p[1] >> 4);
			unsigned int i3 = ((p[1] & 0xf) << 2) | (p[2] >> 6);
			unsigned int i4 = p[2] & 0x3f;
			ret.push_back(mc_base64[i1]);
			ret.push_back(mc_base64[i2]);
			ret.push_back(mc_base64[i3]);
			ret.push_back(mc_base64[i4]);
		}
		if(m == 2){
			const unsigned char* p = e;
			unsigned int i1 = p[0] >> 2;
			unsigned int i2 = ((p[0] & 3) << 4) | (p[1] >> 4);
			unsigned int i3 = ((p[1] & 0xf) << 2);
			ret.push_back(mc_base64[i1]);
			ret.push_back(mc_base64[i2]);
			ret.push_back(mc_base64[i3]);
			ret.push_back('=');
		}else if(m == 1){
			const unsigned char* p = e;
			unsigned int i1 = p[0] >> 2;
			unsigned int i2 = ((p[0] & 3) << 4);
			ret.push_back(mc_base64[i1]);
			ret.push_back(mc_base64[i2]);
			ret.push_back('=');
			ret.push_back('=');
		}
		return ret;
	}

	static lyramilk::data::coding* getter()
	{
		static coding_b64 _mm;
		return &_mm;
	}
};

////////////////////////////////////	Url safe base64
class coding_urlb64:public lyramilk::data::coding
{
  public:
	virtual lyramilk::data::string decode(const lyramilk::data::string& str)
	{
		lyramilk::data::string ret;
		ret.reserve(str.size());
		unsigned int len = str.size();
		if(len == 0) return "";
		unsigned int m = len&0x3;
		if(!m)m=4;
		len -= m;
		const unsigned char* b = (const unsigned char*)str.c_str();
		const unsigned char* e = b + len;

		union{
			unsigned int i;
			unsigned char c[3];
		}u;
		for(const unsigned char* p = b;p<e;p+=4){
			unsigned int i1 = mc_debase64[p[0]];
			unsigned int i2 = mc_debase64[p[1]];
			unsigned int i3 = mc_debase64[p[2]];
			unsigned int i4 = mc_debase64[p[3]];
			if((i1 | i2 | i3 | i4)&0x80) throw lyramilk::data::coding_exception(D("解码%s时发生错误：不该出现该字符","base64"));
			u.i = i1<<18;
			u.i |= i2<<12;
			u.i |= i3<<6;
			u.i |= i4;

			ret.push_back(u.c[2]);
			ret.push_back(u.c[1]);
			ret.push_back(u.c[0]);
		}
		if(m == 4){
			const unsigned char* p = e;
			unsigned int i1 = mc_debase64[p[0]];
			unsigned int i2 = mc_debase64[p[1]];
			unsigned int i3 = mc_debase64[p[2]];
			unsigned int i4 = mc_debase64[p[3]];
			if((i1 | i2 | i3 | i4)&0x80) throw lyramilk::data::coding_exception(D("解码%s时发生错误：不该出现该字符","base64"));
			u.i = i1<<18;
			u.i |= i2<<12;
			u.i |= i3<<6;
			u.i |= i4;
			if(u.c[2])ret.push_back(u.c[2]);
			if(u.c[1])ret.push_back(u.c[1]);
			if(u.c[0])ret.push_back(u.c[0]);
		}else if(m == 3){
			const unsigned char* p = e;
			unsigned int i1 = mc_debase64[p[0]];
			unsigned int i2 = mc_debase64[p[1]];
			unsigned int i3 = mc_debase64[p[2]];
			if((i1 | i2 | i3)&0x80) throw lyramilk::data::coding_exception(D("解码%s时发生错误：不该出现该字符","base64"));
			u.i = i1<<18;
			u.i |= i2<<12;
			u.i |= i3<<6;
			if(u.c[2])ret.push_back(u.c[2]);
			if(u.c[1])ret.push_back(u.c[1]);
			if(u.c[0])ret.push_back(u.c[0]);
		}else if(m == 2){
			const unsigned char* p = e;
			unsigned int i1 = mc_debase64[p[0]];
			unsigned int i2 = mc_debase64[p[1]];
			if((i1 | i2)&0x80) throw lyramilk::data::coding_exception(D("解码%s时发生错误：不该出现该字符","base64"));
			u.i = i1<<18;
			u.i |= i2<<12;
			if(u.c[2])ret.push_back(u.c[2]);
			if(u.c[1])ret.push_back(u.c[1]);
			if(u.c[0])ret.push_back(u.c[0]);
		}else if(m == 1){
			const unsigned char* p = e;
			unsigned int i1 = mc_debase64[p[0]];
			if(i1&0x80) throw lyramilk::data::coding_exception(D("解码%s时发生错误：不该出现该字符","base64"));
			u.i = i1<<18;
			if(u.c[2])ret.push_back(u.c[2]);
			if(u.c[1])ret.push_back(u.c[1]);
			if(u.c[0])ret.push_back(u.c[0]);
		}
		return ret;
	}
	virtual lyramilk::data::string encode(const lyramilk::data::string& str)
	{
		lyramilk::data::string ret;
		ret.reserve(str.size()*3+1);
		unsigned int len = str.size();
		if(len == 0) return "";
		unsigned int m = len%3;
		len -= m;
		const unsigned char* b = (const unsigned char*)str.c_str();
		const unsigned char* e = b + len;
		for(const unsigned char* p = b;p<e;p+=3){
			unsigned int i1 = p[0] >> 2;
			unsigned int i2 = ((p[0] & 3) << 4) | (p[1] >> 4);
			unsigned int i3 = ((p[1] & 0xf) << 2) | (p[2] >> 6);
			unsigned int i4 = p[2] & 0x3f;
			ret.push_back(mc_urlbase64[i1]);
			ret.push_back(mc_urlbase64[i2]);
			ret.push_back(mc_urlbase64[i3]);
			ret.push_back(mc_urlbase64[i4]);
		}
		if(m == 2){
			const unsigned char* p = e;
			unsigned int i1 = p[0] >> 2;
			unsigned int i2 = ((p[0] & 3) << 4) | (p[1] >> 4);
			unsigned int i3 = ((p[1] & 0xf) << 2) | (p[2] >> 6);
			ret.push_back(mc_urlbase64[i1]);
			ret.push_back(mc_urlbase64[i2]);
			ret.push_back(mc_urlbase64[i3]);
		}else if(m == 1){
			const unsigned char* p = e;
			unsigned int i1 = p[0] >> 2;
			unsigned int i2 = ((p[0] & 3) << 4);
			ret.push_back(mc_urlbase64[i1]);
			ret.push_back(mc_urlbase64[i2]);
		}
		return ret;
	}

	static lyramilk::data::coding* getter()
	{
		static coding_urlb64 _mm;
		return &_mm;
	}
};


static bool ___init()
{
	// 初始化url编码表
	memset(mc_deurl,0,255);
	for(int i=0;i<128;++i){
		char c = (char)(i&0xff);
		if (c >= '0' && c <= '9'){
			mc_deurl[i] = c - '0';
		}else if (c >= 'a' && c <= 'f'){
			mc_deurl[i] = c - 'a' + 10;
		}else if (c >= 'A' && c <= 'F'){
			mc_deurl[i] = c - 'A' + 10;
		}
	}

	// 初始化逆base64编码表
	memset(mc_debase64,-1,255);
	for(unsigned int i=0;i<sizeof(mc_base64);++i){
		unsigned char c = mc_base64[i];
		mc_debase64[c] = i;
		if(c == '+'){
			mc_debase64['-'] = i;
		}
		if(c == '/'){
			mc_debase64['_'] = i;
		}
	}
	mc_debase64['='] = 0;

	lyramilk::data::codes::instance()->define("url",coding_url::getter);
	lyramilk::data::codes::instance()->define("urlcomponent",coding_urlcomponent::getter);
	lyramilk::data::codes::instance()->define("js",coding_js::getter);
	lyramilk::data::codes::instance()->define("base64",coding_b64::getter);
	lyramilk::data::codes::instance()->define("urlbase64",coding_urlb64::getter);
	return true;
}

#ifdef __GNUC__

static __attribute__ ((constructor)) void __init()
{
	___init();
}
#else
bool r = __init();
#endif

}}
