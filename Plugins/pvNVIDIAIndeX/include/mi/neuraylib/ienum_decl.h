/***************************************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief enum declarations

#ifndef MI_NEURAYLIB_IENUM_DECL_H
#define MI_NEURAYLIB_IENUM_DECL_H

#include <mi/neuraylib/idata.h>

namespace mi {

class IEnum_decl;

/** \addtogroup mi_neuray_types
@{
*/

/// An enum declaration is used to describe enum types.
///
/// An enum type is represented by a map from names (of type \c const \c char*) to values (of
/// type #mi::Sint32). Such a pair of name and value is also called enumerator. Note that different
/// enumerators can have the same value (but need to have different names).
///
/// \ifnot MDL_SDK_API
/// Enum declarations can be used to create new enum types. In this case, they are populated
/// through a sequence of #add_enumerator() calls. Finally, such a declaration is registered
/// via #mi::neuraylib::IExtension_api::register_enum_decl(). The name used for registration
/// can later be used as a type name to create instances of the type described by the declaration.
/// \endif
///
/// \see #mi::IEnum
class IEnum_decl :
    public base::Interface_declare<0xd15fcacd,0xe31f,0x4eb4,0x80,0x54,0x3b,0xe9,0x1f,0xbc,0x90,0x7d>
{
public:
    /// Adds a new enumerator to the enum declaration.
    ///
    /// \param name        The name of the new enumerator.
    /// \param value       The value of the new enumerator.
    /// \return
    ///                    -  0: Success.
    ///                    - -1: Invalid parameters (\c NULL pointer).
    ///                    - -2: There is already an enumerator with name \p name.
    virtual Sint32 add_enumerator( const char* name, Sint32 value) = 0;

    /// Removes an enumerator from the enum declaration.
    ///
    /// \param name        The name of the enumerator to remove.
    /// \return
    ///                    -  0: Success.
    ///                    - -1: Invalid parameters (\c NULL pointer).
    ///                    - -2: There is no enumerator with name \p name.
    virtual Sint32 remove_enumerator( const char* name) = 0;

    /// Returns the number of enumerators.
    virtual Size get_length() const = 0;

    /// Returns the name of an enumerator.
    ///
    /// \param index   The index of the requested enumerator.
    /// \return        The name of that enumerator, or \c NULL if \p index is out of bounds.
    virtual const char* get_name( Size index) const = 0;

    /// Returns the value of an enumerator.
    ///
    /// \param index   The index of the requested enumerator (invalid indices are treated as
    ///                index 0).
    /// \return        The value of that enumerator.
    virtual Sint32 get_value( Size index) const = 0;

    /// Returns the index for an enumerator identified by its name.
    ///
    /// \param name    The name of the requested enumerator.
    /// \return        The index of the enumerator with name \p name, or -1 if \p name is invalid.
    virtual Size get_index( const char* name) const = 0;

    /// Returns the index for an enumerator identified by its value.
    ///
    /// \param value   The value of the requested enumerator.
    /// \return        The smallest index of an enumerator with value \p value, or -1 if \p value
    ///                is invalid.
    virtual Size get_index( Sint32 value) const = 0;

    /// Returns the type name used to register this enum declaration.
    ///
    /// Note that the type name will only be available after registration, i.e., if the declaration
    /// has been obtained from #mi::IEnum::get_enum_decl() or
    /// #mi::neuraylib::IFactory::get_enum_decl().
    ///
    /// The type name might start with \c '{' which indicates that it has been automatically
    /// generated. In this case the type name should be treated as an opaque string since its format
    /// might change unexpectedly. It is perfectly fine to pass it to other methods, e.g.,
    /// #mi::neuraylib::IFactory::create(), but you should not attempt to interpret the value in
    /// any way. Use the methods on this interface to obtain information about the type itself.
    ///
    /// \return        The type name under which this enum declaration was registered, or
    ///                \c NULL in case of failure.
    virtual const char* get_enum_type_name() const = 0;
};

/*@}*/ // end group mi_neuray_types

} // namespace mi

#endif // MI_NEURAYLIB_IENUM_DECL_H
