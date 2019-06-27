/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Bounding box type.

#ifndef MI_NEURAYLIB_IBBOX_H
#define MI_NEURAYLIB_IBBOX_H

#include <mi/math/bbox.h>
#include <mi/neuraylib/icompound.h>
#include <mi/neuraylib/typedefs.h>

namespace mi {

/** \addtogroup mi_neuray_compounds
@{
*/

/// This interface represents bounding boxes.
///
/// It can be used to represent bounding boxes by an interface derived from #mi::base::IInterface.
///
/// \see #mi::Bbox3_struct
class IBbox3 :
    public base::Interface_declare<0x107953d0,0x70a0,0x48f5,0xb1,0x17,0x68,0x8e,0x7b,0xf8,0x85,0xa1,
                                   ICompound>
{
public:
    /// Returns the bounding box represented by this interface.
    virtual Bbox3_struct get_value() const = 0;

    /// Returns the bounding box represented by this interface.
    virtual void get_value( Bbox3_struct& value) const = 0;

    /// Sets the bounding box represented by this interface.
    virtual void set_value( const Bbox3_struct& value) = 0;

    /// Returns the bounding box represented by this interface.
    ///
    /// This inline method exists for the user's convenience since #mi::math::Bbox
    /// is not derived from #mi::math::Bbox_struct.
    inline void get_value( Bbox3& value) const {
        Bbox3_struct value_struct;
        get_value( value_struct);
        value = value_struct;
    }

    /// Sets the bounding box represented by this interface.
    ///
    /// This inline method exists for the user's convenience since #mi::math::Bbox
    /// is not derived from #mi::math::Bbox_struct.
    inline void set_value( const Bbox3& value) {
        Bbox3_struct value_struct = value;
        set_value( value_struct);
    }

    using ICompound::get_value;

    using ICompound::set_value;
};

/*@}*/ // end group mi_neuray_compounds

} // namespace mi

#endif // MI_NEURAYLIB_IBBOX_H
