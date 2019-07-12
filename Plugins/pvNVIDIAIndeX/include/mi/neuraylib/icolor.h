/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Color type.

#ifndef MI_NEURAYLIB_ICOLOR_H
#define MI_NEURAYLIB_ICOLOR_H

#include <mi/math/color.h>
#include <mi/neuraylib/icompound.h>
#include <mi/neuraylib/typedefs.h>

namespace mi {

/** \addtogroup mi_neuray_compounds
@{
*/

/// This interface represents RGBA colors.
///
/// It can be used to represent colors by an interface derived from #mi::base::IInterface.
///
/// \see #mi::Color_struct
class IColor :
    public base::Interface_declare<0x10a52754,0xa1c7,0x454c,0x8a,0x0b,0x56,0xd4,0xd4,0xdc,0x62,0x18,
                                   ICompound>
{
public:
    /// Returns the color represented by this interface.
    virtual Color_struct get_value() const = 0;

    /// Returns the color represented by this interface.
    virtual void get_value( Color_struct& value) const = 0;

    /// Sets the color represented by this interface.
    virtual void set_value( const Color_struct& value) = 0;

    using ICompound::get_value;

    using ICompound::set_value;
};

/// This interface represents RGB colors.
///
/// It can be used to represent colors by an interface derived from #mi::base::IInterface.
///
/// \see #mi::Color_struct
class IColor3 :
    public base::Interface_declare<0x1189e839,0x6d86,0x4bac,0xbc,0x72,0xb0,0xc0,0x2d,0xa9,0x3c,0x6c,
                                   ICompound>
{
public:
    /// Returns the color represented by this interface.
    ///
    /// The alpha component of the return value is set to 1.0.
    virtual Color_struct get_value() const = 0;

    /// Returns the color represented by this interface.
    ///
    /// The alpha component of \p value is set to 1.0.
    virtual void get_value( Color_struct& value) const = 0;

    /// Sets the color represented by this interface.
    ///
    /// The alpha component of \p value is ignored.
    virtual void set_value( const Color_struct& value) = 0;

    using ICompound::get_value;

    using ICompound::set_value;
};

/*@}*/ // end group mi_neuray_compounds

} // namespace mi

#endif // MI_NEURAYLIB_ICOLOR_H
