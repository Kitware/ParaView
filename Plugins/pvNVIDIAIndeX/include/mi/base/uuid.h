/***************************************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file mi/base/uuid.h
/// \brief A 128 bit representation of a universally unique identifier (UUID or GUID).
///
/// See \ref mi_base_iinterface, type #mi::base::Uuid and template class #mi::base::Uuid_t.

#ifndef MI_BASE_UUID_H
#define MI_BASE_UUID_H

#include <mi/base/types.h>

namespace mi {
namespace base {

/** \addtogroup mi_base_iinterface
@{
*/

/// A 128 bit representation of a universally unique identifier (UUID or GUID).
///
/// A UUID is represented as a sequence of four #mi::Uint32 values. It supports
/// all comparison operators and an #mi::base::uuid_hash32() function.
struct Uuid
{
    Uint32 m_id1; ///< First  value.
    Uint32 m_id2; ///< Second value.
    Uint32 m_id3; ///< Third  value.
    Uint32 m_id4; ///< Fourth value.
};

/// Returns \c true if \c id1 is equal to \p id2.
inline bool operator== ( const Uuid & id1, const Uuid & id2)
{
    if( id1.m_id1 != id2.m_id1 ) return false;
    if( id1.m_id2 != id2.m_id2 ) return false;
    if( id1.m_id3 != id2.m_id3 ) return false;
    if( id1.m_id4 != id2.m_id4 ) return false;
    return true;
}

/// Returns \c true if \c id1 is not equal to \p id2.
inline bool operator!= ( const Uuid & id1, const Uuid & id2)
{
    if( id1.m_id1 != id2.m_id1 ) return true;
    if( id1.m_id2 != id2.m_id2 ) return true;
    if( id1.m_id3 != id2.m_id3 ) return true;
    if( id1.m_id4 != id2.m_id4 ) return true;
    return false;
}

/// Returns \c true if \p id1 is less than \p id2.
inline bool operator< ( const Uuid & id1, const Uuid & id2)
{
    if( id1.m_id1 < id2.m_id1 ) return true;
    if( id1.m_id1 > id2.m_id1 ) return false;
    if( id1.m_id2 < id2.m_id2 ) return true;
    if( id1.m_id2 > id2.m_id2 ) return false;
    if( id1.m_id3 < id2.m_id3 ) return true;
    if( id1.m_id3 > id2.m_id3 ) return false;
    if( id1.m_id4 < id2.m_id4 ) return true;
    return false;
}

/// Returns \c true if \p id1 is greater than \p id2.
inline bool operator > ( const Uuid & id1, const Uuid & id2) {
    if( id1.m_id1 > id2.m_id1 ) return true;
    if( id1.m_id1 < id2.m_id1 ) return false;
    if( id1.m_id2 > id2.m_id2 ) return true;
    if( id1.m_id2 < id2.m_id2 ) return false;
    if( id1.m_id3 > id2.m_id3 ) return true;
    if( id1.m_id3 < id2.m_id3 ) return false;
    if( id1.m_id4 > id2.m_id4 ) return true;
    return false;
}

/// Returns \c true if \p id1 is less than or equal to \p id2.
inline bool operator<= ( const Uuid & id1, const Uuid & id2)
{
    return !(id1 > id2);
}

/// Returns \c true if \p id1 is greater than or equal to \p id2.
inline bool operator >= ( const Uuid & id1, const Uuid & id2)
{
    return !(id1 < id2);
}

/// Returns a 32 bit hash value by performing a bitwise xor of all four 32 bit values.
inline Uint32 uuid_hash32( const Uuid& id)
{
    return id.m_id1 ^ id.m_id2 ^ id.m_id3 ^ id.m_id4;
}


/** Class template for a compile-time representation of universally unique
    identifiers (UUIDs or GUIDs).

    The identifiers are represented as a sequence of four compile-time constants
    in the form of \c static \c const #mi::Uint32 types.

    This class provides an implicit conversion to #mi::base::Uuid. It
    initializes the #mi::base::Uuid value with the compile-time constants
    (\p m_id1, \p m_id2, \p m_id3, \p m_id4). Thus, a simple object
    instantiation of a class instance of this template can be passed
    as id along to functions expecting an #mi::base::Uuid argument such as
    #mi::base::IInterface::get_interface().

    This class template provides also a static const \c hash32 value
    of type #mi::Uint32, which can be used as compile-time constant
    expression value, for example, in switch case labels.

    The template parameters represent the UUID as the commonly generated
    sequence of one 32 bit value, two 16 bit values, and eight 8 bit values.
    The \c uuidgen system utility program creates UUIDs in a similar format.
*/
template <Uint32 id1, Uint16 id2, Uint16 id3
        , Uint8  id4, Uint8  id5, Uint8  id6, Uint8  id7
        , Uint8  id8, Uint8  id9, Uint8 id10, Uint8 id11>
class Uuid_t
{
public:
    /// Conversion operator.
    ///
    /// The conversion to #mi::base::Uuid initializes the UUID to
    /// (\p m_id1, \p m_id2, \p m_id3, \p m_id4).
    operator const Uuid & () const
    {
        static const Uuid uuid = {m_id1, m_id2, m_id3, m_id4};
        return uuid;
    }

    /// First  32 bit out of four.
    static const Uint32 m_id1 = id1;

    /// Second 32 bit out of four.
    static const Uint32 m_id2 = static_cast<Uint32>( id2 | (id3 << 16));

    /// Third  32 bit out of four.
    static const Uint32 m_id3
        = static_cast<Uint32>( id4 | (id5 << 8) | (id6 << 16) | (id7 << 24));

    /// Fourth 32 bit out of four.
    static const Uint32 m_id4
        = static_cast<Uint32>( id8 | (id9 << 8) | (id10 << 16) | (id11 << 24));

    /// A 32 bit hash value from a bitwise xor of all four 32 bit values.
    static const Uint32 hash32 = m_id1 ^ m_id2 ^ m_id3 ^ m_id4;
};

/*@}*/ // end group mi_base_iinterface

} // namespace base
} // namespace mi

#endif // MI_BASE_UUID_H
