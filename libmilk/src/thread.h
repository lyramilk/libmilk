#ifndef _lyramilk_system_threading_thread_h_
#define _lyramilk_system_threading_thread_h_

#include "config.h"
#include <vector>
#include <list>
#include <cassert>

#ifdef __linux__
	#include <pthread.h>
	#include <semaphore.h>
#elif defined WIN32
	#include <windows.h>
	typedef HANDLE pthread_t;
	typedef HANDLE pthread_mutex_t;
	typedef SRWLOCK pthread_rwlock_t;
#endif

/**
	@namespace lyramilk::threading
	@brief 线程
	@details 该命名空间描述线程相关操作
*/

namespace lyramilk{namespace threading
{
	/**
		@brief 线程组
		@details 线程组对象，激活后多线程执行svc函数。使用的时候继承这个对象然后覆盖svc函数就可以。
	*/
#ifdef WIN32
	template class _lyramilk_api_ std::vector < pthread_t >;
#endif
	class _lyramilk_api_ threads
	{
	  protected:
		typedef int return_type;

		typedef std::vector<pthread_t> vector_type;

		vector_type m;
#ifdef WIN32
		static int __stdcall thread_task(threads* p);
#else
		static int thread_task(threads* p);
#endif
	  public:
		/**
			@brief 线程组构造
		*/
		threads();
		/**
			@brief 线程组析构
			@details 析构时会等待所有线程结束才会真正退出。
		*/
		virtual ~threads();

		/**
			@brief 激活线程组
			@details 激活时将启动多个线程执行svc
			@param threadcount 激活的线程数。
		*/
		virtual void active(std::size_t threadcount = 10);
		/**
			@brief 线程组析构
			@details 析构时会等待所有线程结束才会真正退出。
		*/
		virtual void detach();
		/**
			@brief 线程函数，激活线程时触发。
			@details 子类中该成员函数将在额外的线程中执行。
			@return 这个返回值将作为线程的返回值。
		*/
		virtual int svc() = 0;
	};


	/// 互斥锁基类
	class _lyramilk_api_ mutex_super
	{
	  public:
		virtual ~mutex_super();
		/*
			@brief 阻塞上锁
			@details 该操作会阻塞直到上锁成功
		*/
		virtual void lock() = 0;
		/// 解锁
		virtual void unlock() = 0;
		/*
			@brief 非阻塞上锁
			@details 是否上锁成功需要看返回值
		*/
		virtual bool try_lock() = 0;

		/// 测试锁是否可以加锁，但并不实际对锁进行操作
		virtual bool test() const = 0;
	};

	/// 互斥锁的自动操作类，该类对象在构造和析构的时候自动加锁解锁。
	class _lyramilk_api_ mutex_sync
	{
		mutex_super& _mutexobj;
	  public:
		mutex_sync(mutex_super& l);
		~mutex_sync();
	};

	/**
		@brief 自旋互斥锁对象
		@details 不可重入的互斥锁，这个锁会使CPU处于忙等待。
	*/
	class _lyramilk_api_ mutex_spin:public mutex_super
	{
	  public:
#ifdef __GNUC__
		typedef bool native_handle_type;
#elif defined _MSC_VER
		typedef long native_handle_type;
#endif
	  protected:
		native_handle_type locked;
	  public:
		mutex_spin();
		virtual ~mutex_spin();
		virtual void lock();
		virtual void unlock();
		virtual bool try_lock();
		virtual bool test() const;
	};

	/**
		@brief 互斥锁对象
		@details 可重入的互斥锁。
	*/
	class _lyramilk_api_ mutex_os:public mutex_super
	{
	  public:
		typedef pthread_mutex_t native_handle_type;
	  protected:
		native_handle_type handler;
	  public:
		mutex_os();
		virtual ~mutex_os();
		virtual void lock();
		virtual void unlock();
		virtual bool try_lock();
		virtual bool test() const;
	  public:
	};

	/**
		@brief 读写锁
		@details 在互斥锁的基础上允许读与读操作不互斥。
	*/
	class _lyramilk_api_ rwlock
	{
		mutex_super* pr;
		mutex_super* pw;
	  public:
		rwlock();
		virtual ~rwlock();
		/**
			@brief 申请读锁
			@details 申请读锁，被占用时等待一段时间。
			@return 成功时返回true
		*/
		mutex_super& r();

		/**
			@brief 申请读锁
			@details 申请读锁，被占用时等待一段时间。
			@return 成功时返回true
		*/
		mutex_super& w();
	  protected:
		pthread_rwlock_t lock;
	};


	template <typename T>
	class atomic
	{
		T t;
	  public:
		atomic():t(0)
		{}
		atomic(const atomic& o)
		{
			t = o.t;
		}
		atomic(T o)
		{
			t = o; 
		}

		operator T()
		{
			return t;
		}

		atomic& operator =(const atomic& o)
		{
			__sync_bool_compare_and_swap(&t,t,o.t);
			return *this;
		}

		atomic& operator =(const T& o)
		{
			__sync_bool_compare_and_swap(&t,t,o);
			return *this;
		}
		
		atomic& operator +=(const T& o)
		{
			__sync_fetch_and_add(&t,o);
			return *this;
		}
		
		atomic& operator -=(const T& o)
		{
			__sync_fetch_and_sub(&t,o);
			return *this;
		}
		
		atomic operator +(const T& o)
		{
			return atomic(t) += o;
		}
		
		atomic& operator -(const T& o)
		{
			return atomic(t) -= o;
		}
		
		atomic& operator ++()
		{
			__sync_fetch_and_add(&t,1);
			return *this;
		}
		
		atomic& operator --()
		{
			__sync_fetch_and_sub(&t,1);
			return *this;
		}
		
		atomic operator ++(int)
		{
			atomic tmp(t);
			++*this;
			return t;
		}
		
		atomic operator --(int)
		{
			atomic tmp(t);
			--*this;
			return t;
		}
	};


	namespace exclusive{
		template <typename T>
		class list
		{
			mutex_spin l;
		  public:
			struct item
			{
				mutex_spin l;
				T* t;
				list* c;
				atomic<int> r;
				item(T* t,list* c)
				{
					this->t = t;
					this->c = c;
					r = 0;
				}

				~item()
				{}

				bool trylock()
				{
					if(l.try_lock()){
						c->onhire(t);
						return true;
					}
					return false;
				}

				void lock()
				{
					l.lock();
					c->onhire(t);
				}

				bool addref()
				{
					++r;
					return true;
				}

				bool release()
				{
					--r;
					if(r < 1){
						c->onfire(t);
						l.unlock();
					}
					return true;
				}

			};
			class ptr
			{
				mutable item* q;
			  public:
				ptr():q(nullptr)
				{
				}

				ptr(const ptr& p):q(p.q)
				{
					if(q){
						q->addref();
					}
				}

				ptr(item* pi):q(pi)
				{
					if(q)q->addref();
				}

				~ptr()
				{
					if(q)q->release();
				}

				ptr& operator =(const ptr& o) const
				{
					o.q->addref();
					if(q)q->release();
					q = o.q;
				}

				T* operator->() const
				{
					return q->t;
				}

				operator T*() const
				{
					return q->t;
				}
			};

			ptr get()
			{
				typename std::list<item>::iterator it = es.begin();
				for(;it!=es.end();++it){
					if(it->trylock()){
						return ptr(&*it);
					}
				}
				T* tmp = underflow();
				if(tmp){
					mutex_sync _(l);
					es.push_back(item(tmp,this));
					es.back().trylock();
					return ptr(&es.back());
				}

				assert(!es.empty());
				while(true){
					typename std::list<item>::iterator it = es.begin();
					for(;it!=es.end();++it){
						if(it->trylock()){
							return ptr(&*it);
						}
					}
				}
			}
		  protected:
			virtual T* underflow() = 0;
			virtual void onhire(T* o)
			{}
			virtual void onfire(T* o)
			{}
			typedef std::list<item> list_type;
			list_type es;
		};
	}
}}

#endif
