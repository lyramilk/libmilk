#include "json.h"
#include "log.h"
//#include "stringutil.h"
#include <fstream>
extern "C" {
	#include <json-c/json.h>
}
/*
#define NDEBUG
#include <JSONOptions.h>
#include <libjson.h>
*/
namespace lyramilk{namespace data
{

	bool node_to_var(json_object* node,var& v,int dep = 0)
	{
		json_type type = json_object_get_type(node);

		switch(type){
		  case json_type_null:{
				return true;
			}
			break;
		  case json_type_boolean:{
				bool b = json_object_get_boolean(node) != FALSE;
				v = b;
				return true;
			}
			break;
		  case json_type_double:{
				double b = json_object_get_double(node);
				v = b;
				return true;
			}
			break;
		  case json_type_int:{
				int32_t i = json_object_get_int(node);
				v = i;
				return true;
			}
			break;
		  case json_type_string:{
				int len = json_object_get_string_len(node);
				const char* p = json_object_get_string(node);
				v = string(p,len);
				return true;
			}
			break;
		  case json_type_object:{
				var::map m;
				json_object_iter it;
				json_object_object_foreachC(node,it){
					var value;
					node_to_var(it.val,value,dep+1);
					if(value.type() != var::t_invalid){
						m[it.key] = value;
					}
					//json_object_put(it.val);
				}
				v = m;
				return true;
			}
			break;
		  case json_type_array:{
				var::array va;
				int idx = json_object_array_length(node);
				for(int i=0;i<idx;++i){
					json_object* obj = json_object_array_get_idx(node,i);
					var value;
					node_to_var(obj,value,dep+1);
					if(value.type() != var::t_invalid){
						va.push_back(value);
					}
					//json_object_put(obj);
				}
				v = va;
				return true;
			}
			break;
		  default:{
			}
		}
		return false;
	}

	json_object* var_to_node(const var& v)
	{
		switch(v.type()){
		  case var::t_bin:{
			{
				return nullptr;
			}
			break;
		  }
		  case var::t_str:
		  case var::t_wstr:
			{
				string str = v;
				return json_object_new_string_len(str.c_str(),str.size());
			}
			break;
		  case var::t_bool:
			{
				return json_object_new_boolean(v);
			}
			break;
		  case var::t_int:
			{
				return json_object_new_int64(v);
			}
			break;
		  case var::t_uint:
			{
				return json_object_new_int64(v);
			}
			break;
		  case var::t_double:
			{
				return json_object_new_double(v);
			}
			break;
		  case var::t_array:
			{
				struct json_object *ar = json_object_new_array();
				const var::array &a = v;
				for(var::array::const_iterator it=a.begin();it!=a.end();++it){
					if(it->type() == var::t_invalid) continue;
					struct json_object *o = var_to_node(*it);
					json_object_array_add(ar,o);
					//json_object_put(o);
				}
				return ar;
			}
			break;
		  case var::t_map:
			{
				struct json_object *om = json_object_new_object();
				const var::map &m = v;
				for(var::map::const_iterator it=m.begin();it!=m.end();++it){
					if(it->second.type() == var::t_invalid) continue;
					string key = it->first;
					struct json_object *o = var_to_node(it->second);
					json_object_object_add(om, key.c_str(), o);
					//json_object_put(o);
				}
				return om;
			}
			break;
		  case var::t_user:
			{
				return nullptr;
			}
		  case var::t_invalid:
			{
				return nullptr;
			}
			break;
		}
		throw var::type_invalid("json::var类型错误");
	}

	inline string tabs(int i)
	{
		return string(i,'\t');
	}

	string jsonobject_to_string(json_object* node,int dep)
	{
		json_type type = json_object_get_type(node);
		switch(type){
		  case json_type_object:{
				string tabstr = tabs(dep);
				string result;
				json_object_iter it;
				json_object_object_foreachC(node,it){
					json_type chtype = json_object_get_type(it.val);
					switch(chtype){
					  case json_type_object:{
						result += tabs(dep) + "\"" + it.key + "\" : {\r\n";
						result += jsonobject_to_string(it.val,dep + 1);
						result += tabs(dep) + "},\r\n";
					  }break;
					  case json_type_array:{
						result += tabs(dep) + "\"" + it.key + "\" : [\r\n";
						result += jsonobject_to_string(it.val,dep + 1);
						result += tabs(dep) + "],\r\n";
					  }break;
					  default:
						result += tabs(dep) + "\"" + it.key + "\" : " + jsonobject_to_string(it.val,dep + 1) + ",\r\n";
					}
				}
				/*  删除容器中最后一个元素的逗号*/
				if(*(result.end() - 3) == ','){
					result.erase(result.end() - 3);
				}
				/**/
				return result;
			}
			break;
		  case json_type_array:{
				string result;
				var::array va;
				int idx = json_object_array_length(node);
				for(int i=0;i<idx;++i){
					json_object* obj = json_object_array_get_idx(node,i);
					json_type chtype = json_object_get_type(obj);
					switch(chtype){
					  case json_type_object:{
						result += tabs(dep) + "{\r\n";
						result += jsonobject_to_string(obj,dep + 1);
						result += tabs(dep) + "},\r\n";
					  }break;
					  case json_type_array:{
						result += tabs(dep) + "[\r\n";
						result += jsonobject_to_string(obj,dep + 1);
						result += tabs(dep) + "],\r\n";
					  }break;
					  default:
						result += tabs(dep) + jsonobject_to_string(obj,dep + 1) + ",\r\n";
					}
				}


				/*  删除容器中最后一个元素的逗号*/
				if(*(result.end() - 3) == ','){
					result.erase(result.end() - 3);
				}
				/**/
				return result;
			}
			break;
		  default:{
				return json_object_to_json_string(node);
			}
		}
	}

	string jsonobject_to_string(json_object* node)
	{
		json_type chtype = json_object_get_type(node);
		string str;
		switch(chtype){
		  case json_type_object:{
			str = "{\r\n" + jsonobject_to_string(node,1) + "}\r\n";
		  }break;
		  case json_type_array:{
			str = "[\r\n" + jsonobject_to_string(node,1) + "]\r\n";
		  }break;
		  default:
			str = jsonobject_to_string(node,0);
		}
		return str;
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
		struct json_object* jroot = var_to_node(v);
		if(!jroot){
			return "";
		}
		lyramilk::data::string retstr = jsonobject_to_string(jroot);
		//lyramilk::data::string retstr = json_object_to_json_string(jroot);
		json_object_put(jroot);
		return retstr;
	}

	struct myjsontoken
	{
		json_tokener* token;
		myjsontoken()
		{
			token = json_tokener_new();
		}

		~myjsontoken()
		{
			json_tokener_free(token);
		}

		lyramilk::data::string err()
		{
			return json_tokener_error_desc(json_tokener_get_error(token));
		}

	};

	bool json::str(lyramilk::data::string s)
	{
		myjsontoken qt;
		struct json_object* jroot = json_tokener_parse_ex(qt.token,s.c_str(),s.size());
		if(!jroot){
			lyramilk::klog(lyramilk::log::warning,"lyramilk.data.json.parse") << qt.err() << std::endl;
			return false;
		}
		node_to_var(jroot,v);
		json_object_put(jroot);
		return true;
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
