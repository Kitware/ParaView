/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file       mi/base/lock.h
/// \brief      Multithreading locks.
///
/// See \ref mi_base_threads.

#ifndef MI_BASE_LOCK_H
#define MI_BASE_LOCK_H

#include <cstdlib>

#include <mi/base/assert.h>
#include <mi/base/config.h>

#ifndef MI_PLATFORM_WINDOWS
#include <cerrno>
#include <pthread.h>
#else
#include <mi/base/miwindows.h>
#endif

namespace mi
{

namespace base
{

/** \defgroup mi_base_threads Multithreading Support
    \ingroup mi_base

    Primitives useful for multithreaded applications, for example, atomic counters, condition
    variables, and locks.
*/

/** \addtogroup mi_base_threads
@{
*/

/// %Non-recursive lock class.
///
/// The lock implements a critical region that only one thread can enter at a time. The lock is
/// non-recursive, i.e., a thread that holds the lock can not lock it again. Any attempt to do so
/// will abort the process.
///
/// Other pre- and post-conditions are checked via #mi_base_assert.
///
/// \see #mi::base::Lock::Block
class Lock
{
public:
  /// Constructor.
  Lock();

  /// Destructor.
  ~Lock();

  /// Utility class to acquire a lock that is released by the destructor.
  ///
  /// \see #mi::base::Lock
  class Block
  {
  public:
    /// Constructor.
    ///
    /// \param lock   If not \c NULL, this lock is acquired. If \c NULL, #set() can be used to
    ///               explicitly acquire a lock later.
    explicit Block(Lock* lock = 0);

    /// Destructor.
    ///
    /// Releases the lock (if it is acquired).
    ~Block();

    /// Acquires a lock.
    ///
    /// Releases the current lock (if it is set) and acquires the given lock. Useful to acquire
    /// a different lock, or to acquire a lock if no lock was acquired in the constructor.
    ///
    /// This method does nothing if the passed lock is already acquired by this class.
    ///
    /// \param lock   The new lock to acquire.
    void set(Lock* lock);

    /// Tries to acquire a lock.
    ///
    /// Releases the current lock (if it is set) and tries to acquire the given lock. Useful to
    /// acquire a different lock without blocking, or to acquire a lock without blocking if no
    /// lock was acquired in the constructor.
    ///
    /// This method does nothing if the passed lock is already acquired by this class.
    ///
    /// \param lock   The new lock to acquire.
    /// \return       \c true if the lock was acquired, \c false otherwise.
    bool try_set(Lock* lock);

    /// Releases the lock.
    ///
    /// Useful to release the lock before the destructor is called.
    void release();

  private:
    // The lock associated with this helper class.
    Lock* m_lock;
  };

protected:
  /// %Locks the lock.
  void lock();

  /// Tries to lock the lock.
  bool try_lock();

  /// Unlocks the lock.
  void unlock();

private:
  // This class is non-copyable.
  Lock(Lock const&);

  // This class is non-assignable.
  Lock& operator=(Lock const&);

#ifndef MI_PLATFORM_WINDOWS
  // The mutex implementing the lock.
  pthread_mutex_t m_mutex;
#else
  // The critical section implementing the lock.
  CRITICAL_SECTION m_critical_section;
  // The flag used to ensure that the lock is non-recursive.
  bool m_locked;
#endif
};

/// %Recursive lock class.
///
/// The lock implements a critical region that only one thread can enter at a time. The lock is
/// recursive, i.e., a thread that holds the lock can lock it again.
///
/// Pre- and post-conditions are checked via #mi_base_assert.
///
/// \see #mi::base::Lock::Block
class Recursive_lock
{
public:
  /// Constructor.
  Recursive_lock();

  /// Destructor.
  ~Recursive_lock();

  /// Utility class to acquire a lock that is released by the destructor.
  ///
  /// \see #mi::base::Recursive_lock
  class Block
  {
  public:
    /// Constructor.
    ///
    /// \param lock   If not \c NULL, this lock is acquired. If \c NULL, #set() can be used to
    ///               explicitly acquire a lock later.
    explicit Block(Recursive_lock* lock = 0);

    /// Destructor.
    ///
    /// Releases the lock (if it is acquired).
    ~Block();

    /// Acquires a lock.
    ///
    /// Releases the current lock (if it is set) and acquires the given lock. Useful to acquire
    /// a different lock, or to acquire a lock if no lock was acquired in the constructor.
    ///
    /// This method does nothing if the passed lock is already acquired by this class.
    ///
    /// \param lock   The new lock to acquire.
    void set(Recursive_lock* lock);

    /// Tries to acquire a lock.
    ///
    /// Releases the current lock (if it is set) and tries to acquire the given lock. Useful to
    /// acquire a different lock without blocking, or to acquire a lock without blocking if no
    /// lock was acquired in the constructor.
    ///
    /// This method does nothing if the passed lock is already acquired by this class.
    ///
    /// \param lock   The new lock to acquire.
    /// \return       \c true if the lock was acquired, \c false otherwise.
    bool try_set(Recursive_lock* lock);

    /// Releases the lock.
    ///
    /// Useful to release the lock before the destructor is called.
    void release();

  private:
    // The lock associated with this helper class.
    Recursive_lock* m_lock;
  };

protected:
  /// %Locks the lock.
  void lock();

  /// Tries to lock the lock.
  bool try_lock();

  /// Unlocks the lock.
  void unlock();

private:
  // This class is non-copyable.
  Recursive_lock(Recursive_lock const&);

  // This class is non-assignable.
  Recursive_lock& operator=(Recursive_lock const&);

#ifndef MI_PLATFORM_WINDOWS
  // The mutex implementing the lock.
  pthread_mutex_t m_mutex;
#else
  // The critical section implementing the lock.
  CRITICAL_SECTION m_critical_section;
#endif
};

#ifndef MI_FOR_DOXYGEN_ONLY

inline Lock::Lock()
{
#ifndef MI_PLATFORM_WINDOWS
  pthread_mutexattr_t mutex_attributes;
  pthread_mutexattr_init(&mutex_attributes);
  pthread_mutexattr_settype(&mutex_attributes, PTHREAD_MUTEX_ERRORCHECK);
  pthread_mutex_init(&m_mutex, &mutex_attributes);
#else
  InitializeCriticalSection(&m_critical_section);
  m_locked = false;
#endif
}

inline Lock::~Lock()
{
#ifndef MI_PLATFORM_WINDOWS
  int result = pthread_mutex_destroy(&m_mutex);
  // Avoid assertion here because it might be mapped to an exception.
  // mi_base_assert( result == 0);
  (void)result;
#else
  // Avoid assertion here because it might be mapped to an exception.
  // mi_base_assert( !m_locked);
  DeleteCriticalSection(&m_critical_section);
#endif
}

inline void Lock::lock()
{
#ifndef MI_PLATFORM_WINDOWS
  int result = pthread_mutex_lock(&m_mutex);
  if (result == EDEADLK)
  {
    mi_base_assert(!"Dead lock");
    abort();
  }
#else
  EnterCriticalSection(&m_critical_section);
  if (m_locked)
  {
    mi_base_assert(!"Dead lock");
    abort();
  }
  m_locked = true;
#endif
}

inline bool Lock::try_lock()
{
#ifndef MI_PLATFORM_WINDOWS
  int result = pthread_mutex_trylock(&m_mutex);
  // Old glibc versions incorrectly return EDEADLK instead of EBUSY
  // (https://sourceware.org/bugzilla/show_bug.cgi?id=4392).
  mi_base_assert(result == 0 || result == EBUSY || result == EDEADLK);
  return result == 0;
#else
  BOOL result = TryEnterCriticalSection(&m_critical_section);
  if (result == 0)
    return false;
  if (m_locked)
  {
    LeaveCriticalSection(&m_critical_section);
    return false;
  }
  m_locked = true;
  return true;
#endif
}

inline void Lock::unlock()
{
#ifndef MI_PLATFORM_WINDOWS
  int result = pthread_mutex_unlock(&m_mutex);
  mi_base_assert(result == 0);
  (void)result;
#else
  mi_base_assert(m_locked);
  m_locked = false;
  LeaveCriticalSection(&m_critical_section);
#endif
}

inline Lock::Block::Block(Lock* lock)
{
  m_lock = lock;
  if (m_lock)
    m_lock->lock();
}

inline Lock::Block::~Block()
{
  release();
}

inline void Lock::Block::set(Lock* lock)
{
  if (m_lock == lock)
    return;
  if (m_lock)
    m_lock->unlock();
  m_lock = lock;
  if (m_lock)
    m_lock->lock();
}

inline bool Lock::Block::try_set(Lock* lock)
{
  if (m_lock == lock)
    return true;
  if (m_lock)
    m_lock->unlock();
  m_lock = lock;
  if (m_lock && m_lock->try_lock())
    return true;
  m_lock = 0;
  return false;
}

inline void Lock::Block::release()
{
  if (m_lock)
    m_lock->unlock();
  m_lock = 0;
}

inline Recursive_lock::Recursive_lock()
{
#ifndef MI_PLATFORM_WINDOWS
  pthread_mutexattr_t mutex_attributes;
  pthread_mutexattr_init(&mutex_attributes);
  pthread_mutexattr_settype(&mutex_attributes, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&m_mutex, &mutex_attributes);
#else
  InitializeCriticalSection(&m_critical_section);
#endif
}

inline Recursive_lock::~Recursive_lock()
{
#ifndef MI_PLATFORM_WINDOWS
  int result = pthread_mutex_destroy(&m_mutex);
  // Avoid assertion here because it might be mapped to an exception.
  // mi_base_assert( result == 0);
  (void)result;
#else
  DeleteCriticalSection(&m_critical_section);
#endif
}

inline void Recursive_lock::lock()
{
#ifndef MI_PLATFORM_WINDOWS
  int result = pthread_mutex_lock(&m_mutex);
  // Avoid assertion here because it might be mapped to an exception.
  // mi_base_assert( result == 0);
  (void)result;
#else
  EnterCriticalSection(&m_critical_section);
#endif
}

inline bool Recursive_lock::try_lock()
{
#ifndef MI_PLATFORM_WINDOWS
  int result = pthread_mutex_trylock(&m_mutex);
  mi_base_assert(result == 0 || result == EBUSY);
  return result == 0;
#else
  BOOL result = TryEnterCriticalSection(&m_critical_section);
  return result != 0;
#endif
}

inline void Recursive_lock::unlock()
{
#ifndef MI_PLATFORM_WINDOWS
  int result = pthread_mutex_unlock(&m_mutex);
  mi_base_assert(result == 0);
  (void)result;
#else
  LeaveCriticalSection(&m_critical_section);
#endif
}

inline Recursive_lock::Block::Block(Recursive_lock* lock)
{
  m_lock = lock;
  if (m_lock)
    m_lock->lock();
}

inline Recursive_lock::Block::~Block()
{
  release();
}

inline void Recursive_lock::Block::set(Recursive_lock* lock)
{
  if (m_lock == lock)
    return;
  if (m_lock)
    m_lock->unlock();
  m_lock = lock;
  if (m_lock)
    m_lock->lock();
}

inline bool Recursive_lock::Block::try_set(Recursive_lock* lock)
{
  if (m_lock == lock)
    return true;
  if (m_lock)
    m_lock->unlock();
  m_lock = lock;
  if (m_lock && m_lock->try_lock())
    return true;
  m_lock = 0;
  return false;
}

inline void Recursive_lock::Block::release()
{
  if (m_lock)
    m_lock->unlock();
  m_lock = 0;
}

#endif // MI_FOR_DOXYGEN_ONLY

/*@}*/ // end group mi_base_threads

} // namespace base

} // namespace mi

#endif // MI_BASE_LOCK_H
