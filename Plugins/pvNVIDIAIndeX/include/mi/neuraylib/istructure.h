/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Structure type.

#ifndef MI_NEURAYLIB_ISTRUCTURE_H
#define MI_NEURAYLIB_ISTRUCTURE_H

#include <mi/neuraylib/idata.h>

namespace mi {

class IStructure_decl;

/** \addtogroup mi_neuray_collections
@{
*/

/// This interface represents structures, i.e., a key-value based data structure.
///
/// Structures are based on a structure declaration which defines the structure members (their
/// types, name, and the order). The type name of a structure is the name that was used to
/// register its structure declaration. This type name can be used to create instances of a
/// particular structure declaration (note that \c "Structure" itself is not a valid type name as it
/// does not contain any information about a concrete structure type).
///
/// This interface does not offer any specialized methods, except #get_structure_decl(). All the
/// structure functionality is available via methods inherited from #mi::IData_collection where the
/// name of a structure member equals the key. The key indices correspond with the indices in the
/// structure declaration.
///
/// \note
///   The value returned by #mi::IData::get_type_name() might start with \c '{' which indicates that
///   it has been automatically generated. In this case the type name should be treated as an opaque
///   string since its format might change unexpectedly. It is perfectly fine to pass it to other
///   methods, e.g., #mi::neuraylib::IFactory::create(), but you should not attempt to interpret
///   the value in any way. Use #get_structure_decl() to obtain information about the type itself.
///
/// \see #mi::IStructure_decl
class IStructure :
    public base::Interface_declare<0xd23152f6,0x5640,0x4ea0,0x8c,0x59,0x27,0x3e,0xdf,0xab,0xd1,0x8e,
                                   IData_collection>
{
public:
    /// Returns the structure declaration for this structure.
    virtual const IStructure_decl* get_structure_decl() const = 0;
};

/*@}*/ // end group mi_neuray_collections

} // namespace mi

#endif // MI_NEURAYLIB_ISTRUCTURE_H
