﻿#define  _CRT_SECURE_NO_WARNINGS
#include "multilanguage.h"
#include "log.h"
#include "json.h"
#include <stdarg.h>
#include <stdio.h>

lyramilk::data::multilanguage::dict lyramilk::kdict;

namespace lyramilk{namespace data{namespace multilanguage{

	struct dict_inner_keeper
	{
		bool &inner;
		dict_inner_keeper(bool &inn):inner(inn)
		{
			inner = true;
		}
		~dict_inner_keeper()
		{
			inner = false;
		}
	};

	dict::dict()
	{
		m["无法翻译%s"] = "translate \"%s\" failed.";
		p = NULL;
		inner = false;
	}

	dict::~dict()
	{
	}

	bool dict::load(lyramilk::data::string filename)
	{
		if(p && p->load(filename)) return true;
		lyramilk::data::json j;
		if(!j.open(filename)) return false;
		lyramilk::data::var v = j.path("/dict");
		m = v.as<lyramilk::data::var::map>();
		return true;
	}

	void dict::notify(lyramilk::data::string str)
	{
		if(p) {
			p->notify(str);
			return ;
		}
		/*
		if(str.compare("无法翻译\"%s\"") == 0){
			lyramilk::klog(lyramilk::log::warning,"lyramilk.multilanguage.dict") << "translate \"" << str << "\" failed." << std::endl;
		}else{
			lyramilk::klog(lyramilk::log::warning,"lyramilk.multilanguage.dict") << trans("无法翻译\"%s\"",str.c_str()) << std::endl;
		}
		*/

	}

	lyramilk::data::string dict::translate(lyramilk::data::string src)
	{
		if(inner){
			return src;
		}
		dict_inner_keeper _(inner);
		if(p) return p->translate(src);
		lyramilk::data::var::map::iterator it = m.find(src);
		if(it!=m.end()){
			return it->second;
		}

		notify(src);
		return src;
	}

	lyramilk::data::string dict::operator ()(const char* fmt,...)
	{
		char buff[256];
		lyramilk::data::string qfmt = translate(fmt);

		va_list va;
		int cnt;
		va_start(va, fmt);
		cnt = vsnprintf(buff,256, qfmt.c_str(), va);
		va_end(va);
		if(cnt < 256){
			return lyramilk::data::string(buff,cnt);
		}

		std::vector<char> buf(cnt + 1);
		va_start(va, fmt);
		vsprintf(buf.data(),qfmt.c_str(),va);
		va_end(va);
		return lyramilk::data::string(buf.begin(),buf.end());
	}

	lyramilk::data::string dict::trans(const char* fmt,...)
	{
		char buff[256];
		lyramilk::data::string qfmt = translate(fmt);

		va_list va;
		int cnt;
		va_start(va, fmt);
		cnt = vsnprintf(buff,256, qfmt.c_str(), va);
		va_end(va);
		if(cnt < 256){
			return lyramilk::data::string(buff,cnt);
		}

		std::vector<char> buf(cnt + 1);
		va_start(va, fmt);
		vsprintf(buf.data(),qfmt.c_str(),va);
		va_end(va);
		return lyramilk::data::string(buf.begin(),buf.end());
	}

	lyramilk::data::string dict::format(const char* fmt,...)
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

	dict* dict::tie(dict* pdict)
	{
		dict* old = p;
		p = pdict;
		return old;
	}

}}}
