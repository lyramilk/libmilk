#include "json.h"
#include "log.h"
#include "exception.h"
#include <fstream>
#include <math.h>
#include <stdlib.h>

namespace lyramilk{namespace data
{
	//转义
	lyramilk::data::string inline escape(lyramilk::data::string s)
	{
		if(s.empty()) return s;
		lyramilk::data::string ret;
		ret.reserve(s.size() + (s.size() >> 1));

		const char* p = s.c_str();
		const char* e = p + s.size();
		for(;p < e;++p){
			switch(*p){
			  case '\b':
				ret.push_back('\\');
				ret.push_back('b');
				break;
			  case '\f':
				ret.push_back('\\');
				ret.push_back('f');
				break;
			  case '\n':
				ret.push_back('\\');
				ret.push_back('n');
				break;
			  case '\r':
				ret.push_back('\\');
				ret.push_back('r');
				break;
			  case '\t':
				ret.push_back('\\');
				ret.push_back('t');
				break;
			  case '\v':
				ret.push_back('\\');
				ret.push_back('v');
				break;
			  case '"':
				ret.push_back('\\');
				ret.push_back('"');
				break;
			  default:
				ret.push_back(*p);
			}
		}
		return ret;
	}

	bool inline _IsHexChar(std::char_traits<char>::int_type c)
	{
		return (c >= '0' && c <= '9') || (c>='a' && c <= 'f') || (c>='A' && c <= 'F');
	}
	static unsigned char unhex[256];

	unsigned int inline _ToHex(std::char_traits<char>::int_type c)
	{
		int i = unhex[c];
		if(i < 16) return i;
		throw lyramilk::exception("转换16进制数字出错");
	}
	/*
	//去转义
	lyramilk::data::string inline unescape(lyramilk::data::string s)
	{
		if(s.empty()) return s;
		lyramilk::data::string ret;
		ret.reserve(s.size());

		const char* p = s.c_str();
		const char* e = p + s.size();
		for(;p < e;++p){
			if(*p == '\\'){
				switch(p[1]){
				  case '"':
					ret.push_back('"');
					break;
				  case '/':
					ret.push_back('/');
					break;
				  case '\\':
					ret.push_back('\\');
					break;
				  case 'b':
					ret.push_back('\b');
					break;
				  case 'f':
					ret.push_back('\f');
					break;
				  case 'n':
					ret.push_back('\n');
					break;
				  case 'r':
					ret.push_back('\r');
					break;
				  case 't':
					ret.push_back('\t');
					break;
				  case 'v':
					ret.push_back('\v');
					break;
				  case 'u':
					if(_IsHexChar(p[2]) && _IsHexChar(p[3]) && _IsHexChar(p[4]) && _IsHexChar(p[5])){
						unsigned short jwc = (_ToHex(p[2]) << 12)  | (_ToHex(p[3]) << 8) | (_ToHex(p[4]) << 4) | (_ToHex(p[5]));
						unsigned wchar_t wc = jwc;
						if(jwc >= 0xd800 && jwc <= 0xdfff){
							if(p[6] != '\\') throw lyramilk::exception("错误的json字符");
							if(p[7] != 'u') throw lyramilk::exception("错误的json字符");
							if(!(_IsHexChar(p[8]) && _IsHexChar(p[9]) && _IsHexChar(p[10]) && _IsHexChar(p[11]))) throw lyramilk::exception("错误的json字符");
							unsigned short jwc2 = (_ToHex(p[8]) << 12)  | (_ToHex(p[9]) << 8) | (_ToHex(p[10]) << 4) | (_ToHex(p[11]));
							wc = (jwc2&0x03ff) + (((jwc&0x03ff) + 0x40) << 10);
						}
						if(wc < 0x80){
							ret.push_back((unsigned char)wc);
						}else if(wc < 0x800){
							ret.push_back((unsigned char)((wc>>6)&0x1f) | 0xc0);
							ret.push_back((unsigned char)((wc>>0)&0x3f) | 0x80);
						}else if(wc < 0x10000){
							ret.push_back((unsigned char)((wc>>12)&0xf) | 0xe0);
							ret.push_back((unsigned char)((wc>>6)&0x3f) | 0x80);
							ret.push_back((unsigned char)((wc>>0)&0x3f) | 0x80);
						}else if(wc < 0x200000){
							ret.push_back((unsigned char)((wc>>18)&0x7) | 0xf0);
							ret.push_back((unsigned char)((wc>>12)&0x3f) | 0x80);
							ret.push_back((unsigned char)((wc>>6)&0x3f) | 0x80);
							ret.push_back((unsigned char)((wc>>0)&0x3f) | 0x80);
						}else if(wc < 0x4000000){
							ret.push_back((unsigned char)((wc>>24)&0x3) | 0xf8);
							ret.push_back((unsigned char)((wc>>18)&0x3f) | 0x80);
							ret.push_back((unsigned char)((wc>>12)&0x3f) | 0x80);
							ret.push_back((unsigned char)((wc>>6)&0x3f) | 0x80);
							ret.push_back((unsigned char)((wc>>0)&0x3f) | 0x80);
						}else if(wc < 0x80000000){
							ret.push_back((unsigned char)((wc>>30)&0x1) | 0xfc);
							ret.push_back((unsigned char)((wc>>24)&0x3f) | 0xf0);
							ret.push_back((unsigned char)((wc>>18)&0x3f) | 0x80);
							ret.push_back((unsigned char)((wc>>12)&0x3f) | 0x80);
							ret.push_back((unsigned char)((wc>>6)&0x3f) | 0x80);
							ret.push_back((unsigned char)((wc>>0)&0x3f) | 0x80);
						}
					}
					break;
				  default:
					throw lyramilk::exception("错误的json字符");
				}
				++p;
			}else{
				ret.push_back(*p);
			}
		}
		return ret;
	}*/

	struct jsontoken
	{
		enum type {
			UNKNOW,
			STRING,	//				// String
			INTEGER,//				// Number
			DOUBLE,	//				// Number
			COLON,	//	:
			LBRACK,	//	[			// Array
			RBRACK,	//	]			// Array
			LBRACE,	//	{			// Object
			RBRACE,	//	}			// Object
			COMMA,	//	,
			TRUE,	//	true		// Boolean
			FALSE,	//	false		// Boolean
			NIL,	//	null		// Undefined
		} t;
		union{
			long long i;
			double d;
		} u;

		lyramilk::data::string s;
		jsontoken()
		{
			t = UNKNOW;
		}
	};

	jsontoken::type tokentable[256];


	class jsontokenizer
	{
		lyramilk::data::string str;
		const char* p;
		const char* e;
		std::size_t l;
	  public:
		jsontokenizer(lyramilk::data::string str)
		{
			this->str = str;
			p = this->str.c_str();
			l = this->str.size();
			e = p + l;
		}
		~jsontokenizer()
		{
		}

		bool next(jsontoken& token)
		{
label_repeat:
			if(p + 1 > e) return false;
			char c = *p;

			jsontoken::type t = tokentable[(unsigned int)c&0xff];

			switch(t){
			  case jsontoken::STRING:{
				token.s.clear();
				++p;
				while(true){
					c = *p++;
					if(c == '"'){
						break;
					}else if(c == '\\'){
						c = *p++;
						switch(c){
						  case 'b': c = '\b'; break;
						  case 'f': c = '\f'; break;
						  case 'n': c = '\n'; break;
						  case 'r': c = '\r'; break;
						  case 't': c = '\t'; break;
						  case 'v': c = '\v'; break;
						  case '\n': continue;
						  case 'u':{
							std::char_traits<char>::int_type c1 = *p++;
							if(!_IsHexChar(c1)) goto label_badchar;
							std::char_traits<char>::int_type c2 = *p++;
							if(!_IsHexChar(c2)) goto label_badchar;
							std::char_traits<char>::int_type c3 = *p++;
							if(!_IsHexChar(c3)) goto label_badchar;
							std::char_traits<char>::int_type c4 = *p++;
							if(!_IsHexChar(c4)) goto label_badchar;
							unsigned wchar_t wc = (_ToHex(c1) << 12) | (_ToHex(c2) << 8) | (_ToHex(c3) << 4) | (_ToHex(c4) << 0);
							if(wc >= 0xd800 && wc <= 0xdfff){
								if(*p++ != '\\') goto label_badchar;
								if(*p++ != 'u') goto label_badchar;
								std::char_traits<char>::int_type c1 = *p++;
								if(!_IsHexChar(c1)) goto label_badchar;
								std::char_traits<char>::int_type c2 = *p++;
								if(!_IsHexChar(c2)) goto label_badchar;
								std::char_traits<char>::int_type c3 = *p++;
								if(!_IsHexChar(c3)) goto label_badchar;
								std::char_traits<char>::int_type c4 = *p++;
								if(!_IsHexChar(c4)) goto label_badchar;
								unsigned wchar_t wc2 = (_ToHex(c1) << 12) | (_ToHex(c2) << 8) | (_ToHex(c3) << 4) | (_ToHex(c4) << 0);
								wc = (wc2&0x03ff) + (((wc&0x03ff) + 0x40) << 10);
							}
							if(wc < 0x80){
								token.s.push_back((unsigned char)wc);
							}else if(wc < 0x800){
								token.s.push_back((unsigned char)((wc>>6)&0x1f) | 0xc0);
								token.s.push_back((unsigned char)((wc>>0)&0x3f) | 0x80);
							}else if(wc < 0x10000){
								token.s.push_back((unsigned char)((wc>>12)&0xf) | 0xe0);
								token.s.push_back((unsigned char)((wc>>6)&0x3f) | 0x80);
								token.s.push_back((unsigned char)((wc>>0)&0x3f) | 0x80);
							}else if(wc < 0x200000){
								token.s.push_back((unsigned char)((wc>>18)&0x7) | 0xf0);
								token.s.push_back((unsigned char)((wc>>12)&0x3f) | 0x80);
								token.s.push_back((unsigned char)((wc>>6)&0x3f) | 0x80);
								token.s.push_back((unsigned char)((wc>>0)&0x3f) | 0x80);
							}else if(wc < 0x4000000){
								token.s.push_back((unsigned char)((wc>>24)&0x3) | 0xf8);
								token.s.push_back((unsigned char)((wc>>18)&0x3f) | 0x80);
								token.s.push_back((unsigned char)((wc>>12)&0x3f) | 0x80);
								token.s.push_back((unsigned char)((wc>>6)&0x3f) | 0x80);
								token.s.push_back((unsigned char)((wc>>0)&0x3f) | 0x80);
							}else if(wc < 0x80000000){
								token.s.push_back((unsigned char)((wc>>30)&0x1) | 0xfc);
								token.s.push_back((unsigned char)((wc>>24)&0x3f) | 0xf0);
								token.s.push_back((unsigned char)((wc>>18)&0x3f) | 0x80);
								token.s.push_back((unsigned char)((wc>>12)&0x3f) | 0x80);
								token.s.push_back((unsigned char)((wc>>6)&0x3f) | 0x80);
								token.s.push_back((unsigned char)((wc>>0)&0x3f) | 0x80);
							}
							continue;
						  }break;
						  case 'x':{
							std::char_traits<char>::int_type c1 = getchar();
							if(!_IsHexChar(c1)) goto label_badchar;
							std::char_traits<char>::int_type c2 = getchar();
							if(!_IsHexChar(c2)) goto label_badchar;
							c = (_ToHex(c1) << 4) | (_ToHex(c2) << 0);
						  }break;
						  default:{
							int iv = 0;
							if ('0' <= c && c < '8') {
								iv = c - '0';
								c = getchar();
								if ('0' <= c && c < '8') {
									iv = iv * 8 + c - '0';
									c = getchar();
									if ('0' <= c && c < '8') {
										int newiv = iv * 8 + c - '0';
										if(newiv > 0xff){
											--p;
										}else{
											iv = newiv;
										}
									}else{
										--p;
									}
								}else{
									--p;
								}
							}
						  }
						}
						token.s.push_back(c);
					}else{
						token.s.push_back(c);
					}
				}
				token.t = t;
			  }break;	//STRING
			  case jsontoken::INTEGER:
			  case jsontoken::DOUBLE:{
				const char* k = p;
				for(;k<e;++k){
					if(*k == 'e' || *k =='E' || *k =='.'){
						token.t = jsontoken::DOUBLE;
						token.u.d = strtod(p,(char**)&p);
						return true;
					}
					if(*k < '0' || *k > '9'){
						token.t = jsontoken::INTEGER;
						token.u.i = strtoll(p,(char**)&p,10);
						return true;
					}
				}
				return true;
			  }break;
			  case jsontoken::COLON:
			  case jsontoken::LBRACK:
			  case jsontoken::RBRACK:
			  case jsontoken::LBRACE:
			  case jsontoken::RBRACE:
			  case jsontoken::COMMA:{
				token.t = t;
				++p;
				return true;
			  }break;
			  case jsontoken::TRUE:{
				if(*++p != 'r') goto label_badchar;
				if(*++p != 'u') goto label_badchar;
				if(*++p != 'e') goto label_badchar;
				++p;
				token.t = t;
				return true;
			  }break;
			  case jsontoken::FALSE:{
				if(*++p != 'a') goto label_badchar;
				if(*++p != 'l') goto label_badchar;
				if(*++p != 's') goto label_badchar;
				if(*++p != 'e') goto label_badchar;
				++p;
				token.t = t;
				return true;
			  }break;
			  case jsontoken::NIL:{
				if(*++p != 'u') goto label_badchar;
				if(*++p != 'l') goto label_badchar;
				if(*++p != 'l') goto label_badchar;
				++p;
				token.t = t;
				return true;
			  }break;
			  default:
				if(*p == '\t' || *p == ' ' || *p == '\r' || *p == '\n'){
					++p;
					goto label_repeat;
				}
				return false;
			}
			return true;
label_badchar:
			return false;
		}
	};

	bool zparsearray(jsontoken& token,jsontokenizer& z,lyramilk::data::var::array& v,int deep);
	bool zparseobject(jsontoken& token,jsontokenizer& z,lyramilk::data::var::map& v,int deep);
	bool zparse(jsontoken& token,jsontokenizer& z,lyramilk::data::var::var& v,int deep);

	bool zparsearray(jsontoken& token,jsontokenizer& z,lyramilk::data::var::array& v,int deep)
	{
		while(z.next(token)){
			switch(token.t){
			  case jsontoken::STRING:{
				v.push_back(token.s);
			  }break;
			  case jsontoken::INTEGER:{
				v.push_back(token.u.i);
			  }break;
			  case jsontoken::DOUBLE:{
				v.push_back(token.u.d);
			  }break;
			  case jsontoken::LBRACK:{
				lyramilk::data::var::array subarray;
				if(zparsearray(token,z,subarray,deep + 1)){
					v.push_back(subarray);
				}
			  }break;
			  case jsontoken::RBRACK:{
				return true;
			  }break;
			  case jsontoken::LBRACE:{
				lyramilk::data::var::map subobject;
				if(zparseobject(token,z,subobject,deep + 1)){
					v.push_back(subobject);
				}
			  }break;
			  case jsontoken::COMMA:{
			  }break;
			  case jsontoken::TRUE:{
				v.push_back(true);
			  }break;
			  case jsontoken::FALSE:{
				v.push_back(false);
			  }break;
			  case jsontoken::NIL:{
				v.push_back(lyramilk::data::var::nil);
			  }break;
			  default:
				return false;
			}
		}
		return true;
	}
	bool zparseobject(jsontoken& token,jsontokenizer& z,lyramilk::data::var::map& v,int deep)
	{
		while(z.next(token)){
			switch(token.t){
			  case jsontoken::STRING:{
				lyramilk::data::string key = token.s;
				if(z.next(token) && token.t == jsontoken::COLON){
					lyramilk::data::var subvalue;
					if(!zparse(token,z,subvalue,deep + 1)) return false;
					v[key] = subvalue;
				}else{
					return false;
				}
			  }break;
			  case jsontoken::RBRACE:{
				return true;
			  }break;
			  case jsontoken::COMMA:{
			  }break;
			  default:
				return false;
			}
		}
		return true;
	}

	bool zparse(jsontoken& token,jsontokenizer& z,lyramilk::data::var& v,int deep)
	{
		if(z.next(token)){
			switch(token.t){
			  case jsontoken::STRING:{
				v = token.s;
			  }break;
			  case jsontoken::INTEGER:{
				v = token.u.i;
			  }break;
			  case jsontoken::DOUBLE:{
				v = token.u.d;
			  }break;
			  case jsontoken::COLON:{
				return true;
			  }
			  case jsontoken::LBRACK:{
				v.type(lyramilk::data::var::t_array);
				lyramilk::data::var::array& ar = v;
				return zparsearray(token,z,ar,deep + 1);
			  }break;
			  case jsontoken::LBRACE:{
				v.type(lyramilk::data::var::t_map);
				lyramilk::data::var::map& m = v;
				return zparseobject(token,z,m,deep + 1);
			  }break;
			  case jsontoken::COMMA:{
				return true;
			  }break;
			  case jsontoken::TRUE:{
				v = true;
			  }break;
			  case jsontoken::FALSE:{
				v = false;
			  }break;
			  case jsontoken::NIL:{
				v.type(lyramilk::data::var::t_invalid);
			  }break;
			  default:
				return false;
			}
		}
		return true;
	}

	static __attribute__ ((constructor)) void __init()
	{
		for(int i=0;i<256;++i){
			if(i >= 'a' && i<= 'f') unhex[i] = i - 'a' + 10;
			else if(i >= 'A' && i<= 'F') unhex[i] = i - 'A' + 10;
			else if(i >= '0' && i<= '9') unhex[i] = i - '0';
			else unhex[i] = 0xff;
			if(i == '\"') tokentable[i] = jsontoken::STRING;
			else if(i >= '0' && i <= '9') tokentable[i] = jsontoken::INTEGER;
			else if(i == '.') tokentable[i] = jsontoken::DOUBLE;
			else if(i == ':') tokentable[i] = jsontoken::COLON;
			else if(i == '[') tokentable[i] = jsontoken::LBRACK;
			else if(i == ']') tokentable[i] = jsontoken::RBRACK;
			else if(i == '{') tokentable[i] = jsontoken::LBRACE;
			else if(i == '}') tokentable[i] = jsontoken::RBRACE;
			else if(i == ',') tokentable[i] = jsontoken::COMMA;
			else if(i == 't') tokentable[i] = jsontoken::TRUE;
			else if(i == 'f') tokentable[i] = jsontoken::FALSE;
			else if(i == 'n') tokentable[i] = jsontoken::NIL;
			else tokentable[i] = jsontoken::UNKNOW;
		}
	}

	json::json(lyramilk::data::var& o) : v(o)
	{
	}

	json::~json()
	{
	}

	json& json::operator =(const lyramilk::data::var& o)
	{
		v = o;
		return *this;
	}

	lyramilk::data::string json::str() const
	{
		lyramilk::data::string jsonstr;
		if(!stringify(v,jsonstr)) throw lyramilk::exception("json生成错误");
		return jsonstr;
	}

	bool json::str(lyramilk::data::string s)
	{
		if(!parse(s,v)){
			v.type(lyramilk::data::var::t_invalid);
			return false;
		}
		return true;
	}

	bool json::stringify(const lyramilk::data::var& v,lyramilk::data::string& str)
	{
		switch(v.type()){
		  case lyramilk::data::var::t_str:
		  case lyramilk::data::var::t_wstr:{
				lyramilk::data::string s = v.str();
				str += '"' + escape(s) + '"';
			}break;
		  case lyramilk::data::var::t_bool:{
				if((bool)v){
					str += "true";
				}else{
					str += "false";
				}
			}break;
		  case lyramilk::data::var::t_int:
		  case lyramilk::data::var::t_uint:{
				str += v.str();
			}break;
		  case lyramilk::data::var::t_double:{
				str += v.str();
			}break;
		  case lyramilk::data::var::t_array:{
				str.push_back('[');
				const lyramilk::data::var::array& ar = v;
				for(lyramilk::data::var::array::const_iterator it=ar.begin();it!=ar.end();++it){
					lyramilk::data::string jsonstr;
					jsonstr.reserve(4096);
					if(json::stringify(*it,jsonstr)){
						str += jsonstr + ',';
					}
				}
				if(str[str.size() - 1] == ',') str.erase(str.end() - 1);
				str.push_back(']');
			}break;
		  case lyramilk::data::var::t_map:{
				str.push_back('{');
				const lyramilk::data::var::map& m = v;
				for(lyramilk::data::var::map::const_iterator it=m.begin();it!=m.end();++it){
					lyramilk::data::string jsonstr;
					jsonstr.reserve(4096);
					if(json::stringify(it->second,jsonstr)){
						str.reserve(str.size() + jsonstr.size() + it->first.size() + 4);
						str += '"' + escape(it->first) + "\":" + jsonstr + ',';
					}
				}
				if(str[str.size() - 1] == ',') str.erase(str.end() - 1);
				str.push_back('}');
			}break;
		  case lyramilk::data::var::t_invalid:{
				return true;
			}
		  default:{
				return false;
			}
		}
		return true;
	}

	bool json::parse(lyramilk::data::string str,lyramilk::data::var& v)
	{
		jsontoken token;
		token.s.reserve(65536);
		jsontokenizer z(str);
		return zparse(token,z,v,0);
	}
}}

std::ostream& operator << (std::ostream& os, const lyramilk::data::json& t)
{
	os << t.str();
	return os;
}

std::istream& operator >> (std::istream& is, lyramilk::data::json& t)
{
	lyramilk::data::string c;
	while(is){
		char buff[4096];
		is.read(buff,sizeof(buff));
		c.append(buff,is.gcount());
	}
	t.str(c);
	return is;
}
