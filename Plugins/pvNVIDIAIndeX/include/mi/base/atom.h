/***************************************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file  mi/base/atom.h
/// \brief 32-bit unsigned counter with atomic arithmetic, increments, and decrements.
///
/// See \ref mi_base_threads.

#ifndef MI_BASE_ATOM_H
#define MI_BASE_ATOM_H

#include <mi/base/config.h>
#include <mi/base/types.h>

// Select implementation to use
#if defined( MI_ARCH_X86) && defined( MI_COMPILER_MSC)
#  define MI_ATOM32_X86MSC
#  include <intrin.h>
#  pragma intrinsic( _InterlockedExchangeAdd)
#  pragma intrinsic( _InterlockedCompareExchange)
#elif defined( MI_ARCH_X86) && (defined( MI_COMPILER_GCC) || defined( MI_COMPILER_ICC))
#  define MI_ATOM32_X86GCC
#else
#  define MI_ATOM32_GENERIC
#  include <mi/base/lock.h>
#endif

namespace mi {

namespace base {

/** \addtogroup mi_base_threads
@{
*/

/// A 32-bit unsigned counter with atomic arithmetic, increments, and decrements.
class Atom32
{
public:
    /// The default constructor initializes the counter to zero.
    Atom32() : m_value( 0) { }

    /// This constructor initializes the counter to \p value.
    Atom32( const Uint32 value) : m_value( value) { }

#if defined( MI_ATOM32_GENERIC)
    /// The copy constructor assigns the value of \p other to the counter.
    Atom32( const mi::base::Atom32& other) : m_value( other.m_value) { }

    /// Assigns the value of \p rhs to the counter.
    mi::base::Atom32& operator=( const mi::base::Atom32& rhs);
#endif

    /// Assigns \p rhs to the counter.
    Uint32 operator=( const Uint32 rhs) { m_value = rhs; return rhs; }

    /// Adds \p rhs to the counter.
    Uint32 operator+=( const Uint32 rhs);

    /// Subtracts \p rhs from the counter.
    Uint32 operator-=( const Uint32 rhs);

    /// Increments the counter by one (pre-increment).
    Uint32 operator++();

    /// Increments the counter by one (post-increment).
    Uint32 operator++( int);

    /// Decrements the counter by one (pre-decrement).
    Uint32 operator--();

    /// Decrements the counter by one (post-decrement).
    Uint32 operator--( int);

    /// Conversion operator to #mi::Uint32.
    operator Uint32() const { return m_value; }

    /// Assigns \p rhs to the counter and returns the old value of counter.
    Uint32 swap( const Uint32 rhs);

private:
    // The counter.
    volatile Uint32 m_value;

#if defined( MI_ATOM32_GENERIC)
    // The lock for #m_value needed by the generic implementation.
    mi::base::Lock m_lock;
#endif
};

#if !defined( MI_FOR_DOXYGEN_ONLY)

#if defined( MI_ATOM32_X86MSC)

__forceinline Uint32 Atom32::operator+=( const Uint32 rhs)
{
    return _InterlockedExchangeAdd( reinterpret_cast<volatile long*>( &m_value), rhs) + rhs;
}

__forceinline Uint32 Atom32::operator-=( const Uint32 rhs)
{
    return _InterlockedExchangeAdd(
        reinterpret_cast<volatile long*>( &m_value), -static_cast<const Sint32>( rhs)) - rhs;
}

__forceinline Uint32 Atom32::operator++()
{
    return _InterlockedExchangeAdd( reinterpret_cast<volatile long*>( &m_value), 1L) + 1L;
}

__forceinline Uint32 Atom32::operator++( int)
{
    return _InterlockedExchangeAdd( reinterpret_cast<volatile long*>( &m_value), 1L);
}

__forceinline Uint32 Atom32::operator--()
{
    return _InterlockedExchangeAdd( reinterpret_cast<volatile long*>( &m_value), -1L) - 1L;
}

__forceinline Uint32 Atom32::operator--( int)
{
    return _InterlockedExchangeAdd( reinterpret_cast<volatile long*>( &m_value), -1L);
}

__forceinline Uint32 Atom32::swap( const Uint32 rhs)
{
    return _InterlockedExchange( reinterpret_cast<volatile long*>( &m_value), rhs);
}

#elif defined( MI_ATOM32_X86GCC) // defined( MI_ATOM32_X86MSC)

inline Uint32 Atom32::operator+=( const Uint32 rhs)
{
    Uint32 retval;
    asm volatile(
        "movl %2,%0\n"
        "lock; xaddl %0,%1\n"
        "addl %2,%0\n"
        : "=&r"( retval), "+m"( m_value)
        : "r"( rhs)
        : "cc"
        );
    return retval;
}

inline Uint32 Atom32::operator-=( const Uint32 rhs)
{
    Uint32 retval;
    asm volatile(
        "neg %2\n"
        "movl %2,%0\n"
        "lock; xaddl %0,%1\n"
        "addl %2,%0\n"
        : "=&r"( retval), "+m"( m_value)
        : "r"( rhs)
        : "cc", "%2"
        );
    return retval;
}

inline Uint32 Atom32::operator++()
{
    Uint32 retval;
    asm volatile(
        "movl $1,%0\n"
        "lock; xaddl %0,%1\n"
        "addl $1,%0\n"
        : "=&r"( retval), "+m"( m_value)
        :
        : "cc"
        );
    return retval;
}

inline Uint32 Atom32::operator++( int)
{
    Uint32 retval;
    asm volatile(
        "movl $1,%0\n"
        "lock; xaddl %0,%1\n"
        : "=&r"( retval), "+m"( m_value)
        :
        : "cc"
        );
    return retval;
}

inline Uint32 Atom32::operator--()
{
    Uint32 retval;
    asm volatile(
        "movl $-1,%0\n"
        "lock; xaddl %0,%1\n"
        "addl $-1,%0\n"
        : "=&r"( retval), "+m"( m_value)
        :
        : "cc"
        );
    return retval;
}

inline Uint32 Atom32::operator--( int)
{
    Uint32 retval;
    asm volatile(
        "movl $-1,%0\n"
        "lock; xaddl %0,%1\n"
        : "=&r"( retval), "+m"( m_value)
        :
        : "cc"
        );
    return retval;
}

inline Uint32 Atom32::swap( const Uint32 rhs)
{
    Uint32 retval;
    asm volatile(
    "0:\n"
        "movl %1,%0\n"
        "lock; cmpxchg %2,%1\n"
        "jnz 0b\n"
        : "=&a"( retval), "+m"( m_value)
        : "r"( rhs)
        : "cc"
        );
    return retval;
}

#elif defined( MI_ATOM32_GENERIC) // defined( MI_ATOM32_X86GCC)

inline mi::base::Atom32& Atom32::operator=( const mi::base::Atom32& rhs)
{
    m_value = rhs.m_value;
    return *this;
}

inline Uint32 Atom32::operator+=( const Uint32 rhs)
{
    mi::base::Lock::Block block( &m_lock);
    return m_value += rhs;
}

inline Uint32 Atom32::operator-=( const Uint32 rhs)
{
    mi::base::Lock::Block block( &m_lock);
    return m_value -= rhs;
}

inline Uint32 Atom32::operator++()
{
    mi::base::Lock::Block block( &m_lock);
    return ++m_value;
}

inline Uint32 Atom32::operator++( int)
{
    mi::base::Lock::Block block( &m_lock);
    return m_value++;
}

inline Uint32 Atom32::operator--()
{
    mi::base::Lock::Block block( &m_lock);
    return --m_value;
}

inline Uint32 Atom32::operator--( int)
{
    mi::base::Lock::Block block( &m_lock);
    return m_value--;
}

inline Uint32 Atom32::swap( const Uint32 rhs)
{
    mi::base::Lock::Block block( &m_lock);
    Uint32 retval = m_value;
    m_value = rhs;
    return retval;
}

#else // MI_ATOM32_GENERIC
#error One of MI_ATOM32_X86MSC, MI_ATOM32_X86GCC, or MI_ATOM32_GENERIC must be defined. 
#endif

#undef MI_ATOM32_X86MSC
#undef MI_ATOM32_X86GCC
#undef MI_ATOM32_GENERIC

#endif // !MI_FOR_DOXYGEN_ONLY

/*@}*/ // end group mi_base_threads

} // namespace base

} // namespace mi

#endif // MI_BASE_ATOM_H
