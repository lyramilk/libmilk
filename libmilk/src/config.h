﻿#ifndef _lyramilk_config_h_
	#ifdef WIN32
		#ifndef _lyramilk_api_
			#ifdef milk_EXPORTS
				#define _lyramilk_api_ __declspec(dllexport) 
			#else
				#define _lyramilk_api_ __declspec(dllimport) 
			#endif
		#endif
	#else
		#define _lyramilk_api_
	#endif


	#ifdef __GNUC__
		#define THIS_FUNCTION_IS_DEPRECATED(func) func __attribute__ ((deprecated))
	#elif defined(_MSC_VER)
		#define THIS_FUNCTION_IS_DEPRECATED(func) __declspec(deprecated) func
	#else
		/**
			@brief 描述过时函数
			@details 这个宏将某函数标记为过时函数，以建议用户避免使用该函数。
		*/
		#define THIS_FUNCTION_IS_DEPRECATED(func) func
	#endif

	#ifdef _MSC_VER
	#pragma warning( disable : 4290 )
	#endif

	#if __cplusplus < 201103L
		//not c++11
		#ifndef nullptr
			#define nullptr NULL
		#endif
	#endif
	#define TODO()		std::cout << __FILE__ << ":" << __LINE__ << "函数" << __FUNCTION__ << "未实现" <<std::endl; \
				throw __FUNCTION__;
#endif