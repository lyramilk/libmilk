﻿#ifndef _lyramilk_data_var_h_
#define _lyramilk_data_var_h_

#include "config.h"
#include <stddef.h>

#include <string>
#include <list>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <exception>
#include <stdio.h>


#ifdef Z_HAVE_UNORDEREDMAP
	#include <unordered_map>
#elif defined Z_HAVE_TR1_UNORDEREDMAP
	#include <tr1/unordered_map>
#endif

/**
	@namespace lyramilk::data
	@brief 数据
	@details 该命名空间描述数据的表达形式。
*/

namespace lyramilk{namespace data
{
	using std::vector;
	using std::map;
	using std::pair;
	using std::multimap;
#ifdef Z_HAVE_UNORDEREDMAP
	using std::unordered_map;
	using std::hash;
#elif defined Z_HAVE_TR1_UNORDEREDMAP
	using std::tr1::unordered_map;
	using std::tr1::hash;
#endif
	using std::list;

	typedef char int8;
	typedef unsigned char uint8;

	typedef short int16;
	typedef unsigned short uint16;

	typedef int int32;
	typedef unsigned int uint32;

	typedef long long int64;
	typedef unsigned long long uint64;


	_lyramilk_api_ void* milk_malloc(size_t size);
	_lyramilk_api_ void milk_free(void* p, size_t size);

#ifdef USEMILKALLOC
	/**
		@brief 这是一个支持标准库的内存分配器，可用于标准库中各种容器对象的安全跨库使用。
	*/
	template <typename T>
	class allocator
	{
	  public:
		typedef size_t		size_type;
		typedef ptrdiff_t	difference_type;
		typedef T*			pointer;
		typedef const T*	const_pointer;
		typedef T&			reference;
		typedef const T&	const_reference;
		typedef T			value_type;

		template<typename U>
		struct rebind {
			typedef allocator<U> other;
		};
		allocator() throw()
		{}
		allocator(const allocator&) throw()
		{}
		template<typename U>
		allocator(const allocator<U>&) throw()
		{}
		~allocator() throw()
		{}

		pointer address(reference __x) const { return &__x; }
		const_pointer address(const_reference __x) const { return &__x; }

		pointer allocate(size_type __n, const void* = 0)
		{
			return static_cast<T*>(lyramilk::data::milk_malloc(__n * sizeof(T)));
		}
		void deallocate(pointer __p, size_type __n)
		{
			lyramilk::data::milk_free(__p,__n * sizeof(T));
		}
		size_type max_size() const throw()
		{
			return size_t(-1) / sizeof(T);
		}

		void construct(pointer __p, const T& __val)
		{
			::new((void *)__p) T(__val);
		}

#if __cplusplus >= 201103L
		template<typename... _Args> void construct(pointer __p, _Args&&... __args)
		{
			::new((void *)__p) T(__args...);
		}
#endif
		void destroy(pointer __p)
		{
#ifdef _MSC_VER
			__p;
#endif
			__p->~T();
		}
	};

	template <typename T> inline bool operator==(const allocator<T>&, const allocator<T>&) { return true; }
	template <typename T> inline bool operator!=(const allocator<T>&, const allocator<T>&) { return false; }

	template <typename T> inline bool operator==(const allocator<T>&, const std::allocator<T>&) { return true; }
	template <typename T> inline bool operator!=(const allocator<T>&, const std::allocator<T>&) { return false; }
#else
	using std::allocator;
#endif


	typedef class _lyramilk_api_ std::basic_string<char, std::char_traits<char>, allocator<char> > string;
	typedef class _lyramilk_api_ std::basic_string<unsigned char, std::char_traits<unsigned char>, allocator<unsigned char> > chunk;
	typedef class _lyramilk_api_ std::basic_string<wchar_t, std::char_traits<wchar_t>, allocator<wchar_t> > wstring;

	typedef class _lyramilk_api_ std::basic_stringstream<char, std::char_traits<char>, allocator<char> > stringstream;
	typedef class _lyramilk_api_ std::basic_stringstream<unsigned char, std::char_traits<unsigned char>, allocator<unsigned char> > datastream;
	typedef class _lyramilk_api_ std::basic_stringstream<wchar_t, std::char_traits<wchar_t>, allocator<wchar_t> > wstringstream;

	typedef class _lyramilk_api_ std::basic_istringstream<char, std::char_traits<char>, allocator<char> > istringstream;
	typedef class _lyramilk_api_ std::basic_istringstream<unsigned char, std::char_traits<unsigned char>, allocator<unsigned char> > idatastream;
	typedef class _lyramilk_api_ std::basic_istringstream<wchar_t, std::char_traits<wchar_t>, allocator<wchar_t> > wistringstream;

	typedef class _lyramilk_api_ std::basic_ostringstream<char, std::char_traits<char>, allocator<char> > ostringstream;
	typedef class _lyramilk_api_ std::basic_ostringstream<unsigned char, std::char_traits<unsigned char>, allocator<unsigned char> > odatastream;
	typedef class _lyramilk_api_ std::basic_ostringstream<wchar_t, std::char_traits<wchar_t>, allocator<wchar_t> > wostringstream;

	typedef class _lyramilk_api_ std::basic_iostream<char> stream;
	typedef class _lyramilk_api_ std::basic_iostream<unsigned char> bstream;
	typedef class _lyramilk_api_ std::basic_iostream<wchar_t> wstream;

	typedef class _lyramilk_api_ std::basic_istream<char> istream;
	typedef class _lyramilk_api_ std::basic_istream<unsigned char> bistream;
	typedef class _lyramilk_api_ std::basic_istream<wchar_t> wistream;

	typedef class _lyramilk_api_ std::basic_ostream<char> ostream;
	typedef class _lyramilk_api_ std::basic_ostream<unsigned char> bostream;
	typedef class _lyramilk_api_ std::basic_ostream<wchar_t> wostream;


	typedef class _lyramilk_api_ std::vector<string,allocator<string> > strings;



#ifdef USEMILKALLOC
	std::string inline str(lyramilk::data::string str)
	{
		return std::string(str.c_str(),str.size());
	}

	lyramilk::data::string inline str(std::string str)
	{
		return lyramilk::data::string(str.c_str(),str.size());
	}
#else
	std::string inline str(lyramilk::data::string str)
	{
		return str;
	}
#endif
	lyramilk::data::string inline str(unsigned long long i)
	{
		char buff[64];
		snprintf(buff,sizeof(buff),"%llu",i);
		return buff;
	}
#ifdef Z_HAVE_TR1_UNORDEREDMAP
		typedef lyramilk::data::unordered_map<string,string> stringdict;
#else
		typedef lyramilk::data::map<string,string> stringdict;
#endif
#ifdef Z_HAVE_TR1_UNORDEREDMAP
		typedef lyramilk::data::unordered_map<wstring,wstring> wstringdict;
#else
		typedef lyramilk::data::map<wstring,wstring> wstringdict;
#endif
	/**
		@brief 这是一个超级变量，封装了对整数、小数、字符串、数组、映射表的表达，它尽可能在各种类型间进行转换。
	*/
	class _lyramilk_api_ var
	{
#ifdef Z_HAVE_TR1_UNORDEREDMAP
		typedef lyramilk::data::unordered_map<string,const void*> _userdata;
#else
		typedef lyramilk::data::map<string,const void*> _userdata;
#endif
	  public:
		typedef class _lyramilk_api_ std::vector<lyramilk::data::var, allocator<lyramilk::data::var> > array;
#ifdef Z_HAVE_TR1_UNORDEREDMAP
		typedef lyramilk::data::unordered_map<lyramilk::data::string,lyramilk::data::var,hash<lyramilk::data::string>, std::equal_to<lyramilk::data::string> ,lyramilk::data::allocator<std::pair<lyramilk::data::string,lyramilk::data::var> > > map;
#else
		typedef class _lyramilk_api_ std::map<lyramilk::data::string, lyramilk::data::var, std::less<lyramilk::data::string>, allocator<lyramilk::data::string> > map;
#endif
		class type_invalid:public std::exception
		{
			string p;
		  public:
			type_invalid(string msg);
			virtual ~type_invalid() throw();
			virtual const char* what() const throw();
		};

		const static var nil;
		enum vt
		{
			t_invalid = 0x20,
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
		var(const chunk& v);
		var(const string& v);
		var(const wstring& v);
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
		var(const array& v);
		var(const map& v);
		var(const stringdict& v);
		var(const string& n,const void* v);
		var(const var& v);

		var operator +(const var& v) const throw(type_invalid);
		var& operator +=(const var& v) throw(type_invalid);
		bool operator ==(const var& v) const throw(type_invalid);
		bool operator !=(const var& v) const throw(type_invalid);
		bool operator <(const var& v) const throw(type_invalid);
		bool operator >(const var& v) const throw(type_invalid);

		template <typename T>
		var operator +(const T& v) const
		{
			return var(*this) += var(v);
		}
		
		template <typename T>
		var& operator +=(const T& v)
		{
			return *this += var(v);
		}

		template <typename T>
		bool operator ==(const T& v) const
		{
			return *this == var(v);
		}

		template <typename T>
		bool operator !=(const T& v) const
		{
			return *this != var(v);
		}

		template <typename T>
		bool operator <(const T& v) const
		{
			return *this < var(v);
		}

		template <typename T>
		bool operator >(const T& v) const
		{
			return *this > var(v);
		}

		template <typename T>
		bool operator <=(const T& v) const
		{
			return !(*this > var(v));
		}

		template <typename T>
		bool operator >=(const T& v) const
		{
			return !(*this < var(v));
		}

		var& operator =(const var& v);

		var& at(lyramilk::data::uint64 index) throw(type_invalid);
		var& at(const string& index) throw(type_invalid);
		var& at(const wstring& index) throw(type_invalid);

		const var& at(lyramilk::data::uint64 index) const throw(type_invalid);
		const var& at(const string& index) const throw(type_invalid);
		const var& at(const wstring& index) const throw(type_invalid);
		/**
			@brief 为var赋值
			@details 用另外一个var赋值这个var。
			@return 返回自身的引用。
		*/
		var& assign(const var& v);
		var& assign(const unsigned char* v);
		var& assign(const char* v);
		var& assign(const wchar_t* v);
		var& assign(const chunk& v);
		var& assign(const string& v);
		var& assign(const wstring& v);
		var& assign(bool v);
		var& assign(int8 v);
		var& assign(uint8 v);
		var& assign(int16 v);
		var& assign(uint16 v);
		var& assign(int32 v);
		var& assign(uint32 v);
		var& assign(long v);
		var& assign(unsigned long v);
		var& assign(int64 v);
		var& assign(uint64 v);
		var& assign(double v);
		var& assign(float v);
		var& assign(const array& v);
		var& assign(const map& v);
		var& assign(const stringdict& v);
		/**
			@brief 为var赋值
			@details 用一个事有标识的用户指针初始化一个var。
			@param n 用户数据的字符串标识。
			@param v 用户数据的指针。
			@return 返回自身的引用。
		*/
		var& assign(const string& n,const void* v);

		operator chunk () const throw(type_invalid);
		operator string () const throw(type_invalid);
		operator wstring () const throw(type_invalid);
		operator bool () const throw(type_invalid);
		operator int8 () const throw(type_invalid);
		operator uint8 () const throw(type_invalid);
		operator int16 () const throw(type_invalid);
		operator uint16 () const throw(type_invalid);
		operator int32 () const throw(type_invalid);
		operator uint32 () const throw(type_invalid);
		operator long () const throw(type_invalid);
		operator unsigned long () const throw(type_invalid);
		operator int64 () const throw(type_invalid);
		operator uint64 () const throw(type_invalid);
		operator double () const throw(type_invalid);
		operator float () const throw(type_invalid);
		operator array& () throw(type_invalid);
		operator const array& () const throw(type_invalid);
		operator map& () throw(type_invalid);
		operator const map& () const throw(type_invalid);

		/**
			@brief 安全类型转换
			@details 安全获得某种类型，如果var无法转换到该类型。则返回if_not_compat
			@param if_not_compat 在无法转换时的默认值。
			@return 转换结果
		*/

		chunk conv(chunk if_not_compat) const;
		string conv(string if_not_compat) const;
		wstring conv(wstring if_not_compat) const;
		string conv(const char* if_not_compat) const;
		string conv(char* if_not_compat) const;
		wstring conv(const wchar_t* if_not_compat) const;
		wstring conv(wchar_t* if_not_compat) const;
		chunk conv(const unsigned char* if_not_compat) const;
		chunk conv(unsigned char* if_not_compat) const;

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
		array& conv(array& if_not_compat);
		map& conv(map& if_not_compat);
		const array& conv(const array& if_not_compat) const;
		const map& conv(const map& if_not_compat) const;

		/**
			@brief 定义额外的用户数据。
			@param v 用户数据的标识。
			@param p 用户数据的指针。
		*/
		void userdata(string v,const void* p) throw(type_invalid);
		/**
			@brief 取得用户数据的值。
			@details 根据用户数据的标识获取用户数据的指针。
			@param v 用户数据的标识。
			@return 用户数据的指针。
		*/
		const void* userdata(string v) const;
		/**
			@brief 取得用户数据的值。
			@details 取得第一个用户数据的指针。
			@return 用户数据的指针。
		*/
		const void* userdata() const;
		/**
			@brief 取得var的类型。
		*/
		vt type() const;
		/**
			@brief 取得var的当前类型名称。
		*/
		string type_name() const;
		/**
			@brief 取得var的类型名称。
			@param nt 需要取得名称的var类型的枚举值。
			@return nt类型的字符串名称。
		*/
		static string type_name(vt nt);
		/**
			@brief 指定var的类型。
			@param nt 新的var类型。
			@return 返回var自身。
		*/
		var& type(vt nt) throw(type_invalid);
		/**
			@brief 判断var是否可以转换成指定的var类型。
			@param nt 目标var类型。
			@return 返回是否可以转换。
		*/
		bool type_like(vt nt) const;

		var& operator[](const char* index) throw(type_invalid);
		var& operator[](const wchar_t* index) throw(type_invalid);
		var& operator[](const string& index) throw(type_invalid);
		var& operator[](const wstring& index) throw(type_invalid);
		var& operator[](bool index) throw(type_invalid);
		var& operator[](int8 index) throw(type_invalid);
		var& operator[](uint8 index) throw(type_invalid);
		var& operator[](int16 index) throw(type_invalid);
		var& operator[](uint16 index) throw(type_invalid);
		var& operator[](int32 index) throw(type_invalid);
		var& operator[](uint32 index) throw(type_invalid);
		var& operator[](long index) throw(type_invalid);
		var& operator[](int64 index) throw(type_invalid);
		var& operator[](uint64 index) throw(type_invalid);
		var& operator[](double index) throw(type_invalid);

		const var& operator[](const char* index) const throw(type_invalid);
		const var& operator[](const wchar_t* index) const throw(type_invalid);
		const var& operator[](const string& index) const throw(type_invalid);
		const var& operator[](const wstring& index) const throw(type_invalid);
		const var& operator[](bool index) const throw(type_invalid);
		const var& operator[](int8 index) const throw(type_invalid);
		const var& operator[](uint8 index) const throw(type_invalid);
		const var& operator[](int16 index) const throw(type_invalid);
		const var& operator[](uint16 index) const throw(type_invalid);
		const var& operator[](int32 index) const throw(type_invalid);
		const var& operator[](uint32 index) const throw(type_invalid);
		const var& operator[](long index) const throw(type_invalid);
		const var& operator[](int64 index) const throw(type_invalid);
		const var& operator[](uint64 index) const throw(type_invalid);
		const var& operator[](double index) const throw(type_invalid);

		/**
			@brief 以字符串返回
			@details 强制将该var以字符串形式输出。
			@return 返回字符串形式的var。
		*/
		array::size_type size() const throw(type_invalid);

		/**
			@brief 类型转换
			@details 将该var的类型强制转换为T，要求T必须是var所支持的类型。
			@return 将该var转换成T类型的结果。
		*/
		template <typename T>
		T as() throw(type_invalid)
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
		bool serialize(ostream& os) const throw(type_invalid);

		/**
			@brief 序列化
			@details 将该var对象序列化到os流中。
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
		var& path(string varpath) throw(type_invalid);
		const var& path(string varpath) const throw(type_invalid);
	  private:
		vt t;

		union vu
		{
			bool b;
			int64 i8;
			uint64 u8;
			double f8;

			char bp[sizeof(chunk) + 10];
			char bs[sizeof(string) + 10];
			char bw[sizeof(wstring) + 10];
			char ba[sizeof(std::vector<int>) + 10];
			char bm[sizeof(_userdata) + 10];
			char bo[sizeof(_userdata) + 10];
		}u;
		bool _serialize(ostream& os) const throw(type_invalid);
		bool _deserialize(istream& is);

		static _userdata __reserve0;
	};

	template < >
	lyramilk::data::chunk& lyramilk::data::var::as<lyramilk::data::chunk&>() throw(type_invalid);

	template < >
	lyramilk::data::string& lyramilk::data::var::as<lyramilk::data::string&>() throw(type_invalid);

	template < >
	lyramilk::data::wstring& lyramilk::data::var::as<lyramilk::data::wstring&>() throw(type_invalid);
}}

#ifdef Z_HAVE_UNORDEREDMAP
namespace std{
#elif defined Z_HAVE_TR1_UNORDEREDMAP
namespace std{namespace tr1{
#endif

#if (defined Z_HAVE_UNORDEREDMAP) || (defined Z_HAVE_TR1_UNORDEREDMAP)
	template <>
	size_t hash<lyramilk::data::string>::operator()(lyramilk::data::string) const;
#endif
#ifdef Z_HAVE_UNORDEREDMAP
}
#elif defined Z_HAVE_TR1_UNORDEREDMAP
}}
#endif

_lyramilk_api_ std::ostream& operator << (std::ostream& os, const lyramilk::data::var& t);
_lyramilk_api_ std::istream& operator >> (std::istream& is, lyramilk::data::var& t);

_lyramilk_api_ lyramilk::data::bostream& operator << (lyramilk::data::bostream& os, char c);
_lyramilk_api_ lyramilk::data::bostream& operator << (lyramilk::data::bostream& os, signed char c);
_lyramilk_api_ lyramilk::data::bostream& operator << (lyramilk::data::bostream& os, const char* c);
_lyramilk_api_ lyramilk::data::bostream& operator << (lyramilk::data::bostream& os, const signed char* c);

#endif
