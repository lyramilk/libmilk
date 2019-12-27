#include "yaml1.h"
#include <yaml.h>
#include "exception.h"
#include "dict.h"
#include "stringutil.h"

namespace lyramilk{namespace data
{

//	http://www.ruanyifeng.com/blog/2016/07/yaml.html

	class yaml_tokern_stream
	{
		yaml_parser_t parser;
		bool isinited;
	  public:
		yaml_tokern_stream(const lyramilk::data::string& str)
		{
			if(yaml_parser_initialize(&parser) == 1){
				yaml_parser_set_input_string(&parser,(const unsigned char*)str.c_str(),str.size());
				isinited = true;
			}else{
				isinited = false;
			}


		}

	  	virtual ~yaml_tokern_stream()
		{
			yaml_parser_delete(&parser);
		}

		lyramilk::data::string static token_name(yaml_token_t* token)
		{
			switch(token->type)
			{
			  case YAML_NO_TOKEN:
				return lyramilk::data::format("YAML_NO_TOKEN");
			  case YAML_STREAM_START_TOKEN:
				if(token->data.stream_start.encoding != YAML_UTF8_ENCODING){
					throw lyramilk::exception(D("yaml解析错误:","Source is not utf8 encoded"));
				}
				return lyramilk::data::format("YAML_STREAM_START_TOKEN");
			  case YAML_STREAM_END_TOKEN:
				return lyramilk::data::format("YAML_STREAM_END_TOKEN");
			  case YAML_VERSION_DIRECTIVE_TOKEN:
				return lyramilk::data::format("YAML_VERSION_DIRECTIVE_TOKEN");
			  case YAML_TAG_DIRECTIVE_TOKEN:
				return lyramilk::data::format("YAML_TAG_DIRECTIVE_TOKEN");
			  case YAML_DOCUMENT_START_TOKEN:
				return lyramilk::data::format("YAML_DOCUMENT_START_TOKEN");
			  case YAML_DOCUMENT_END_TOKEN:
				return lyramilk::data::format("YAML_DOCUMENT_END_TOKEN");
			  case YAML_BLOCK_SEQUENCE_START_TOKEN:
				return lyramilk::data::format("YAML_BLOCK_SEQUENCE_START_TOKEN");
			  case YAML_BLOCK_MAPPING_START_TOKEN:
				return lyramilk::data::format("YAML_BLOCK_MAPPING_START_TOKEN");
			  case YAML_BLOCK_END_TOKEN:
				return lyramilk::data::format("YAML_BLOCK_END_TOKEN");
			  case YAML_FLOW_SEQUENCE_START_TOKEN:
				return lyramilk::data::format("YAML_FLOW_SEQUENCE_START_TOKEN");
			  case YAML_FLOW_SEQUENCE_END_TOKEN:
				return lyramilk::data::format("YAML_FLOW_SEQUENCE_END_TOKEN");
			  case YAML_FLOW_MAPPING_START_TOKEN:
				return lyramilk::data::format("YAML_FLOW_MAPPING_START_TOKEN");
			  case YAML_FLOW_MAPPING_END_TOKEN:
				return lyramilk::data::format("YAML_FLOW_MAPPING_END_TOKEN");
			  case YAML_BLOCK_ENTRY_TOKEN:
				return lyramilk::data::format("YAML_BLOCK_ENTRY_TOKEN");
			  case YAML_FLOW_ENTRY_TOKEN:
				return lyramilk::data::format("YAML_FLOW_ENTRY_TOKEN");
			  case YAML_KEY_TOKEN:
				return lyramilk::data::format("YAML_KEY_TOKEN");
			  case YAML_VALUE_TOKEN:
				return lyramilk::data::format("YAML_VALUE_TOKEN");
			  case YAML_ALIAS_TOKEN:
				return lyramilk::data::format("YAML_ALIAS_TOKEN:%s",(const char*)token->data.alias.value);
			  case YAML_ANCHOR_TOKEN:
				return lyramilk::data::format("YAML_ANCHOR_TOKEN:%s",(const char*)token->data.anchor.value);
			  case YAML_TAG_TOKEN:
				return lyramilk::data::format("YAML_TAG_TOKEN\t%s\t%s",(const char*)token->data.tag.handle,(const char*)token->data.scalar.value);
			  case YAML_SCALAR_TOKEN:
				return lyramilk::data::format("YAML_SCALAR_TOKEN\t%.*s | type=%d",token->data.scalar.length,(const char*)token->data.scalar.value,token->data.scalar.style);
			  default:
				return "未知类型";
			}
		}


		bool next(yaml_token_t* token)
		{
			yaml_parser_scan(&parser, token);

			if(token->type == YAML_STREAM_START_TOKEN && token->data.stream_start.encoding != YAML_UTF8_ENCODING){
				throw lyramilk::exception(D("yaml解析错误:","Source is not utf8 encoded"));
				return false;
			}

			//COUT << token_name(token) << std::endl;

			if(token->type == YAML_NO_TOKEN) return false;
			if(token->type == YAML_STREAM_END_TOKEN) return false;
			return true;
		}

	};

	lyramilk::data::var yvalue(yaml_token_t* token)
	{
		return lyramilk::data::string((const char*)token->data.scalar.value,token->data.scalar.length);
	}

	bool inline yparsevalue(yaml_token_t* token,yaml_tokern_stream& z,lyramilk::data::var& result,lyramilk::data::map& anchor,int deep)
	{
		if(token->type == YAML_TAG_TOKEN){
			if(!z.next(token)) return false;
		}

		if(token->type == YAML_BLOCK_SEQUENCE_START_TOKEN){
			if(!z.next(token)) return false;
			result.type(lyramilk::data::var::t_array);
			lyramilk::data::array& ar = result;
			ar.reserve(16);
			while(true){
				if(token->type == YAML_BLOCK_ENTRY_TOKEN){
					if(!z.next(token)) return false;
					ar.push_back(lyramilk::data::var::nil);
					if(!yparsevalue(token,z,ar.back(),anchor,deep+1)){
						ar.pop_back();
						return false;
					}
				}else if(token->type == YAML_BLOCK_END_TOKEN){
					if(!z.next(token)) return false;
					return true;
				}else{
					return false;
				}
			}
		}else if(token->type == YAML_FLOW_SEQUENCE_START_TOKEN){
			if(!z.next(token)) return false;

			result.type(lyramilk::data::var::t_array);
			lyramilk::data::array& ar = result;
			ar.reserve(16);
			while(true){
				if(token->type == YAML_FLOW_ENTRY_TOKEN){
					if(!z.next(token)) return false;
					ar.push_back(lyramilk::data::var::nil);
					if(!yparsevalue(token,z,ar.back(),anchor,deep+1)){
						ar.pop_back();
						return false;
					}
				}else if(token->type == YAML_FLOW_SEQUENCE_END_TOKEN){
					if(!z.next(token)) return false;
					return true;
				}else{
					ar.push_back(lyramilk::data::var::nil);
					if(!yparsevalue(token,z,ar.back(),anchor,deep+1)){
						ar.pop_back();
						return false;
					}
				}
			}
		}else if(token->type == YAML_BLOCK_MAPPING_START_TOKEN){
			if(!z.next(token)) return false;

			lyramilk::data::map m;
			lyramilk::data::array ar;

			while(true){
				if(token->type == YAML_BLOCK_END_TOKEN){
					if(!z.next(token)) return false;
					return true;
				}
				if(token->type == YAML_KEY_TOKEN){
					if(!z.next(token)) return false;
					if(token->type == YAML_SCALAR_TOKEN){
						lyramilk::data::string key = yvalue(token);
						if(!z.next(token)) return false;
						if(token->type == YAML_VALUE_TOKEN){
							if(!z.next(token)) return false;
							if(!yparsevalue(token,z,m[key],anchor,deep+1)){
								return false;
							}
						}
						if(key == "<<"){
							lyramilk::data::var v = m[key];
							m.erase(key);
							if(v.type() == lyramilk::data::var::t_map){
								lyramilk::data::map& a = v;
								lyramilk::data::map::iterator it = a.begin();
								for(;it!=a.end();++it){
									m[it->first] = it->second;
								}
							}else if(v.type() == lyramilk::data::var::t_array){
								lyramilk::data::array& am = v;
								lyramilk::data::array::iterator it = am.begin();
								for(;it!=am.end();++it){
									if(it->type() == lyramilk::data::var::t_map){
										lyramilk::data::map& a = *it;
										lyramilk::data::map::iterator it = a.begin();
										for(;it!=a.end();++it){
											m[it->first] = it->second;
										}
									}
								}
							}
						}
					}else{
						return false;
					}
				}else{
					return false;
				}

				if(ar.empty()){
					result = m;
				}else{
					result = ar;
				}
			}
			if(!z.next(token)) return false;
		}else if(token->type == YAML_FLOW_MAPPING_START_TOKEN){
			if(!z.next(token)) return false;

			result.type(lyramilk::data::var::t_map);
			lyramilk::data::map& m = result;
			while(true){
				if(token->type == YAML_FLOW_MAPPING_END_TOKEN){
					if(!z.next(token)) return false;
					return true;
				}
				if(token->type == YAML_KEY_TOKEN){
					if(!z.next(token)) return false;
					if(token->type == YAML_SCALAR_TOKEN){
						lyramilk::data::string key = yvalue(token);
						if(!z.next(token)) return false;
						if(token->type == YAML_VALUE_TOKEN){
							if(!z.next(token)) return false;
							if(!yparsevalue(token,z,m[key],anchor,deep+1)){
								return false;
							}
						}
						if(key == "<<"){
							lyramilk::data::var v = m[key];
							m.erase(key);
							if(v.type() == lyramilk::data::var::t_map){
								lyramilk::data::map& a = v;
								lyramilk::data::map::iterator it = a.begin();
								for(;it!=a.end();++it){
									m[it->first] = it->second;
								}
							}
						}
						if(token->type == YAML_FLOW_ENTRY_TOKEN){
							if(!z.next(token)) return false;
						}
					}else{
						return false;
					}
				}else{
					return false;
				}
			}
			if(!z.next(token)) return false;
		}else if(token->type == YAML_SCALAR_TOKEN){
			lyramilk::data::string str = yvalue(token);
			if(str == "~"){
				result.type(lyramilk::data::var::t_invalid);
			}else{
				result = str;
			}
			if(!z.next(token)) return false;
			return true;
		}else if(token->type == YAML_KEY_TOKEN){
			if(!z.next(token)) return false;
			if(token->type == YAML_SCALAR_TOKEN){
				result.type(lyramilk::data::var::t_map);
				lyramilk::data::map&m = result;
				lyramilk::data::string key = yvalue(token);
				if(!z.next(token)) return false;
				if(token->type == YAML_VALUE_TOKEN){
					if(!z.next(token)) return false;
					if(!yparsevalue(token,z,m[key],anchor,deep+1)){
						return false;
					}
				}
				if(token->type == YAML_FLOW_ENTRY_TOKEN){
					if(!z.next(token)) return false;
				}
				return true;
			}else{
				return false;
			}
		}else if(token->type == YAML_ANCHOR_TOKEN){
			lyramilk::data::string anchor_key = (const char*)token->data.anchor.value;
			if(!z.next(token)) return false;
			if(!yparsevalue(token,z,result,anchor,deep+1)){
				return false;
			}
			anchor[anchor_key] = result;
			return true;
		}else if(token->type == YAML_ALIAS_TOKEN){
			lyramilk::data::string anchor_key = (const char*)token->data.alias.value;
			result = anchor[anchor_key];
			if(!z.next(token)) return false;
			return true;
		}else if(token->type == YAML_BLOCK_ENTRY_TOKEN){
			lyramilk::data::array ar;
			while(token->type == YAML_BLOCK_ENTRY_TOKEN){
				if(!z.next(token)) return false;
				ar.push_back(lyramilk::data::var::nil);

				if(!yparsevalue(token,z,ar.back(),anchor,deep+1)){
					return false;
				}
			}
			result = ar;
			return true;
		}else{
			throw lyramilk::exception(D("yaml解析错误:","未知描述符：%s",z.token_name(token).c_str()));
		}
		return false;
	}

	bool inline yparseroot(yaml_token_t* token,yaml_tokern_stream& z,lyramilk::data::array& ar,lyramilk::data::map& anchor,int deep)
	{
		while(z.next(token)){
			if(token->type == YAML_STREAM_START_TOKEN){
				continue;
			}
			if(token->type == YAML_DOCUMENT_START_TOKEN){
				lyramilk::data::var v;
				ar.push_back(v);
				if(!z.next(token)){
					break;
				}
				if(!yparsevalue(token,z,ar.back(),anchor,deep+1)){
					break;
				}
			}else{
				lyramilk::data::var v;
				ar.push_back(v);
				if(!yparsevalue(token,z,ar.back(),anchor,deep+1)){
					break;
				}
			}
		}
		return token->type == YAML_STREAM_END_TOKEN;
	}




	yaml::yaml(lyramilk::data::array& o) : ar(o)
	{
	}

	yaml::~yaml()
	{
	}

	yaml& yaml::operator =(const lyramilk::data::array& o)
	{
		ar = o;
		return *this;
	}

	lyramilk::data::string yaml::str() const
	{
		lyramilk::data::string yamlstr;
		if(!stringify(ar,&yamlstr)) throw lyramilk::exception(D("yaml生成错误"));
		return yamlstr;
	}

	bool yaml::str(lyramilk::data::string s)
	{
		if(!parse(s,&ar)){
			return false;
		}
		return true;
	}

	int inline yaml_stringify(yaml_document_t* doc,const lyramilk::data::var& v)
	{
		lyramilk::data::var::vt t = v.type();
		if(t == lyramilk::data::var::t_map){
			int mapid = yaml_document_add_mapping(doc,NULL,YAML_ANY_MAPPING_STYLE);
			const lyramilk::data::map& m = v;
			for(lyramilk::data::map::const_iterator it = m.begin();it!=m.end();++it){
				int kid = yaml_document_add_scalar(doc,NULL,(yaml_char_t*)it->first.c_str(),it->first.size(),YAML_ANY_SCALAR_STYLE);
				int vid = yaml_stringify(doc,it->second);

				yaml_document_append_mapping_pair(doc,mapid,kid,vid);
			}

			return mapid;
		}else if(t == lyramilk::data::var::t_array){
			int arrayid = yaml_document_add_sequence(doc,NULL,YAML_ANY_SEQUENCE_STYLE);
			const lyramilk::data::array& ar = v;
			for(lyramilk::data::array::const_iterator it = ar.begin();it!=ar.end();++it){
				int vid = yaml_stringify(doc,*it);
				yaml_document_append_sequence_item(doc,arrayid,vid);
			}
			return arrayid;
		}else if(v.type_like(lyramilk::data::var::t_str)){
			lyramilk::data::string str = v.str();
			return yaml_document_add_scalar(doc,NULL,(yaml_char_t*)str.c_str(),str.size(),YAML_ANY_SCALAR_STYLE);
		}else if(t == lyramilk::data::var::t_invalid){
			lyramilk::data::string str = "~";
			return yaml_document_add_scalar(doc,NULL,(yaml_char_t*)str.c_str(),str.size(),YAML_ANY_SCALAR_STYLE);
		}

		return 0;
	}

	bool yaml::stringify(const lyramilk::data::array& ar,lyramilk::data::string* pstr)
	{
		std::size_t reserve = 65536;
		std::size_t rz = 0;
		pstr->resize(reserve);


		yaml_emitter_t emitter;
		if(!yaml_emitter_initialize(&emitter)) return false;
		yaml_emitter_set_encoding(&emitter,YAML_UTF8_ENCODING);
		yaml_emitter_set_output_string(&emitter,(unsigned char*)pstr->data(),pstr->size(),&rz);

		yaml_emitter_open(&emitter);

		lyramilk::data::array::const_iterator it = ar.begin();
		for(;it!=ar.end();++it){
			yaml_document_t doc;
			yaml_document_initialize(&doc,NULL,NULL,NULL,0,0);

			if(yaml_stringify(&doc,*it) == 0){
				yaml_emitter_close(&emitter);
				yaml_emitter_delete(&emitter);
				return false;
			}
			yaml_emitter_dump(&emitter, &doc);
			yaml_document_delete(&doc);
		}
		yaml_emitter_close(&emitter);
		yaml_emitter_delete(&emitter);


		pstr->erase(pstr->begin() + rz,pstr->end());
		return true;
	}

	bool yaml::parse(lyramilk::data::string str,lyramilk::data::array* ar)
	{
		yaml_token_t  token;
		yaml_tokern_stream z(str);
		lyramilk::data::map anchor;
		bool result = yparseroot(&token,z,*ar,anchor,0);
		yaml_token_delete(&token);
		return result;
	}

	lyramilk::data::string yaml::stringify(const lyramilk::data::array& ar)
	{
		lyramilk::data::string yamlstr;
		if(stringify(ar,&yamlstr)){
			return yamlstr;
		}
		return "";
	}

	lyramilk::data::array yaml::parse(lyramilk::data::string str)
	{
		lyramilk::data::array v;
		if(parse(str,&v)){
			return v;
		}
		return v;
	}

}}

std::ostream& operator << (std::ostream& os, const lyramilk::data::yaml& t)
{
	os << t.str();
	return os;
}

std::istream& operator >> (std::istream& is, lyramilk::data::yaml& t)
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
