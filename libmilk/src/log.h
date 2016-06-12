﻿#ifndef _lyramilk_log_h_
#define _lyramilk_log_h_

#include "var.h"
#include "thread.h"
#include <sstream>

/**
	@namespace lyramilk::log
	@brief 日志组件命名空间
	@details 该命名空间提供支持控制台彩色打印并适用性良好的日志基类logb和将logb封装到C++流中的日志对象logss
*/
namespace lyramilk { namespace log
{
	/**
		@brief 日志类型
	*/
	enum type{
		/// 调试日志，发布版本可能会忽略这种日志。
		debug,
		/// 跟踪日志，通过该种类日志打印正常的运行信息。
		trace,
		/// 调试日志，程序运行参数不准确，程序运行不会受到影响，但输出可能不精确。
		warning,
		/// 错误日志，程序运行参数不正确，程序运行可能会受到影响，输出不准确或有遗漏。
		error
	};
	/**
		@brief 基本日志
		@details 提供基本日志功能。这个模块将日志输出到控制台。
	*/
	class _lyramilk_api_ logb
	{
	  protected:
		/**
			@brief 格式化时间
			@details 将时间格式化后输出到日志中。
			@param ti 时间整数，从1970年1月1号到现在的秒数。
		*/
		virtual lyramilk::data::string strtime(time_t ti);
	  public:
		/**
			@brief 记录日志。
			@details 提供基本日志功能。这个模块将日志输出到控制台。
			@param ti 时间，内部会调用strtime被格式化成字符串。
			@param ty 类型，调试信息的类别，例如trace。
			@param usr 当前用户。
			@param app 当前进程名。
			@param module 模块名。
			@param str 日志消息。

		*/
		virtual void log(time_t ti,type ty,lyramilk::data::string usr,lyramilk::data::string app,lyramilk::data::string module,lyramilk::data::string str);
		/**
			@brief 构造函数
		*/
		logb();
		~logb();
	};

	class _lyramilk_api_ logss;
#ifdef WIN32
	template class _lyramilk_api_ lyramilk::data::vector<char, lyramilk::data::allocator<char> >;
#endif
	/**
		@brief 日志流的缓冲
	*/
	class _lyramilk_api_ logbuf : public std::basic_streambuf<char>
	{
		lyramilk::data::vector<char,lyramilk::data::allocator<char> > buf;
		logss& p;
		int r;
	  public:
		/**
			@brief 构造函数，通过一个日志流来构造流缓冲。
			@param pp 日志流。
		*/
		logbuf(logss& pp);
		/**
			@brief 析构函数
		*/
		virtual ~logbuf();
		virtual std::streamsize sputn (const char_type* s, std::streamsize  n);
		virtual int_type sputc (char_type c);
		virtual std::streamsize xsputn (const char_type* s, std::streamsize  n);
		/**
			@brief 继承于template std::basic_streambuf，当写入缓冲发生溢出时触发。
			@param _Meta 缓冲区溢出时正在写入的字符，这个字符尚未写入到缓冲区中，因此清理完缓冲区后需要将它写到缓冲区里。
		*/
		virtual int_type overflow(int_type _Meta);
		/**
			@brief 继承于template std::basic_streambuf，同步缓冲区时发生，对于日志流来说，同步就是将日志写入到日志系统中。默认的实现会写入到控制台。
		*/
		virtual int sync();
	};

	/**
		@brief 日志流
		@details 提供符合C++标准流的日志承载功能，默认输出到控制台，可以通过tie绑定一定输出到其它位置的logb实例。
	*/
	class _lyramilk_api_ logss : public lyramilk::data::ostringstream
	{
		type t;
		logb loger;
		logb* p;
		lyramilk::data::string module;
		lyramilk::data::string module_suffix;
		lyramilk::system::threading::mutex_os lock;
		logbuf db;
		friend class logbuf;
	  public:
		/**
			@brief 构造函数。
		*/
		logss();
		/**
			@brief 构造函数，通过给定一个模块名来构造。
			@param m 这个流所服务的模块。
		*/
		logss(lyramilk::data::string m);
		/**
			@brief 构造函数，通过给定一个默认流来构造。
			@param qlog 默认的流。
			@param m 这个流所服务的模块。
		*/
		logss(const logss& qlog,lyramilk::data::string m);
		virtual ~logss();
		/**
			@brief 模拟一个函数。通过这个函数来设置日志类型。
			@details 例如
			@verbatim
				输出一条日志logss.(lyramilk::log::warning)  << "这是一条调试信息" << std::endl;
				将会输出黄色 [基准模块]   [时间] 这是一条调试信息
			@endverbatim
			@param ty 日志类型
			@return 返回日志流自身以方便 << 运算符表现。
		*/
		logss& operator()(type ty);
		/**
			@brief 模拟一个函数。通过这个函数来设置子模块。
			@details 例如
			@verbatim
				输出一条日志logss.("成员函数")  << "这是一条调试信息" << std::endl;
				将会输出 [基准模块.成员函数]   [时间] 这是一条调试信息
			@endverbatim
			@param m 子模块名称
			@return 返回日志流自身以方便 << 运算符表现。
		*/
		logss& operator()(lyramilk::data::string m);
		/**
			@brief 模拟一个函数。通过这个函数来设置日志类型和子模块。
			@details 例如
			@verbatim
				输出一条日志logss.(lyramilk::log::warning,"成员函数")  << "这是一条调试信息" << std::endl;
				将会输出黄色 [基准模块.成员函数]   [时间] 这是一条调试信息
			@endverbatim
			@param m 子模块名称
			@param ty 日志类型
			@return 返回日志流自身以方便 << 运算符表现。
		*/
		logss& operator()(type ty,lyramilk::data::string m);
		/**
			@brief 将日志输出到指定的logb实现中。
			@details 将该日志流定向到指定的ploger中，可以改变日志的表现形式以及存储方式。
			@param ploger 自定义的logb，在里面可以改变日志的现实以存储动作。
			@return 旧的logb。
		*/
		logb* rebase(logb* ploger);
	};
}}


namespace lyramilk{
	/// 全局默认日志流，所有的日志最好通过这个流来初始化。
	extern  _lyramilk_api_ lyramilk::log::logss klog;
}

#ifdef _MSC_VER
	#define D(...) lyramilk::kdict(__VA_ARGS__)
#elif defined __GNUC__
	#define D(x...) lyramilk::kdict(x)
#else
	#define D(x)	
#endif

#endif
