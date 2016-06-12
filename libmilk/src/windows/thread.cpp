#include "thread.h"

namespace lyramilk{
	namespace system{
		namespace threading{

			mutex::mutex()
			{
				handler = CreateMutex(NULL, FALSE, NULL);
			}

			mutex::~mutex()
			{
				CloseHandle(handler);
			}

			void mutex::lock()
			{
				WaitForSingleObject(handler, INFINITE);
			}

			void mutex::unlock()
			{
				ReleaseMutex(handler);
			}

			bool mutex::try_lock()
			{
				return WaitForSingleObject(handler, 0) == WAIT_OBJECT_0;
			}

			bool mutex::test()
			{
				if (try_lock()){
					unlock();
					return true;
				}
				return false;
			}

			mutex::native_handle_type mutex::native_handle()
			{
				return handler;
			}

			mutex::alock::alock(mutex &lock) :mlock(lock)
			{
				mlock.lock();
			}

			mutex::alock::~alock()
			{
				mlock.unlock();
			}
			//////////////////////////////////
			spinmutex::spinmutex()
			{
				locked = 0;
			}

			spinmutex::~spinmutex()
			{
			}

			void spinmutex::lock()
			{
				while (!InterlockedCompareExchange(&locked, 0, 1) == 0);
			}

			void spinmutex::unlock()
			{
				locked = false;
			}

			bool spinmutex::try_lock()
			{
				return InterlockedCompareExchange(&locked, 0, 1) == 1;
			}

			bool spinmutex::test()
			{
				return !locked;
			}

			spinmutex::native_handle_type spinmutex::native_handle()
			{
				return locked;
			}

			spinmutex::alock::alock(spinmutex &lock) :mlock(lock)
			{
				mlock.lock();
			}

			spinmutex::alock::~alock()
			{
				mlock.unlock();
			}
			//////////////////////////////////

			threads::return_type threads::thread_task(threads* p)
			{
				return p->svc();
			}

			threads::threads()
			{
			}

			threads:: ~threads()
			{
				WaitForMultipleObjects((DWORD)m.size(), m.data(), TRUE, INFINITE);
			}

			void threads::active(std::size_t threadcount)
			{
				m.reserve(threadcount);

				for (std::size_t i = 0; i<threadcount; ++i){
					m.push_back(CreateThread(NULL, 0, (PTHREAD_START_ROUTINE)thread_task, this, NULL, NULL));
				}
			}

			void threads::detach()
			{
				vector_type::iterator it = m.begin();
				for (; it != m.end(); ++it){
					CloseHandle(*it);
				}
				m.clear();
			}

			rwlock::rwlock()
			{
				InitializeSRWLock(&lock);
			}

			rwlock::~rwlock()
			{
			}

			bool rwlock::rlock()
			{
				AcquireSRWLockShared(&lock);
				return true;
			}

			bool rwlock::wlock()
			{
				AcquireSRWLockExclusive(&lock);
				return true;
			}

			bool rwlock::try_rlock()
			{
				return TryAcquireSRWLockShared(&lock) != FALSE;
			}

			bool rwlock::try_wlock()
			{
				return TryAcquireSRWLockExclusive(&lock) != FALSE;
			}

			bool rwlock::unrlock()
			{
				ReleaseSRWLockShared(&lock);
				return true;
			}

			bool rwlock::unwlock()
			{
				ReleaseSRWLockExclusive(&lock);
				return true;
			}

			rwlock::arlock::arlock(rwlock &lock) : mlock(lock)
			{
				mlock.rlock();
			}

			rwlock::arlock::~arlock()
			{
				mlock.unrlock();
			}

			rwlock::awlock::awlock(rwlock &lock) : mlock(lock)
			{
				mlock.wlock();
			}

			rwlock::awlock::~awlock()
			{
				mlock.unwlock();
			}
		}
	}
}