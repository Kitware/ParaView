/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Generic buffer interface

#ifndef MI_NEURAYLIB_IBUFFER_H
#define MI_NEURAYLIB_IBUFFER_H

#include <mi/base/interface_declare.h>

namespace mi
{

namespace neuraylib
{

/** \addtogroup mi_neuray_types
@{
*/

/// Abstract interface for a simple buffer with binary data.
///
/// The length of the buffer is given by #get_data_size(). The data is \em not \c '\\0'-terminated.
class IBuffer : public mi::base::Interface_declare<0xfb925baf, 0x1e38, 0x461b, 0x8e, 0xcd, 0x65,
                  0xa3, 0xf5, 0x20, 0xe5, 0x92>
{
public:
  /// Returns a pointer to the data represented by the buffer.
  virtual const Uint8* get_data() const = 0;

  /// Returns the size of the buffer.
  virtual Size get_data_size() const = 0;
};

/*@}*/ // end group mi_neuray_types

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IBUFFER_H
