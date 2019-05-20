/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief structure declarations

#ifndef MI_NEURAYLIB_ISTRUCTURE_DECL_H
#define MI_NEURAYLIB_ISTRUCTURE_DECL_H

#include <mi/neuraylib/idata.h>

namespace mi
{

/** \addtogroup mi_neuray_types
@{
*/

/// A structure declaration is used to describe structure types.
///
/// It contains all the type information required for structure types: the number of fields,
/// their names and types. In contrast to element of a map structure fields are ordered and do
/// not need to be of the same type.
///
/// \ifnot MDL_SDK_API
/// Structure declarations can be used to create new structure types. In this case, they are
/// populated through a sequence of #add_member() calls. Finally, such a declaration is registered
/// via #mi::neuraylib::IExtension_api::register_structure_decl(). The name used for registration
/// can later be used as a type name to create instances of the type described by the declaration.
/// \endif
///
/// \see #mi::IStructure, #mi::IMap
class IStructure_decl : public base::Interface_declare<0xcd206d33, 0x0906, 0x4e70, 0x82, 0x42, 0x6a,
                          0x90, 0x8a, 0xf5, 0x82, 0x43>
{
public:
  /// Adds a new member to the structure declaration.
  ///
  /// \param type_name   The type name of the new member, see \ref mi_neuray_types for valid type
  ///                    names. Note that this method does not check the type name for validity
  ///                    (since validity at the time of this method call does not imply validity
  ///                    at the time of instantiation).
  /// \param name        The name of the new member.
  /// \return
  ///                    -  0: Success.
  ///                    - -1: Invalid parameters (\c NULL pointer).
  ///                    - -2: There is already a member with name \p name.
  virtual Sint32 add_member(const char* type_name, const char* name) = 0;

  /// Removes a member from the structure declaration.
  ///
  /// \param name        The name of the member to remove.
  /// \return
  ///                    -  0: Success.
  ///                    - -1: Invalid parameters (\c NULL pointer).
  ///                    - -2: There is no member with name \p name.
  virtual Sint32 remove_member(const char* name) = 0;

  /// Returns the number of structure members.
  virtual Size get_length() const = 0;

  /// Returns the type name of a certain structure member.
  ///
  /// \param index   The index of the requested structure member.
  /// \return        The type name of that structure member, or \c NULL if \p index is out of
  ///                bounds.
  virtual const char* get_member_type_name(Size index) const = 0;

  /// Returns the type name of a certain structure member.
  ///
  /// \param name    The name of the requested structure member.
  /// \return        The type name of that structure member, or \c NULL if there is no structure
  ///                member with name \p name.
  virtual const char* get_member_type_name(const char* name) const = 0;

  /// Returns the member name of a certain structure member.
  ///
  /// \param index   The index of the requested structure member.
  /// \return        The member name of that structure member, or \c NULL if \p index is out of
  ///                bounds.
  virtual const char* get_member_name(Size index) const = 0;

  /// Returns the type name used to register this structure declaration.
  ///
  /// Note that the type name will only be available after registration, i.e., if the declaration
  /// has been obtained from #mi::IStructure::get_structure_decl() or
  /// #mi::neuraylib::IFactory::get_structure_decl().
  ///
  /// The type name might start with \c '{' which indicates that it has been automatically
  /// generated. In this case the type name should be treated as an opaque string since its format
  /// might change unexpectedly. It is perfectly fine to pass it to other methods, e.g.,
  /// #mi::neuraylib::IFactory::create(), but you should not attempt to interpret the value in
  /// any way. Use the methods on this interface to obtain information about the type itself.
  ///
  /// \return        The type name under which this structure declaration was registered, or
  ///                \c NULL in case of failure.
  virtual const char* get_structure_type_name() const = 0;
};

/*@}*/ // end group mi_neuray_types

} // namespace mi

#endif // MI_NEURAYLIB_ISTRUCTURE_DECL_H
