/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file       mi/base/condition.h
/// \brief      Multithreading condition.
///
/// See \ref mi_base_threads.

#ifndef MI_BASE_CONDITION_H
#define MI_BASE_CONDITION_H

#include <mi/base/config.h>
#include <mi/base/types.h>

#ifndef MI_PLATFORM_WINDOWS
#include <cerrno>
#include <pthread.h>
#include <sys/time.h>
#else
#include <mi/base/miwindows.h>
#endif

namespace mi {

namespace base {

/** \addtogroup mi_base_threads
@{
*/

/// Conditions allow threads to signal an event and to wait for such a signal, respectively.
class Condition
{
public:
    /// Constructor
    Condition()
    {
#ifndef MI_PLATFORM_WINDOWS
        m_signaled = false;
        pthread_mutex_init( &m_mutex, NULL);
        pthread_cond_init( &m_condvar, NULL);
#else
        m_handle = CreateEvent( NULL, false, false, NULL);
#endif
    }

    /// Destructor
    ~Condition()
    {
#ifndef MI_PLATFORM_WINDOWS
        pthread_mutex_destroy( &m_mutex);
        pthread_cond_destroy( &m_condvar);
#else
        CloseHandle( m_handle);
#endif
    }

    /// Waits for the condition to be signaled.
    ///
    /// If the condition is already signaled at this time the call will return immediately.
    void wait()
    {
#ifndef MI_PLATFORM_WINDOWS
        pthread_mutex_lock( &m_mutex);
        while( !m_signaled)
            pthread_cond_wait( &m_condvar, &m_mutex);
        m_signaled = false;
        pthread_mutex_unlock( &m_mutex);
#else
        WaitForSingleObject( m_handle, INFINITE);
#endif
    }

    /// Waits for the condition to be signaled until a given timeout.
    ///
    /// If the condition is already signaled at this time the call will return immediately.
    ///
    /// \param timeout    Maximum time period (in seconds) to wait for the condition to be signaled.
    /// \return           \c true if the timeout was hit, and \c false if the condition was
    ///                   signaled.
    bool timed_wait( Float64 timeout) {
#ifndef MI_PLATFORM_WINDOWS
        struct timeval now;
        gettimeofday( &now, NULL);
        struct timespec timeout_abs;
        timeout_abs.tv_sec = now.tv_sec + static_cast<long>( floor( timeout));
        timeout_abs.tv_nsec
            = 1000 * now.tv_usec + static_cast<long>( 1E9 * ( timeout - floor( timeout)));
        if( timeout_abs.tv_nsec > 1000000000) {
            timeout_abs.tv_sec  += 1;
            timeout_abs.tv_nsec -= 1000000000;
        }

        bool timed_out = false;
        pthread_mutex_lock( &m_mutex);
        while( !m_signaled)
        {
            int result = pthread_cond_timedwait( &m_condvar, &m_mutex, &timeout_abs);
            timed_out = result == ETIMEDOUT;
            if( result != EINTR)
                break;
        }
        m_signaled = false;
        pthread_mutex_unlock( &m_mutex);
        return timed_out;
#else
        DWORD timeout_ms = static_cast<DWORD>( 1000 * timeout);
        DWORD result = WaitForSingleObject( m_handle, timeout_ms);
        return result == WAIT_TIMEOUT;   
#endif    
    }

    /// Signals the condition.
    ///
    /// This will wake up one thread waiting for the condition. It does not matter if the call to
    /// #signal() or #wait() comes first.
    ///
    /// \note If there are two or more calls to #signal() without a call to #wait() in between (and
    /// no outstanding #wait() call), all calls to #signal() except the first one are ignored, i.e.,
    /// calls to #signal() do not increment some counter, but just set a flag.
    void signal()
    {
#ifndef MI_PLATFORM_WINDOWS
        pthread_mutex_lock( &m_mutex);
        m_signaled = true;
        pthread_cond_signal( &m_condvar);
        pthread_mutex_unlock( &m_mutex);
#else
        SetEvent( m_handle);
#endif
    }

    /// Resets the condition.
    ///
    /// This will undo the effect of a #signal() call if there was no outstanding #wait() call.
    void reset()
    {
#ifndef MI_PLATFORM_WINDOWS
        pthread_mutex_lock( &m_mutex);
        m_signaled = false;
        pthread_mutex_unlock( &m_mutex);
#else
        ResetEvent( m_handle);
#endif
    }

private:
#ifndef MI_PLATFORM_WINDOWS
    /// The mutex to be used to protect the m_signaled variable.
    pthread_mutex_t m_mutex;
    /// The condition used to let the thread sleep.
    pthread_cond_t m_condvar;
    /// A variable storing the signaled state.
    bool m_signaled;
#else
    /// The event handle
    HANDLE m_handle;
#endif
};

/*@}*/ // end group mi_base_threads

} // namespace base

} // namespace mi

#endif // MI_BASE_CONDITION_H
