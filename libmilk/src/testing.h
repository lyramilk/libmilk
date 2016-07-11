﻿#ifndef _lyramilk_cpu
#define _lyramilk_cpu

#include <exception>
#include "var.h"
#include <sys/sysinfo.h>

namespace lyramilk{ namespace debug
{
	class _lyramilk_api_ cpuinfo
	{
	  public:
		static void bind(int cpucore);
		static int count();
	};

	class timediff
	{
	  public:
		virtual void mark() = 0;
		virtual long long diff() = 0;
	};

	class _lyramilk_api_ nsecdiff : public timediff
	{
		/// 纳秒时间戳
		struct timespec timestamp;
	  public:
		void mark();
		long long diff();
	};
	/**
		@breif 时间间隔计算。这里的时间数都是以CPU时钟周期为计算单位。
	*/
	class _lyramilk_api_ clockdiff : public timediff
	{
		/// 时钟周期数时间戳
		long long timestamp;
	  public:
		/**
			@brief 构造函数，构造函数在调用的时候会初始化调用差和时间戳
		*/
		clockdiff();

		/**
			@brief 标记时间戳。
		*/
		void mark();

		/**
			@brief 计算上一次标记时间戳到现在为止经过的CPU时钟周期数。
		*/
		long long diff();
	};

	/**
		@brief 测试从构造到析构所用掉的cpu时钟周期数。
	*/
	class _lyramilk_api_ clocktester
	{
		timediff& td;
		lyramilk::data::string str;
		std::ostream& outer;
		bool printable;
	  public:
		/**
			@brief 默认的构造函数，输出信息将打印在std::cout中。
		*/
		clocktester(timediff& td);
		/**
			@brief 使用一个字符串值构造，该字符串作为打印时的模板。输出信息将打印在std::cout中。
			@param msg 该字符串中需要包含%lld以便获取CPU时钟周期数的数字，该字符串会被翻译。
		*/
		clocktester(timediff& td,lyramilk::data::string msg);
		/**
			@brief 使用一个字符串值构造，该字符串作为打印时的模板。
			@param os stl输出流，支持std::cout和lyramilk::log::logss。
			@param msg 该字符串中需要包含%lld以便获取CPU时钟周期数的数字，该字符串会被翻译。
		*/
		clocktester(timediff& td,std::ostream& os,lyramilk::data::string msg);
		/**
			@brief 在析构的同时打印信息。
		*/
		~clocktester();
		/**
			@brief 取消自动打印。
		*/
		void cancel();
		/**
			@brief 恢复自动打印。
		*/
		void resume();
		/**
			@brief 重新设置消息字符串。
		*/
		void setmsg(lyramilk::data::string msg);
	};

	/**
		@brief 计时器
		@details 开始或刚刚检查状态后，间隔一段时间状态会再次变为true。
	*/
	class _lyramilk_api_ timer
	{
		time_t v;
		volatile time_t l;
	  public:
		timer();
		~timer();
		operator bool();
	};

#ifdef _DEBUG
	/**
		@breif 宏，用来在_DEBUG模式下显示当前代码的执行所消耗的CPU时钟周期数。
	*/
	#define TT(x) lyramilk::debug::clocktester(#x " 耗时：%lld"),x
#else
	#define TT(x) x
#endif
}}
#endif

