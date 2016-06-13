#ifndef _lyramilk_data_iterator_h_
#define _lyramilk_data_iterator_h_

#include "config.h"
#include <map>
#include <iostream>

namespace lyramilk{ namespace data{

	template <typename C,typename T>
	class output_iterator:public std::iterator<std::output_iterator_tag,T,int>
	{
	  public:
		virtual ~output_iterator()
		{}

		output_iterator()
		{}

		output_iterator(const C &o)
		{
			assign(o);
		}

		C& operator =(const C &o)
		{
			return assign(o);
		}

		bool operator ==(const C &o)
		{
			return equal(o);
		}

		bool operator !=(const C &o)
		{
			return !equal(o);
		}

		C& operator ++()
		{
			tonext();
			return *(C*)this;
		}

		C operator ++(int)
		{
			C c = *(C*)this;
			++*(C*)this;
			return c;
		}
		
		T& operator*()
		{
			return *get();
		}

		T* operator->()
		{
			return get();
		}
	  protected:
		/// 获取当前数据的指针
		virtual T* get() = 0;
		/// 数据指针指向下一个迭代器
		virtual void tonext() = 0;
		/// 比较两个迭代器是否相等
		virtual bool equal(const C& c) const = 0;
		/// 以新的迭代器重置当前迭代器
		virtual C& assign(const C &o) = 0;
		/// 返回一个空的迭代器
		virtual C& eof() const = 0;
	};

	template <typename C,typename T>
	class input_iterator:public std::iterator<std::output_iterator_tag,T,int>
	{
	  public:
		virtual ~input_iterator()
		{}

		input_iterator()
		{}

		input_iterator(const C &o)
		{
			assign(o);
		}

		C& operator =(const C &o)
		{
			return assign(o);
		}

		bool operator ==(const C &o)
		{
			return equal(o);
		}

		bool operator !=(const C &o)
		{
			return !equal(o);
		}

		C& operator ++()
		{
			tonext();
			return *(C*)this;
		}

		C operator ++(int)
		{
			C c = *(C*)this;
			++*(C*)this;
			return c;
		}

		T& operator*()
		{
			return *get();
		}

		T* operator->()
		{
			return get();
		}
	  protected:
		/// 获取当前数据的指针
		virtual T* get() = 0;
		/// 数据指针指向下一个迭代器
		virtual void tonext() = 0;
		/// 比较两个迭代器是否相等
		virtual bool equal(const C& c) const = 0;
		/// 以新的迭代器重置当前迭代器
		virtual C& assign(const C &o) = 0;
		/// 返回一个空的迭代器
		virtual C& eof() const = 0;
	};

	template <typename C,typename T>
	class foward_iterator:public std::iterator<std::forward_iterator_tag,T,int>
	{
	  public:
		virtual ~foward_iterator()
		{}

		foward_iterator()
		{}

		foward_iterator(const C &o)
		{
			assign(o);
		}

		C& operator =(const C &o)
		{
			return assign(o);
		}

		bool operator ==(const C &o)
		{
			return equal(o);
		}

		bool operator !=(const C &o)
		{
			return !equal(o);
		}

		C& operator ++()
		{
			tonext();
			return *(C*)this;
		}

		C operator ++(int)
		{
			C c = *(C*)this;
			++*(C*)this;
			return c;
		}

		T& operator*()
		{
			return *get();
		}

		T* operator->()
		{
			return get();
		}
	  protected:
		/// 获取当前数据的指针
		virtual T* get() = 0;
		/// 数据指针指向下一个迭代器
		virtual void tonext() = 0;
		/// 比较两个迭代器是否相等
		virtual bool equal(const C& c) const = 0;
		/// 以新的迭代器重置当前迭代器
		virtual C& assign(const C &o) = 0;
		/// 返回一个空的迭代器
		virtual C& eof() const = 0;
	};

	template <typename C,typename T>
	class bidirectional_iterator:public std::iterator<std::bidirectional_iterator_tag,T,int>
	{
	  public:
		virtual ~bidirectional_iterator()
		{}

		bidirectional_iterator()
		{}

		bidirectional_iterator(const C &o)
		{
			assign(o);
		}

		C& operator =(const C &o)
		{
			return assign(o);
		}

		bool operator ==(const C &o)
		{
			return equal(o);
		}

		bool operator !=(const C &o)
		{
			return !equal(o);
		}

		C& operator ++()
		{
			tonext();
			return *(C*)this;
		}

		C operator ++(int)
		{
			C c = *(C*)this;
			++*(C*)this;
			return c;
		}

		C& operator --()
		{
			toprev();
			return *(C*)this;
		}

		C operator --(int)
		{
			C c = *(C*)this;
			--*(C*)this;
			return c;
		}

		T& operator*()
		{
			return *get();
		}

		T* operator->()
		{
			return get();
		}
	  protected:
		/// 获取当前数据的指针
		virtual T* get() = 0;
		/// 数据指针指向下一个迭代器
		virtual void tonext() = 0;
		/// 数据指针指向上一个迭代器
		virtual void toprev() = 0;
		/// 比较两个迭代器是否相等
		virtual bool equal(const C& c) const = 0;
		/// 以新的迭代器重置当前迭代器
		virtual C& assign(const C &o) = 0;
		/// 返回一个空的迭代器
		virtual C& eof() const = 0;
	};

	template <typename C,typename T,typename L=int>
	class random_access_iterator:public std::iterator<std::random_access_iterator_tag,T,int>
	{
	  public:
		typedef L key_type;

		virtual ~random_access_iterator()
		{}

		random_access_iterator()
		{}

		random_access_iterator(const C &o)
		{
			assign(o);
		}

		C& operator =(const C &o)
		{
			return assign(o);
		}

		bool operator ==(const C &o)
		{
			return equal(o);
		}

		bool operator !=(const C &o)
		{
			return !equal(o);
		}

		C& operator ++()
		{
			L ci = index() + 1;
			if(hittest(ci)){
				to(ci);
				return *(C*)this;
			}
			*(C*)this = eof();
			return *(C*)this;
		}

		C operator ++(int)
		{
			C c = *(C*)this;
			++*(C*)this;
			return c;
		}

		C& operator --()
		{
			key_type ci = index() - 1;
			if(hittest(ci)){
				return to(ci);
			}
			*(C*)this = eof();
			return *(C*)this;
		}

		C operator --(int)
		{
			C c = *(C*)this;
			--*(C*)this;
			return c;
		}

		C operator +(key_type i)
		{
			C c = *(C*)this;
			return c+=i;
		}

		C operator -(key_type i)
		{
			C c = *(C*)this;
			return c-=i;
		}

		C& operator +=(key_type i)
		{
			key_type ci = index() + i;
			if(hittest(ci)){
				to(ci);
				return *(C*)this;
			}
			*(C*)this = eof();
			return *(C*)this;
		}

		C& operator -=(key_type i)
		{
			key_type ci = index() - i;
			if(hittest(ci)){
				to(ci);
				return *(C*)this;
			}
			*(C*)this = eof();
			return *(C*)this;
		}

		bool operator <(const C &o);
		bool operator >(const C &o);

		bool operator <=(const C &o);
		bool operator >=(const C &o);

		T* operator [](key_type i)
		{
			key_type ci = 1;
			if(hittest(ci)){
				to(ci);
				return *(C*)this;
			}
			return get();
		}

		T& operator*()
		{
			return *get();
		}

		T* operator->()
		{
			return get();
		}
	  protected:
		/// 获取当前数据的指针
		virtual T* get() = 0;
		/// 将当前迭代器指向指定索引位置
		virtual void to(key_type i) = 0;
		/// 取得当前数据指针的索引值
		virtual key_type index() const = 0;
		/// 比较两个迭代器是否相等
		virtual bool equal(const C& c) const = 0;
		/// 以新的迭代器重置当前迭代器
		virtual C& assign(const C &o) = 0;
		/// 测试当前索引是否有较
		virtual bool hittest(key_type i) const = 0;
		/// 返回一个空的迭代器
		virtual C& eof() const = 0;
	};
}}



#endif
