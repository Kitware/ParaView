/***************************************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief UUID type.

#ifndef MI_NEURAYLIB_IUUID_H
#define MI_NEURAYLIB_IUUID_H

#include <mi/neuraylib/idata.h>

namespace mi {

/** \addtogroup mi_neuray_simple_types
@{
*/

/// This interface represents UUIDs.
///
/// \see #mi::base::Uuid.
class IUuid :
    public base::Interface_declare<0xc89e880b,0x78ff,0x40b7,0x9c,0xcf,0x0b,0x21,0x45,0xfe,0xe7,0x7b,
                                   IData_simple>
{
public:
    /// Sets the UUID.
    virtual void set_uuid( base::Uuid uuid) = 0;

    /// Returns the UUID.
    virtual base::Uuid get_uuid() const = 0;
};

/*@}*/ // end group mi_neuray_simple_types

} // namespace mi

#endif // MI_NEURAYLIB_IUUID_H
