#ifndef _lyramilk_system_threading_atom_h_
#define _lyramilk_system_threading_atom_h_

#include "config.h"
#include "thread.h"

/**
	@namespace lyramilk::threading
	@brief 线程
	@details 该命名空间描述线程相关操作
*/

namespace lyramilk{namespace threading
{
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
		
		bool operator ==(const T& o)
		{
			return __sync_bool_compare_and_swap(&t,o,t);
		}
		
		bool operator !=(const T& o)
		{
			return !__sync_bool_compare_and_swap(&t,o,t);
		}
	};


	namespace exclusive{
		template <typename T>
		class list
		{
		  public:
			struct item
			{
				mutex_spin l;
				T* t;
				list* c;
				int r;
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
					__sync_add_and_fetch(&r,1);
					return true;
				}

				bool release()
				{
					if(__sync_sub_and_fetch(&r,1) < 1){
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
					if(q) q->addref();
				}

				ptr(item* pi):q(pi)
				{
					if(q) q->addref();
				}

				~ptr()
				{
					if(q) q->release();
				}

				bool good()
				{
					return q != NULL;
				}

				ptr& operator =(const ptr& o)
				{
					if(o.q) o.q->addref();
					if(q) q->release();
					q = o.q;
					return *this;
				}

				T* operator->() const
				{
					return q?q->t:nullptr;
				}

				operator T*() const
				{
					return q?q->t:nullptr;
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
					es.push_back(item(tmp,this));
					es.back().trylock();
					return ptr(&es.back());
				}
				return ptr(nullptr);
			}


			virtual void clear()
			{
				typename std::list<item>::iterator it = es.begin();
				for(;it!=es.end();++it){
					onremove(it->t);
				}
				es.clear();
				
			}
		  protected:
			virtual T* underflow() = 0;
			virtual void onhire(T* o)
			{}
			virtual void onfire(T* o)
			{}
			virtual void onremove(T* o)
			{}

			list()
			{}

			virtual ~list()
			{}

			typedef std::list<item> list_type;
			list_type es;
		};
	}
}}

#endif
