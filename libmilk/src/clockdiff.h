#ifndef _lyramilk_debug_clockdiff_
#define _lyramilk_debug_clockdiff_

#include <exception>
#include "var.h"


namespace lyramilk{ namespace debug
{

	/**
		@breif 时间间隔计算。这里的时间数都是以CPU时钟周期为计算单位。
	*/
	class _lyramilk_api_ clockdiff
	{
		/// 时间戳
		long long timestamp;
		/// 时间差，该差值反应了调用一次mark和调用一次diff在调用过程中消耗的CPU时钟周期数。
		long long d;
	  public:

		/**
			@brief 构造函数，构造函数在调用的时候会初始化调用差和时间戳
		*/
		clockdiff();

		/**
			@brief 测试函数，在计算时间差值之前用来测试在当前模块里调用mark和diff上的消耗。
		*/
		inline void test()
		{
			mark();
			diff();
			long long d1,d2;
			d = d1 = d2 = 0;
			mark();
			d1 = diff();
			mark();
			d2 = diff();
			d = d1 > d2 ? d2 : d1;
		}

		/**
			@brief 标记时间戳。
		*/
		void mark();

		/**
			@brief 计算上一次标记时间戳到现在为止经过的CPU时钟周期数。
		*/
		long long diff();

		/**
			@brief 计算上一次标记时间戳到现在为止经过的CPU时钟周期数。并将结果输出返回成字符串形式。
		*/
		lyramilk::data::string diff_str();
	};

	/**
		@brief 测试从构造到析构所用掉的cpu时钟周期数。
	*/
	class _lyramilk_api_ clocktester
	{
		/// 时间戳
		long long timestamp;
		/// 时间差，该差值反应了调用一次mark和调用一次diff在调用过程中消耗的CPU时钟周期数。
		static long long d;
		lyramilk::data::string str;
		std::ostream& outer;
		bool printable;
	  public:
		/**
			@brief 默认的构造函数，输出信息将打印在std::cout中。
		*/
		clocktester();
		/**
			@brief 使用一个字符串值构造，该字符串作为打印时的模板。输出信息将打印在std::cout中。
			@param msg 该字符串中需要包含%lld以便获取CPU时钟周期数的数字，该字符串会被翻译。
		*/
		clocktester(lyramilk::data::string msg);
		/**
			@brief 使用一个字符串值构造，该字符串作为打印时的模板。
			@param os stl输出流，支持std::cout和lyramilk::log::logss。
			@param msg 该字符串中需要包含%lld以便获取CPU时钟周期数的数字，该字符串会被翻译。
		*/
		clocktester(std::ostream& os,lyramilk::data::string msg);
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

