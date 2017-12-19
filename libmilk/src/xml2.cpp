#include "xml.h"
#include "log.h"
#include "exception.h"
#include <fstream>
#include <math.h>
#include <stdlib.h>
#include <tinyxml2.h>
#include <libmilk/multilanguage.h>

namespace lyramilk{namespace data
{
	bool static var_to_xml(tinyxml2::XMLDocument* doc,const lyramilk::data::var::map& m,tinyxml2::XMLElement* x)
	{
		lyramilk::data::var::map::const_iterator it = m.begin();
		for(;it!=m.end();++it){
			if(it->first == "xml.tag" || it->first == "xml.body") continue;
			x->SetAttribute(it->first.c_str(),it->second.str().c_str());
		}

		lyramilk::data::var::map::const_iterator it_body = m.find("xml.body");
		if(it_body == m.end()) return true;
		if(it_body->second.type() != lyramilk::data::var::t_array) throw lyramilk::exception(D("%s应该是%s,但它是%s","xml.body","t_array",it_body->second.type_name().c_str()));

		const lyramilk::data::var::array& ar = it_body->second;
		for(lyramilk::data::var::array::const_iterator it = ar.begin();it!=ar.end();++it){
			if(it->type() == lyramilk::data::var::t_map){
				const lyramilk::data::var::map& childm = *it;
				lyramilk::data::var::map::const_iterator it_tag = childm.find("xml.tag");
				if(it_tag == childm.end()) return false;
				if(it_tag->second.type() != lyramilk::data::var::t_str) throw lyramilk::exception(D("%s应该是%s,但它是%s","xml.tag","t_str",it_tag->second.type_name().c_str()));

				tinyxml2::XMLElement* p = doc->NewElement(it_tag->second.str().c_str());
				x->LinkEndChild(p);
				if(!var_to_xml(doc,childm,p)) return false;
			}else if(it->type_like(lyramilk::data::var::t_str)){
				x->LinkEndChild(doc->NewText(it->str().c_str()));
			}else{
				throw lyramilk::exception(D("xml属性不支持%s类型",it->type_name().c_str()));
			}
		}
		return true;
	}

	bool static xml_to_var(tinyxml2::XMLDocument* doc,const tinyxml2::XMLNode* x,lyramilk::data::var::map& m)
	{
		for(const tinyxml2::XMLAttribute* ax = x->ToElement()->FirstAttribute();ax;ax = ax->Next()){
			m[ax->Name()] = ax->Value();
		}

		const tinyxml2::XMLNode* cx = x->FirstChild();
		if(cx == nullptr) return true;
		lyramilk::data::var& tmpv = m["xml.body"];
		tmpv.type(lyramilk::data::var::t_array);
		lyramilk::data::var::array& ar = tmpv;
		for(;cx;cx = cx->NextSibling()){
			const tinyxml2::XMLElement* ele = cx->ToElement();
			if(ele){
				ar.push_back(lyramilk::data::var::map());
				lyramilk::data::var::map& m = ar.back();
				m["xml.tag"] = cx->Value();
				xml_to_var(doc,ele,m);
				continue;
			}
			
			const tinyxml2::XMLText* txt = cx->ToText();
			if(txt){
				ar.push_back(cx->Value());
			}
		}
		return true;
	}


	xml::xml(lyramilk::data::var::map& o) : m(o)
	{
	}

	xml::~xml()
	{
	}

	xml& xml::operator =(const lyramilk::data::var::map& o)
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

	bool xml::stringify(const lyramilk::data::var::map& m,lyramilk::data::string* pstr)
	{
		if(!pstr) return false;
		tinyxml2::XMLDocument xml_doc;
		lyramilk::data::var::map::const_iterator it_tag = m.find("xml.tag");
		if(it_tag == m.end()) return false;
		if(it_tag->second.type() != lyramilk::data::var::var::t_str) return false;

		tinyxml2::XMLElement* p = xml_doc.NewElement(it_tag->second.str().c_str());
		if(!var_to_xml(&xml_doc,m,p)) return false;
		xml_doc.LinkEndChild(p);

		tinyxml2::XMLPrinter printer;
		xml_doc.Print(&printer);

		pstr->assign(printer.CStr(),printer.CStrSize());
		return true;
	}

	bool xml::parse(lyramilk::data::string str,lyramilk::data::var::map* m)
	{
		tinyxml2::XMLDocument xml_doc;
		if(tinyxml2::XML_SUCCESS != xml_doc.Parse(str.c_str(),str.size())){
			return false;
		}

		tinyxml2::XMLNode* root = xml_doc.RootElement();
		if(root == nullptr) return false;
		(*m)["xml.tag"] = root->Value();
		if(!xml_to_var(&xml_doc,root,*m)){
			m->clear();
			return false;
		}

		return true;
	}

	lyramilk::data::string xml::stringify(const lyramilk::data::var::map& m)
	{
		lyramilk::data::string xmlstr;
		if(stringify(m,&xmlstr)){
			return xmlstr;
		}
		return "";
	}

	lyramilk::data::var::map xml::parse(lyramilk::data::string str)
	{
		lyramilk::data::var::map v;
		if(parse(str,&v)){
			return v;
		}
		return v;
	}

	lyramilk::data::string xml::escape(const lyramilk::data::string& s)
	{
		TODO();
	}

	lyramilk::data::string xml::unescape(const lyramilk::data::string& s)
	{
		TODO();
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
