#ifndef _lyramilk_data_var_h_
#define _lyramilk_data_var_h_

#include "config.h"
#include "def.h"
#include <stddef.h>

#include <string>
#include <list>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <exception>
#include <stdio.h>






/**
	@namespace lyramilk::data
	@brief 数据
	@details 该命名空间描述数据的表达形式。
*/

namespace lyramilk{namespace data
{

	class datawrapper;
	lyramilk::data::string iconv(const lyramilk::data::string& str,const lyramilk::data::string& from,const lyramilk::data::string& to);

	/**
		@brief 这是一个超级变量，封装了对整数、小数、字符串、数组、映射表的表达，它尽可能在各种类型间进行转换。
	*/
	class _lyramilk_api_ var
	{
	  public:

		typedef class _lyramilk_api_ std::vector<lyramilk::data::var, allocator<lyramilk::data::var> > array;
		typedef lyramilk::data::unordered_map<lyramilk::data::string,lyramilk::data::var,hash<lyramilk::data::string>, std::equal_to<lyramilk::data::string> ,lyramilk::data::allocator<std::pair<lyramilk::data::string,lyramilk::data::var> > > map;


		const static lyramilk::data::var nil;
		enum vt
		{
			t_invalid = 0x20,
			t_valid = 0x23,	///泛指所有非t_invalid类型，只能用作判断，不可作为实际的类型。
			t_any = 0x22,	///泛指所有类型，只能用作判断，不可作为实际的类型。
			t_user = 0x53,	///自定义类型
			t_bin = 0x50,	///bin类型和str的区别
			t_str = 0x30,
			t_wstr = 0x31,
			t_bool = 0x21,
			t_int = 0x46,
			t_uint = 0x47,
			t_double = 0x48,
			t_array = 0x51,
			t_map = 0x52
		};

		var();
		~var();

		var(const unsigned char* v);
		var(const char* v);
		var(const wchar_t* v);
		var(const lyramilk::data::chunk& v);
		var(const lyramilk::data::string& v);
		var(const lyramilk::data::wstring& v);
		var(bool v);
		var(int8 v);
		var(uint8 v);
		var(int16 v);
		var(uint16 v);
		var(int32 v);
		var(uint32 v);
		var(long v);
		var(unsigned long v);
		var(int64 v);
		var(uint64 v);
		var(double v);
		var(float v);
		var(const lyramilk::data::var::array& v);
		var(const lyramilk::data::var::map& v);
		var(const lyramilk::data::stringdict& v);
		var(const lyramilk::data::case_insensitive_unordered_map& v);
		var(const lyramilk::data::case_insensitive_map& v);
		var(const lyramilk::data::datawrapper& v);
		var(const lyramilk::data::var& v);

		bool operator ==(const lyramilk::data::var& v) const;
		bool operator !=(const lyramilk::data::var& v) const;
		bool operator <(const lyramilk::data::var& v) const;

/*
		template <typename T>
		bool operator <(const T& v) const
		{
			return *this < lyramilk::data::var(v);
		}
*/

		lyramilk::data::var& operator =(const lyramilk::data::var& v);

		lyramilk::data::var& at(lyramilk::data::uint64 index);
		lyramilk::data::var& at(const lyramilk::data::string& index);
		lyramilk::data::var& at(const lyramilk::data::wstring& index);

		const lyramilk::data::var& at(lyramilk::data::uint64 index) const;
		const lyramilk::data::var& at(const lyramilk::data::string& index) const;
		const lyramilk::data::var& at(const lyramilk::data::wstring& index) const;
		/**
			@brief 为var赋值
			@details 用另外一个var赋值这个var。
			@return 返回自身的引用。
		*/
		lyramilk::data::var& assign(const lyramilk::data::var& v);
		lyramilk::data::var& assign(const unsigned char* v);
		lyramilk::data::var& assign(const char* v);
		lyramilk::data::var& assign(const wchar_t* v);
		lyramilk::data::var& assign(const lyramilk::data::chunk& v);
		lyramilk::data::var& assign(const lyramilk::data::string& v);
		lyramilk::data::var& assign(const lyramilk::data::wstring& v);
		lyramilk::data::var& assign(bool v);
		lyramilk::data::var& assign(int8 v);
		lyramilk::data::var& assign(uint8 v);
		lyramilk::data::var& assign(int16 v);
		lyramilk::data::var& assign(uint16 v);
		lyramilk::data::var& assign(int32 v);
		lyramilk::data::var& assign(uint32 v);
		lyramilk::data::var& assign(long v);
		lyramilk::data::var& assign(unsigned long v);
		lyramilk::data::var& assign(int64 v);
		lyramilk::data::var& assign(uint64 v);
		lyramilk::data::var& assign(double v);
		lyramilk::data::var& assign(float v);
		lyramilk::data::var& assign(const lyramilk::data::var::array& v);
		lyramilk::data::var& assign(const lyramilk::data::var::map& v);
		lyramilk::data::var& assign(const lyramilk::data::stringdict& v);
		lyramilk::data::var& assign(const lyramilk::data::case_insensitive_unordered_map& v);
		lyramilk::data::var& assign(const lyramilk::data::case_insensitive_map& v);

		/**
			@brief 为var赋值
			@details 用一个事有标识的用户指针初始化一个var。
			@param n 用户数据的字符串标识。
			@param v 用户数据的指针。
			@return 返回自身的引用。
		*/
		lyramilk::data::var& assign(const datawrapper& v);

		operator lyramilk::data::chunk () const;
		operator lyramilk::data::string () const;
		operator lyramilk::data::wstring () const;
		operator bool () const;
		operator int8 () const;
		operator uint8 () const;
		operator int16 () const;
		operator uint16 () const;
		operator int32 () const;
		operator uint32 () const;
		operator long () const;
		operator unsigned long () const;
		operator int64 () const;
		operator uint64 () const;
		operator double () const;
		operator float () const;
		operator lyramilk::data::var::array& ();
		operator const lyramilk::data::var::array& () const;
		operator lyramilk::data::var::map& ();
		operator const lyramilk::data::var::map& () const;

		/**
			@brief 安全类型转换
			@details 安全获得某种类型，如果var无法转换到该类型。则返回if_not_compat
			@param if_not_compat 在无法转换时的默认值。
			@return 转换结果
		*/

		lyramilk::data::chunk conv(const lyramilk::data::chunk& if_not_compat) const;
		lyramilk::data::string conv(const lyramilk::data::string& if_not_compat) const;
		lyramilk::data::wstring conv(const lyramilk::data::wstring& if_not_compat) const;
		lyramilk::data::string conv(const char* if_not_compat) const;
		lyramilk::data::string conv(char* if_not_compat) const;
		lyramilk::data::wstring conv(const wchar_t* if_not_compat) const;
		lyramilk::data::wstring conv(wchar_t* if_not_compat) const;
		lyramilk::data::chunk conv(const unsigned char* if_not_compat) const;
		lyramilk::data::chunk conv(unsigned char* if_not_compat) const;

		bool conv(bool if_not_compat) const;
		uint64 conv(int8 if_not_compat) const;
		uint64 conv(uint8 if_not_compat) const;
		uint64 conv(int16 if_not_compat) const;
		uint64 conv(uint16 if_not_compat) const;
		uint64 conv(int32 if_not_compat) const;
		uint64 conv(uint32 if_not_compat) const;
		uint64 conv(int64 if_not_compat) const;
		uint64 conv(uint64 if_not_compat) const;
		uint64 conv(long if_not_compat) const;
		uint64 conv(unsigned long if_not_compat) const;
		double conv(double if_not_compat) const;

		lyramilk::data::var::array& conv(lyramilk::data::var::array& if_not_compat);
		lyramilk::data::var::map& conv(lyramilk::data::var::map& if_not_compat);
		const lyramilk::data::var::array& conv(const lyramilk::data::var::array& if_not_compat) const;
		const lyramilk::data::var::map& conv(const lyramilk::data::var::map& if_not_compat) const;
		/**
			@brief 取得用户数据的值。
			@details 取得第一个用户数据的指针。
			@return 用户数据的指针。
		*/
		datawrapper* userdata() const;
		/**
			@brief 取得var的类型。
		*/
		vt type() const;
		/**
			@brief 取得var的当前类型名称。
		*/
		lyramilk::data::string type_name() const;
		/**
			@brief 取得var的类型名称。
			@param nt 需要取得名称的var类型的枚举值。
			@return nt类型的字符串名称。
		*/
		static lyramilk::data::string type_name(vt nt);
		/**
			@brief 指定var的类型。
			@param nt 新的var类型。
			@return 返回var自身。
		*/
		lyramilk::data::var& type(vt nt);
		/**
			@brief 判断var是否可以转换成指定的var类型。
			@param nt 目标var类型。
			@return 返回是否可以转换。
		*/
		bool type_like(vt nt) const;

		lyramilk::data::var& operator[](const char* index);
		lyramilk::data::var& operator[](const wchar_t* index);
		lyramilk::data::var& operator[](const lyramilk::data::string& index);
		lyramilk::data::var& operator[](const lyramilk::data::wstring& index);
		lyramilk::data::var& operator[](uint64 index);
		lyramilk::data::var& operator[](int index);

		const lyramilk::data::var& operator[](const char* index) const;
		const lyramilk::data::var& operator[](const wchar_t* index) const;
		const lyramilk::data::var& operator[](const lyramilk::data::string& index) const;
		const lyramilk::data::var& operator[](const lyramilk::data::wstring& index) const;
		const lyramilk::data::var& operator[](uint64 index) const;
		const lyramilk::data::var& operator[](int index) const;

		/**
			@brief 以字符串返回
			@details 强制将该var以字符串形式输出。
			@return 返回字符串形式的var。
		*/
		lyramilk::data::var::array::size_type size() const;

		/**
			@brief 类型转换
			@details 将该var的类型强制转换为T，要求T必须是var所支持的类型。
			@return 将该var转换成T类型的结果。
		*/
		template <typename T>
		T as()
		{
			return *this;
		}

		/**
			@brief 以字符串返回
			@details 强制将该var以字符串形式输出。
			@return 返回字符串形式的var。
		*/
		lyramilk::data::string str() const;

		/**
			@brief 清空
			@details 将该var对象置空
		*/
		void clear();

		/**
			@brief 序列化
			@details 将该var对象序列化到os流中。
			@return 成功时返回true
		*/
		bool serialize(ostream& os) const;

		/**
			@brief 反序列化
			@details 将is流中的数据还原成var。
			@return 成功时返回true
		*/
		bool deserialize(istream& is);

		/**
			@brief 打印该var对象
			@details 将该var的信息输出到os流中。
			@param os 将该var的信息输出到os流中。
		*/
		void dump(ostream& os) const;

		/**
			@brief 通过路径访问var
			@details 因为var内部包含了数组以及映射表两种容器，因此可以表达成一个树对象。这里通过路径方位这个树。树枝必须是数组或映射表这样的容器类型，而树叶可以是var支持的任何类型。
			@param varpath 该var对象内部的路径表达式
			@return 返回容器类型var内部的一个var。
		*/
		lyramilk::data::var& path(const lyramilk::data::string& varpath);
		const lyramilk::data::var& path(const lyramilk::data::string& varpath) const;
	  private:
		vt t;

		union vu
		{
			bool b;
			int64 i8;
			uint64 u8;
			double f8;
			datawrapper* pu;

			char bp[sizeof(lyramilk::data::chunk)];
			char bs[sizeof(lyramilk::data::string)];
			char bw[sizeof(lyramilk::data::wstring)];
			char ba[sizeof(std::vector<int>)];
			char bm[sizeof(lyramilk::data::unordered_map<lyramilk::data::string,int,hash<lyramilk::data::string>, std::equal_to<lyramilk::data::string> ,lyramilk::data::allocator<std::pair<lyramilk::data::string,int> > >)];
		}u;
		bool _serialize(ostream& os) const;
		bool _deserialize(istream& is);
	};


	typedef lyramilk::data::var::array array;
	typedef lyramilk::data::var::map map;

	template < >
	lyramilk::data::chunk& lyramilk::data::var::as<lyramilk::data::chunk&>();

	template < >
	lyramilk::data::string& lyramilk::data::var::as<lyramilk::data::string&>();

	template < >
	lyramilk::data::wstring& lyramilk::data::var::as<lyramilk::data::wstring&>();
}}

_lyramilk_api_ std::ostream& operator << (std::ostream& os, const lyramilk::data::var& t);
_lyramilk_api_ std::istream& operator >> (std::istream& is, lyramilk::data::var& t);

_lyramilk_api_ std::ostream& operator << (std::ostream& os, const lyramilk::data::map& t);
_lyramilk_api_ std::ostream& operator << (std::ostream& os, const lyramilk::data::array& t);

#endif
