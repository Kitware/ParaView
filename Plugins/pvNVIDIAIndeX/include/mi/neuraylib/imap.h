/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Map type.

#ifndef MI_NEURAYLIB_IMAP_H
#define MI_NEURAYLIB_IMAP_H

#include <mi/neuraylib/idata.h>

namespace mi
{

namespace neuraylib
{
class IFactory;
}

/** \addtogroup mi_neuray_collections
@{
*/

/// This interface represents maps, i.e., a key-value based data structure.
///
/// Maps are either typed or untyped. Keys are always strings. Typed maps enforce all values to be
/// of the same type. The values of typed maps have to be derived from #mi::IData. The type name
/// of a typed map is \c "Map<", followed by the type name the values, and finally \c ">", e.g.,
/// "Map<Sint32>" for a map with values of type #mi::Sint32. Initially, a map is empty, i.e.,
/// no keys exist.
///
/// Untyped maps simply store pointers of type #mi::base::IInterface. The type name of an untyped
/// map is \c "Map<Interface>".
///
/// Most methods of #mi::IData_collection come in two versions, an index-based and a key-based
/// version. Since a map is a key-based data structure, the key-based variants of the methods of
/// #mi::IData_collection are more efficient (constant time). In general, the index-based methods
/// require linear time to compute the key for a given index (in particular for random access
/// patterns). As an exception, accessing the indices in sequence from 0 to #get_length()-1
/// requires only constant time per access (provided the structure of the map is not changed,
/// the values of the keys may change). The mapping of indices to keys is unspecified.
class IMap : public base::Interface_declare<0xca097e3a, 0x2621, 0x41e7, 0x80, 0xa3, 0x97, 0x2f,
               0x0d, 0x56, 0xf8, 0x47, IData_collection>
{
public:
  /// Returns the size of the map.
  ///
  /// The size of a map is the number of keys in the map.
  virtual Size get_length() const = 0;

  /// Checks whether the map is empty.
  ///
  /// Equivalent to #get_length() == 0.
  virtual bool empty() const = 0;

  /// Removes all keys and their associated values from the map.
  virtual void clear() = 0;

  /// Inserts a new key including its value into the map.
  ///
  /// \return
  ///                -  0: Success.
  ///                - -1: Invalid parameters (\c NULL pointer).
  ///                - -2: \p key exists already.
  ///                - -3: \p value has the wrong type.
  virtual Sint32 insert(const char* key, base::IInterface* value) = 0;

  /// Removes a key and its value from the map.
  ///
  /// \return
  ///                -  0: Success.
  ///                - -1: Invalid parameters (\c NULL pointer).
  ///                - -2: \p key does not exist
  virtual Sint32 erase(const char* key) = 0;
};

/*@}*/ // end group mi_neuray_collections

} // namespace mi

#endif // MI_NEURAYLIB_IMAP_H
