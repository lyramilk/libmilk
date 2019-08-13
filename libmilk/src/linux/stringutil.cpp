#include "stringutil.h"
#include <stdarg.h>
#include <algorithm>

namespace lyramilk{namespace data
{
	lyramilk::data::string format(const char* fmt,...)
	{
		char buff[256];
		va_list va;
		int cnt;
		va_start(va, fmt);
		cnt = vsnprintf(buff,256, fmt, va);
		va_end(va);
		if(cnt < 256){
			return lyramilk::data::string(buff,cnt);
		}

		std::vector<char> buf(cnt + 1);
		va_start(va, fmt);
		vsprintf(buf.data(),fmt,va);
		va_end(va);
		return lyramilk::data::string(buf.begin(),buf.end());
	}


	lyramilk::data::strings split(const lyramilk::data::string& data,const lyramilk::data::string& sep)
	{
		lyramilk::data::strings lines;
		lines.reserve(10);
		std::size_t posb = 0;
		do{
			std::size_t poscrlf = data.find(sep,posb);
			if(poscrlf == data.npos){
				lines.push_back(data.substr(posb));
				posb = poscrlf;
			}else{
				lines.push_back(data.substr(posb,poscrlf - posb));
				posb = poscrlf + sep.size();
			}
		}while(posb != data.npos);
		return lines;
	}

	lyramilk::data::strings path_split(const lyramilk::data::string& path)
	{
		lyramilk::data::strings ret;
		lyramilk::data::strings v = split(path,"/");
		lyramilk::data::strings::iterator it = v.begin();
		if(it==v.end()) return ret;
		ret.reserve(10);
		ret.push_back(*it);
		for(++it;it!=v.end();++it){
			if(*it == ".") continue;
			if(*it == ".." && !ret.empty()){
				ret.pop_back();
				continue;
			}
			ret.push_back(*it);
		}
		return ret;
	}

	lyramilk::data::string trim(const lyramilk::data::string& data,const lyramilk::data::string& pattern)
	{
		if (data.empty()) return data;
		std::size_t pos1 = data.find_first_not_of(pattern);
		std::size_t pos2 = data.find_last_not_of(pattern);
		if(pos2 == data.npos){
			return "";
		}
		pos2++;
		std::size_t des = pos2-pos1;
		if(des == data.size()) return data;
		return data.substr(pos1,des);
	}

	lyramilk::data::string ltrim(const lyramilk::data::string& data,const lyramilk::data::string& pattern)
	{
		if (data.empty()) return data;
		std::size_t pos1 = data.find_first_not_of(pattern);
		return data.substr(pos1);
	}

	lyramilk::data::string rtrim(const lyramilk::data::string& data,const lyramilk::data::string& pattern)
	{
		if (data.empty()) return data;
		std::size_t pos1 = 0;
		std::size_t pos2 = data.find_last_not_of(pattern);
		if(pos2 == data.npos){
			return "";
		}
		pos2++;
		std::size_t des = pos2-pos1;
		if(des == data.size()) return data;
		return data.substr(pos1,des);
	}

	lyramilk::data::string lower_case(const lyramilk::data::string& src)
	{
		lyramilk::data::string ret;
		ret.resize(src.size());
		std::transform(src.begin(), src.end(), ret.begin(), tolower);
		return ret;
	}

	lyramilk::data::string upper_case(const lyramilk::data::string& src)
	{
		lyramilk::data::string ret;
		ret.resize(src.size());
		std::transform(src.begin(), src.end(), ret.begin(), toupper);
		return ret;
	}


	lyramilk::data::string replace(const lyramilk::data::string& str,const lyramilk::data::string& src,const lyramilk::data::string& dest)
	{
		lyramilk::data::string ret;

		lyramilk::data::string::size_type pos, tpos = 0;
		bool bHasReplaced = false;

		pos = str.find(src);
		while(pos != lyramilk::data::string::npos) {
			ret += str.substr(tpos, pos - tpos) + dest;		
			tpos = pos + src.length();
			pos = str.find(src, tpos);
			bHasReplaced = true;
		}

		if(bHasReplaced) {
			ret += str.substr(tpos);
			return ret;
		}
		return str;
	}

}}
