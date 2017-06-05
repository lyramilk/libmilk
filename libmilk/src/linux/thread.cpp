#include "thread.h"
#include <sys/sysinfo.h>

namespace lyramilk{namespace threading
{
	// threads
	threads::return_type threads::thread_task(threads* p)
	{
		threads::return_type ret = 0;
		__sync_add_and_fetch(&p->c,1);
		ret = p->svc();
		__sync_sub_and_fetch(&p->c,1);
		return ret;
	}

	threads::threads()
	{
		c = 0;
	}

	threads:: ~threads()
	{
		void* tmp;
		vector_type::iterator it = m.begin();
		for(;it != m.end();++it){
			pthread_join(*it,&tmp);
		}
	}

	void threads::active(std::size_t threadcount)
	{
		m.reserve(threadcount);

		for(std::size_t i = 0;i<threadcount;++i){
			pthread_t thread;
			if(pthread_create(&thread,NULL,(void* (*)(void*))thread_task,this) == 0){
				m.push_back(thread);
			}
		}
	}

	void threads::active()
	{
		std::size_t t = get_nprocs();
		if(t < 1){
			t = 1;
		}else if(t > 1000){
			t = 20;
		}else{
			std::size_t e = t;
			t <<= 1;
			t |= e;
		}
		active(t);
	}

	void threads::detach()
	{
		vector_type::iterator it = m.begin();
		for(;it != m.end();++it){
			pthread_detach(*it);
		}
		m.clear();
	}

	std::size_t threads::size()
	{
		return c;
	}

	// mutex_super
	mutex_super::~mutex_super()
	{}

	// mutex_sync
	mutex_sync::mutex_sync(mutex_super& l):_mutexobj(l)
	{
		_mutexobj.lock();
	}

	mutex_sync::~mutex_sync()
	{
		_mutexobj.unlock();
	}

	// mutex_spin
	mutex_spin::mutex_spin()
	{
		locked = false;
	}

	mutex_spin::~mutex_spin()
	{
	}

	void mutex_spin::lock()
	{
		while(!__sync_bool_compare_and_swap(&locked,false,true));
	}

	void mutex_spin::unlock()
	{
		locked = false;
	}

	bool mutex_spin::try_lock()
	{
		return __sync_bool_compare_and_swap(&locked,false,true);
	}

	bool mutex_spin::test() const
	{
		return __sync_bool_compare_and_swap((volatile bool*)&locked,false,false);
	}

	// mutex
	mutex_os::mutex_os()
	{
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&handler,&attr);
	}
	mutex_os::~mutex_os()
	{
		pthread_mutex_destroy(&handler);
	}

	void mutex_os::lock()
	{
		pthread_mutex_lock(&handler);
	}

	void mutex_os::unlock()
	{
		pthread_mutex_unlock(&handler);
	}

	bool mutex_os::try_lock()
	{
		return pthread_mutex_trylock(&handler) == 0;
	}

	bool mutex_os::test() const
	{
		if(pthread_mutex_trylock((pthread_mutex_t*)&handler) == 0){
			pthread_mutex_unlock((pthread_mutex_t*)&handler);
			return true;
		}
		return false;
	}

	// rwlock
	class rwlock_r:public mutex_super
	{
		pthread_rwlock_t* el;

		virtual void lock()
		{
			pthread_rwlock_rdlock(el);
		}

		virtual void unlock()
		{
			pthread_rwlock_unlock(el); 
		}

		virtual bool try_lock()
		{
			return 0 == pthread_rwlock_tryrdlock(el);
		}

		virtual bool test() const
		{
			if(((rwlock_r*)this)->try_lock()){
				return true;
			}
			return false;
		}
	  public:
		rwlock_r(pthread_rwlock_t* el)
		{
			this->el = el;
		}

		virtual ~rwlock_r()
		{}
	};

	class rwlock_w:public mutex_super
	{
		pthread_rwlock_t* el;

		virtual void lock()
		{
			pthread_rwlock_wrlock(el);
		}

		virtual void unlock()
		{
			pthread_rwlock_unlock(el); 
		}

		virtual bool try_lock()
		{
			return 0 == pthread_rwlock_trywrlock(el);
		}

		virtual bool test() const
		{
			if(((rwlock_w*)this)->try_lock()){
				return true;
			}
			return false;
		}
	  public:
		rwlock_w(pthread_rwlock_t* el)
		{
			this->el = el;
		}

		virtual ~rwlock_w()
		{}
	};


	mutex_rw::mutex_rw()
	{
		pthread_rwlock_init(&lock,NULL);
		pr = new rwlock_r(&lock);
		pw = new rwlock_w(&lock);
	}

	mutex_rw::~mutex_rw()
	{
		if(pr)delete pr;
		if(pw)delete pw;
		pthread_rwlock_destroy(&lock);
	}

	mutex_rw::mutex_rw(const mutex_rw& o)
	{
		pthread_rwlock_init(&lock,NULL);
		pr = new rwlock_r(&lock);
		pw = new rwlock_w(&lock);
	}

	mutex_rw& mutex_rw::operator =(const mutex_rw& o)
	{
		if(pr)delete pr;
		if(pw)delete pw;
		pthread_rwlock_destroy(&lock);
		pthread_rwlock_init(&lock,NULL);
		pr = new rwlock_r(&lock);
		pw = new rwlock_w(&lock);
		return *this;
	}

	mutex_super& mutex_rw::r()
	{
		return *pr;
	}

	mutex_super& mutex_rw::w()
	{
		return *pw;
	}


	struct thread_cleaner_param
	{
		void (*routine)(void*);
		void* arg;
	};

	void thread_cleanup_push(void (*routine)(void*),void* arg)
	{
		pthread_key_t key;
		pthread_key_create(&key,routine);
		pthread_setspecific(key,arg);
	}

}}
