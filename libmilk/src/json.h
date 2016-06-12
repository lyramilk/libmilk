#ifndef _lyramilk_data_json_h_
#define _lyramilk_data_json_h_
#include <vector>

#include "var.h"

namespace lyramilk{ namespace data
{
	/*
		@brief json对象
		@details 用来操作一个json对象。
	*/
	class _lyramilk_api_ json
	{
		var v;
	  public:
		/**
			@brief 构造函数
		*/
		json();
		/**
			@brief 构造函数
			@details 构造并open一个文件以读取或修改json文件。
		*/
		json(string filename);
		/**
			@brief 析构函数
		*/
		virtual ~json();

		/**
			@brief 打开一个json文件
			@details 打开一个json文件。
			@param filename json文件路径。
			@return 为true时打开文件成功。
		*/
		bool open(string filename);

		/**
			@brief 保存一个json文件
			@details 保存一个json文件。
			@param filename json文件路径。
			@return 为true时保存文件成功。
		*/
		bool save(string filename);

		/**
			@brief 从一个字符串中获取json对象。
			@param buffer 一个json的字符串表达式。
			@return 为true时表示获取成功。
		*/
		bool from_str(string buffer);

		/**
			@brief 用该对象生成一个json字符串表达式。
			@return 用该对象生成的字符串表达式。
		*/
		string to_str();

		/**
			@brief 清空该json对象
		*/
		void clear();

		/**
			@brief 通过路径操作该json对象。
			@return 返回一个可读可写的var对象。
		*/
		var& path(string path) throw(var::type_invalid);

		/**
			@brief 获取该json对象的根var对象。
			@return 该json对象的根var对象。
		*/
		var& get_var();
	};
}}
#endif
