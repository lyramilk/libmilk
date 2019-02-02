#include "xml.h"
#include "log.h"
#include "exception.h"
#include <fstream>
#include <math.h>
#include <stdlib.h>
#include <tinyxml.h>
#include <libmilk/dict.h>

namespace lyramilk{namespace data
{
	bool static var_to_xml(const lyramilk::data::map& m,TiXmlElement* x)
	{
		lyramilk::data::map::const_iterator it = m.begin();
		for(;it!=m.end();++it){
			if(it->first == "xml.tag" || it->first == "xml.body") continue;
			x->SetAttribute(lyramilk::data::str(it->first),lyramilk::data::str(it->second.str()));
		}

		lyramilk::data::map::const_iterator it_body = m.find("xml.body");
		if(it_body == m.end()) return true;
		if(it_body->second.type() != lyramilk::data::var::t_array) throw lyramilk::exception(D("%s应该是%s,但它是%s","xml.body","t_array",it_body->second.type_name().c_str()));

		const lyramilk::data::array& ar = it_body->second;
		for(lyramilk::data::array::const_iterator it = ar.begin();it!=ar.end();++it){
			if(it->type() == lyramilk::data::var::t_map){
				const lyramilk::data::map& childm = *it;
				lyramilk::data::map::const_iterator it_tag = childm.find("xml.tag");
				if(it_tag == childm.end()) continue;	//return false;	当xml.body中某个map不包含xml.tag时，从整个失败改成跳过这一项。
				if(it_tag->second.type() != lyramilk::data::var::t_str) throw lyramilk::exception(D("%s应该是%s,但它是%s","xml.tag","t_str",it_tag->second.type_name().c_str()));

				TiXmlElement* p = new TiXmlElement(lyramilk::data::str(it_tag->second.str()));
				x->LinkEndChild(p);
				if(!var_to_xml(childm,p)) return false;
			}else if(it->type_like(lyramilk::data::var::t_str)){
				x->LinkEndChild(new TiXmlText(lyramilk::data::str(it->str())));
			}else{
				throw lyramilk::exception(D("xml属性不支持%s类型",it->type_name().c_str()));
			}
		}
		return true;
	}

	bool static xml_to_var(const TiXmlNode* x,lyramilk::data::map& m)
	{
		for(const TiXmlAttribute* ax = x->ToElement()->FirstAttribute();ax;ax = ax->Next()){
			m[lyramilk::data::str(ax->NameTStr())] = lyramilk::data::str(ax->ValueStr());
		}

		const TiXmlNode* cx = x->FirstChild();
		if(cx == nullptr) return true;
		lyramilk::data::var& tmpv = m["xml.body"];
		tmpv.type(lyramilk::data::var::t_array);
		lyramilk::data::array& ar = tmpv;
		for(;cx;cx = cx->NextSibling()){
			int t = cx->Type();
			if(t == TiXmlNode::TINYXML_ELEMENT){
				ar.push_back(lyramilk::data::map());
				lyramilk::data::map& m = ar.back();
				m["xml.tag"] = lyramilk::data::str(cx->ValueStr());
				xml_to_var(cx->ToElement(),m);
			}else if(t == TiXmlNode::TINYXML_TEXT){
				ar.push_back(lyramilk::data::str(cx->ValueStr()));
			}
		}
		return true;
	}


	xml::xml(lyramilk::data::map& o) : m(o)
	{
	}

	xml::~xml()
	{
	}

	xml& xml::operator =(const lyramilk::data::map& o)
	{
		m = o;
		return *this;
	}

	lyramilk::data::string xml::str() const
	{
		lyramilk::data::string xmlstr;
		if(!stringify(m,&xmlstr)) throw lyramilk::exception(D("xml生成错误"));
		return xmlstr;
	}

	bool xml::str(lyramilk::data::string s)
	{
		if(!parse(s,&m)){
			return false;
		}
		return true;
	}

	bool xml::stringify(const lyramilk::data::map& m,lyramilk::data::string* pstr)
	{
		TiXmlDocument xml_doc;
		lyramilk::data::map::const_iterator it_tag = m.find("xml.tag");
		if(it_tag == m.end()) return false;
		if(it_tag->second.type() != lyramilk::data::var::var::t_str) return false;

		TiXmlElement* p = new TiXmlElement(lyramilk::data::str(it_tag->second.str()));
		xml_doc.LinkEndChild(p);
		if(!var_to_xml(m,p)) return false;


		std::string str;
		str << xml_doc;
		*pstr = lyramilk::data::str(str);
		return true;
	}

	bool xml::parse(lyramilk::data::string str,lyramilk::data::map* m)
	{
		lyramilk::data::stringstream ss(str);
		TiXmlDocument xml_doc;
		ss >> xml_doc;

		TiXmlNode* root = xml_doc.RootElement();
		if(root == nullptr) return false;
		(*m)["xml.tag"] = lyramilk::data::str(root->ValueStr());
		if(!xml_to_var(root,*m)){
			m->clear();
			return false;
		}

		return true;
	}

	lyramilk::data::string xml::stringify(const lyramilk::data::map& m)
	{
		lyramilk::data::string xmlstr;
		if(stringify(m,&xmlstr)){
			return xmlstr;
		}
		return "";
	}

	lyramilk::data::map xml::parse(lyramilk::data::string str)
	{
		lyramilk::data::map v;
		if(parse(str,&v)){
			return v;
		}
		return v;
	}

	lyramilk::data::string xml::escape(const lyramilk::data::string& s)
	{
		lyramilk::data::string ret;
		ret.reserve(s.size() + 20);

		const char* b = s.c_str();
		const char* e = b + s.size();
		for(;b < e;++b){
			switch(*b){
			  case '&':
				ret.append("&amp;");
				break;
			  case '<':
				ret.append("&lt;");
				break;
			  case '>':
				ret.append("&gt;");
				break;
			  case '\"':
				ret.append("&quot;");
				break;
			  case '\'':
				ret.append("&apos;");
				break;
			  default:
				ret.push_back(*b);
			}
		}
		return ret;
	}

	lyramilk::data::string xml::unescape(const lyramilk::data::string& s)
	{
		lyramilk::data::string ret;
		ret.reserve(s.size());

		const char* b = s.c_str();
		const char* e = b + s.size();
		while(b < e){
			if(*b == '&'){
				if(b + 6 < e){
					if(memcmp(b,"&amp;",5) == 0){
						ret.push_back('&');
						b += 5;
					}else if(memcmp(b,"&lt;",4) == 0){
						ret.push_back('<');
						b += 4;
					}else if(memcmp(b,"&gt;",4) == 0){
						ret.push_back('>');
						b += 4;
					}else if(memcmp(b,"&quot;",6) == 0){
						ret.push_back('\"');
						b += 6;
					}else if(memcmp(b,"&apos;",6) == 0){
						ret.push_back('\'');
						b += 6;
					}else{
						ret.push_back('&');
						b += 1;
					}
				}else if(b + 5 < e){
					if(memcmp(b,"&amp;",5) == 0){
						ret.push_back('&');
						b += 5;
					}else if(memcmp(b,"&lt;",4) == 0){
						ret.push_back('<');
						b += 4;
					}else if(memcmp(b,"&gt;",4) == 0){
						ret.push_back('>');
						b += 4;
					}else{
						ret.push_back('&');
						b += 1;
					}
				}else if(b + 4 < e){
					if(memcmp(b,"&lt;",4) == 0){
						ret.push_back('<');
						b += 4;
					}else if(memcmp(b,"&gt;",4) == 0){
						ret.push_back('>');
						b += 4;
					}else{
						ret.push_back('&');
						b += 1;
					}
				}else{
					ret.push_back('&');
					b += 1;
				}
			}else{
				ret.push_back(*b);
				b += 1;
			}
		}
		return ret;
	}
}}

std::ostream& operator << (std::ostream& os, const lyramilk::data::xml& t)
{
	os << t.str();
	return os;
}

std::istream& operator >> (std::istream& is, lyramilk::data::xml& t)
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
