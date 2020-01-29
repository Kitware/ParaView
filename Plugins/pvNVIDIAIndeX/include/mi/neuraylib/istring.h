/***************************************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief String type.

#ifndef MI_NEURAYLIB_ISTRING_H
#define MI_NEURAYLIB_ISTRING_H

#include <mi/neuraylib/idata.h>

namespace mi {

/** \addtogroup mi_neuray_simple_types
@{
*/

/// A simple string class
class IString :
    public base::Interface_declare<0xe556a043,0xf99c,0x4804,0xa7,0xfd,0xa7,0x89,0x6a,0x07,0x9e,0x7a,
                                   IData_simple>
{
public:
    /// Returns the content as a C-style string.
    ///
    /// \return      The stored string as a C-style string. Never returns \c NULL.
    virtual const char* get_c_str() const = 0;

    /// Sets the content via a C-style string.
    ///
    /// \param str   The string to store as a C-style string. The value \c NULL is treated as the
    ///              empty string.
    virtual void set_c_str( const char* str) = 0;
};

/*@}*/ // end group mi_neuray_simple_types

} // namespace mi

#endif // MI_NEURAYLIB_ISTRING_H
