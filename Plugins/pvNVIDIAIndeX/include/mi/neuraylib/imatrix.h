/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Matrix types.

#ifndef MI_NEURAYLIB_IMATRIX_H
#define MI_NEURAYLIB_IMATRIX_H

#include <mi/math/matrix.h>
#include <mi/neuraylib/icompound.h>
#include <mi/neuraylib/typedefs.h>

namespace mi
{

/** \addtogroup mi_neuray_compounds
@{
*/

/// This interface represents a 2 x 2 matrix of bool.
///
/// \see #mi::Boolean_2_2_struct
///
/// \see #mi::Boolean_2_2_struct
class IBoolean_2_2 : public base::Interface_declare<0x5d106447, 0xd197, 0x48f9, 0x83, 0xd8, 0x43,
                       0x7d, 0x08, 0x66, 0x09, 0x35, ICompound>

{
public:
  /// Returns the matrix represented by this interface.
  virtual Boolean_2_2_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Boolean_2_2_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Boolean_2_2_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 2 x 3 matrix of bool.
///
/// \see #mi::Boolean_2_3_struct
///
/// \see #mi::Boolean_2_3_struct
class IBoolean_2_3 : public base::Interface_declare<0x6145389f, 0x9baa, 0x4d87, 0x8e, 0xf2, 0x69,
                       0x9d, 0x0b, 0xd5, 0xaf, 0x8c, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Boolean_2_3_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Boolean_2_3_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Boolean_2_3_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 2 x 4 matrix of bool.
///
/// \see #mi::Boolean_2_4_struct
class IBoolean_2_4 : public base::Interface_declare<0x61d853dc, 0x6ba4, 0x46e6, 0x97, 0xd4, 0xcd,
                       0xdb, 0x25, 0xf0, 0xc7, 0xf6, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Boolean_2_4_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Boolean_2_4_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Boolean_2_4_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 3 x 2 matrix of bool.
///
/// \see #mi::Boolean_3_2_struct
class IBoolean_3_2 : public base::Interface_declare<0x630a979d, 0xdc70, 0x442a, 0x94, 0xb3, 0x47,
                       0x0b, 0xbe, 0x92, 0x92, 0xc8, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Boolean_3_2_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Boolean_3_2_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Boolean_3_2_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 3 x 3 matrix of bool.
///
/// \see #mi::Boolean_3_3_struct
class IBoolean_3_3 : public base::Interface_declare<0x69c4af0e, 0xe70f, 0x4435, 0xbd, 0x5d, 0xcf,
                       0x56, 0xdf, 0xf1, 0x96, 0xff, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Boolean_3_3_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Boolean_3_3_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Boolean_3_3_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 3 x 4 matrix of bool.
///
/// \see #mi::Boolean_3_4_struct
class IBoolean_3_4 : public base::Interface_declare<0x75f80041, 0x08c0, 0x42c0, 0x90, 0x34, 0xf6,
                       0x80, 0x4b, 0x05, 0x96, 0xa6, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Boolean_3_4_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Boolean_3_4_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Boolean_3_4_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 4 x 2 matrix of bool.
///
/// \see #mi::Boolean_4_2_struct
class IBoolean_4_2 : public base::Interface_declare<0x766c0535, 0xdf09, 0x4b6e, 0xb8, 0x1c, 0x09,
                       0x1c, 0xa5, 0xa9, 0xb2, 0x67, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Boolean_4_2_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Boolean_4_2_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Boolean_4_2_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 4 x 3 matrix of bool.
///
/// \see #mi::Boolean_4_3_struct
class IBoolean_4_3 : public base::Interface_declare<0x7ae72374, 0x8953, 0x4a40, 0x88, 0x80, 0x0e,
                       0x8c, 0x97, 0x51, 0x61, 0x11, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Boolean_4_3_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Boolean_4_3_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Boolean_4_3_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 4 x 4 matrix of bool.
///
/// \see #mi::Boolean_4_4_struct
class IBoolean_4_4 : public base::Interface_declare<0x7c94c35a, 0x1831, 0x4ae2, 0xa9, 0x16, 0x68,
                       0xf0, 0x29, 0x4c, 0xfd, 0xc8, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Boolean_4_4_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Boolean_4_4_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Boolean_4_4_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 2 x 2 matrix of %Sint32.
///
/// \see #mi::Sint32_2_2_struct
class ISint32_2_2 : public base::Interface_declare<0x8023e460, 0x8c07, 0x4d22, 0x95, 0xc6, 0x70,
                      0xb5, 0xa8, 0x2e, 0x58, 0x4a, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Sint32_2_2_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Sint32_2_2_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Sint32_2_2_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 2 x 3 matrix of %Sint32.
///
/// \see #mi::Sint32_2_3_struct
class ISint32_2_3 : public base::Interface_declare<0x810cf1e9, 0x6559, 0x40d1, 0xbf, 0xfe, 0xa5,
                      0xda, 0x9b, 0x40, 0xf5, 0xaf, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Sint32_2_3_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Sint32_2_3_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Sint32_2_3_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 2 x 4 matrix of %Sint32.
///
/// \see #mi::Sint32_2_4_struct
class ISint32_2_4 : public base::Interface_declare<0x88360736, 0x9177, 0x4f36, 0x80, 0x72, 0x7c,
                      0x12, 0x87, 0xf5, 0xbc, 0xab, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Sint32_2_4_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Sint32_2_4_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Sint32_2_4_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 3 x 2 matrix of %Sint32.
///
/// \see #mi::Sint32_3_2_struct
class ISint32_3_2 : public base::Interface_declare<0x896365dd, 0x4f16, 0x46e9, 0xac, 0xff, 0xb6,
                      0xe6, 0x03, 0x26, 0x77, 0xb7, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Sint32_3_2_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Sint32_3_2_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Sint32_3_2_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 3 x 3 matrix of %Sint32.
///
/// \see #mi::Sint32_3_3_struct
class ISint32_3_3 : public base::Interface_declare<0x896a5521, 0x3faa, 0x4ab3, 0xae, 0x18, 0xc8,
                      0x67, 0x23, 0xb0, 0x97, 0xc0, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Sint32_3_3_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Sint32_3_3_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Sint32_3_3_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 3 x 4 matrix of %Sint32.
///
/// \see #mi::Sint32_3_4_struct
class ISint32_3_4 : public base::Interface_declare<0x8c7cdbd2, 0xe910, 0x4805, 0x9e, 0x8c, 0x0a,
                      0xe7, 0x42, 0xad, 0x76, 0xca, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Sint32_3_4_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Sint32_3_4_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Sint32_3_4_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 4 x 2 matrix of %Sint32.
///
/// \see #mi::Sint32_4_2_struct
class ISint32_4_2 : public base::Interface_declare<0x8cc34e01, 0xa5d7, 0x48c2, 0x89, 0xeb, 0x34,
                      0x38, 0xf9, 0x22, 0xd8, 0x14, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Sint32_4_2_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Sint32_4_2_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Sint32_4_2_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 4 x 3 matrix of %Sint32.
///
/// \see #mi::Sint32_4_3_struct
class ISint32_4_3 : public base::Interface_declare<0x94cafc84, 0x28ae, 0x4d34, 0x90, 0x74, 0xdb,
                      0x6b, 0xf5, 0xc1, 0xe9, 0x89, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Sint32_4_3_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Sint32_4_3_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Sint32_4_3_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 4 x 4 matrix of %Sint32.
///
/// \see #mi::Sint32_4_4_struct
class ISint32_4_4 : public base::Interface_declare<0xa21d9b0f, 0x1247, 0x426f, 0xa3, 0x20, 0xd7,
                      0x36, 0x6d, 0xfc, 0x28, 0xc9, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Sint32_4_4_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Sint32_4_4_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Sint32_4_4_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 2 x 2 matrix of %Uint32.
///
/// \see #mi::Uint32_2_2_struct
class IUint32_2_2 : public base::Interface_declare<0xa87fd0c9, 0x3ada, 0x4c0b, 0xb3, 0x71, 0x36,
                      0x9c, 0xd7, 0x4a, 0x1f, 0xcf, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Uint32_2_2_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Uint32_2_2_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Uint32_2_2_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 2 x 3 matrix of %Uint32.
///
/// \see #mi::Uint32_2_3_struct
class IUint32_2_3 : public base::Interface_declare<0xac9458cf, 0x2502, 0x4279, 0x91, 0x83, 0xa6,
                      0x65, 0xe7, 0x8e, 0xcb, 0xca, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Uint32_2_3_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Uint32_2_3_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Uint32_2_3_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 2 x 4 matrix of %Uint32.
///
/// \see #mi::Uint32_2_4_struct
class IUint32_2_4 : public base::Interface_declare<0xad5cc27f, 0xec85, 0x4499, 0x89, 0x12, 0xcd,
                      0x6b, 0x0f, 0xf7, 0x22, 0x5f, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Uint32_2_4_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Uint32_2_4_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Uint32_2_4_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 3 x 2 matrix of %Uint32.
///
/// \see #mi::Uint32_3_2_struct
class IUint32_3_2 : public base::Interface_declare<0xadcfb745, 0xf396, 0x40bf, 0xab, 0x8b, 0x09,
                      0xaf, 0xb5, 0xe8, 0xc5, 0xd7, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Uint32_3_2_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Uint32_3_2_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Uint32_3_2_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 3 x 3 matrix of %Uint32.
///
/// \see #mi::Uint32_3_3_struct
class IUint32_3_3 : public base::Interface_declare<0xb2f0b878, 0xbb43, 0x4677, 0x87, 0x30, 0xad,
                      0x60, 0xe6, 0x17, 0x04, 0x9f, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Uint32_3_3_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Uint32_3_3_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Uint32_3_3_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 3 x 4 matrix of %Uint32.
///
/// \see #mi::Uint32_3_4_struct
class IUint32_3_4 : public base::Interface_declare<0xbc8a491c, 0x2c05, 0x4b03, 0x91, 0x5b, 0x84,
                      0x4d, 0x36, 0xe5, 0x1a, 0xe8, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Uint32_3_4_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Uint32_3_4_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Uint32_3_4_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 4 x 2 matrix of %Uint32.
///
/// \see #mi::Uint32_4_2_struct
class IUint32_4_2 : public base::Interface_declare<0xc35dd2a2, 0x11d1, 0x420e, 0x8b, 0xea, 0xbf,
                      0x4e, 0x82, 0x19, 0xc1, 0x0c, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Uint32_4_2_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Uint32_4_2_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Uint32_4_2_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 4 x 3 matrix of %Uint32.
///
/// \see #mi::Uint32_4_3_struct
class IUint32_4_3 : public base::Interface_declare<0xc379de7e, 0x4624, 0x41a5, 0xb5, 0x3c, 0x92,
                      0xf4, 0x8a, 0xdc, 0xfa, 0xa6, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Uint32_4_3_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Uint32_4_3_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Uint32_4_3_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 4 x 4 matrix of %Uint32.
///
/// \see #mi::Uint32_4_4_struct
class IUint32_4_4 : public base::Interface_declare<0xc5b8c13e, 0x2fb0, 0x48a5, 0x8c, 0x79, 0x04,
                      0xa4, 0x31, 0x27, 0x74, 0x73, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Uint32_4_4_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Uint32_4_4_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Uint32_4_4_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 2 x 2 matrix of %Float32.
///
/// \see #mi::Float32_2_2_struct
class IFloat32_2_2 : public base::Interface_declare<0xc7f2f4ed, 0x3f90, 0x4564, 0xa5, 0x42, 0xbd,
                       0x36, 0x01, 0xa6, 0x77, 0x0c, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Float32_2_2_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Float32_2_2_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Float32_2_2_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 2 x 3 matrix of %Float32.
///
/// \see #mi::Float32_2_3_struct
class IFloat32_2_3 : public base::Interface_declare<0xc845c505, 0xc345, 0x4bd0, 0x81, 0x50, 0x6b,
                       0x18, 0xd7, 0xc8, 0x3f, 0xa9, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Float32_2_3_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Float32_2_3_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Float32_2_3_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 2 x 4 matrix of %Float32.
///
/// \see #mi::Float32_2_4_struct
class IFloat32_2_4 : public base::Interface_declare<0xcaeac729, 0xea48, 0x4c9a, 0xa0, 0xda, 0xda,
                       0x6a, 0x36, 0xbe, 0x72, 0x64, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Float32_2_4_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Float32_2_4_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Float32_2_4_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 3 x 2 matrix of %Float32.
///
/// \see #mi::Float32_3_2_struct
class IFloat32_3_2 : public base::Interface_declare<0xd17f3d5a, 0x549f, 0x4823, 0x84, 0x70, 0xa5,
                       0x2d, 0xc4, 0x5d, 0xf4, 0xab, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Float32_3_2_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Float32_3_2_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Float32_3_2_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 3 x 3 matrix of %Float32.
///
/// \see #mi::Float32_3_3_struct
class IFloat32_3_3 : public base::Interface_declare<0xd1e53e9d, 0xcf1b, 0x438e, 0xa8, 0xcb, 0x87,
                       0x7c, 0x03, 0xa7, 0x66, 0xa3, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Float32_3_3_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Float32_3_3_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Float32_3_3_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 3 x 4 matrix of %Float32.
///
/// \see #mi::Float32_3_4_struct
class IFloat32_3_4 : public base::Interface_declare<0xd1ff55d2, 0x6c7b, 0x4421, 0xa1, 0x48, 0x82,
                       0x7d, 0x01, 0xce, 0xf5, 0x14, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Float32_3_4_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Float32_3_4_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Float32_3_4_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 4 x 2 matrix of %Float32.
///
/// \see #mi::Float32_4_2_struct
class IFloat32_4_2 : public base::Interface_declare<0xd202f3db, 0x4d0a, 0x4cd0, 0xa6, 0x88, 0xf2,
                       0xf2, 0x3e, 0xe3, 0x62, 0x4d, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Float32_4_2_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Float32_4_2_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Float32_4_2_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 4 x 3 matrix of %Float32.
///
/// \see #mi::Float32_4_3_struct
class IFloat32_4_3 : public base::Interface_declare<0xd571c16c, 0xb441, 0x4437, 0xaa, 0xfc, 0xe5,
                       0x1a, 0x2a, 0xbe, 0x35, 0xfe, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Float32_4_3_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Float32_4_3_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Float32_4_3_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 4 x 4 matrix of %Float32.
///
/// \see #mi::Float32_4_4_struct
class IFloat32_4_4 : public base::Interface_declare<0xd6c71e4f, 0xeb0e, 0x4efd, 0xb7, 0xfe, 0x48,
                       0x41, 0x2f, 0x65, 0x7c, 0x1a, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Float32_4_4_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Float32_4_4_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Float32_4_4_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 2 x 2 matrix of %Float64.
///
/// \see #mi::Float64_2_2_struct
class IFloat64_2_2 : public base::Interface_declare<0xd74ae71c, 0x13ca, 0x49b3, 0xa4, 0xdc, 0xb8,
                       0x4b, 0x33, 0x3e, 0x79, 0x63, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Float64_2_2_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Float64_2_2_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Float64_2_2_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 2 x 3 matrix of %Float64.
///
/// \see #mi::Float64_2_3_struct
class IFloat64_2_3 : public base::Interface_declare<0xdb03b6f9, 0x2e87, 0x4afa, 0x98, 0xc4, 0x00,
                       0xaa, 0xc3, 0x40, 0xc7, 0xc5, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Float64_2_3_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Float64_2_3_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Float64_2_3_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 2 x 4 matrix of %Float64.
///
/// \see #mi::Float64_2_4_struct
class IFloat64_2_4 : public base::Interface_declare<0xde9ff829, 0x045c, 0x427d, 0xb2, 0x27, 0x1c,
                       0xb9, 0x1e, 0x6c, 0x81, 0x5a, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Float64_2_4_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Float64_2_4_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Float64_2_4_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 3 x 2 matrix of %Float64.
///
/// \see #mi::Float64_3_2_struct
class IFloat64_3_2 : public base::Interface_declare<0xe554261e, 0x1aed, 0x44de, 0x88, 0xb7, 0x02,
                       0x26, 0x97, 0xde, 0xf6, 0x6b, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Float64_3_2_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Float64_3_2_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Float64_3_2_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 3 x 3 matrix of %Float64.
///
/// \see #mi::Float64_3_3_struct
class IFloat64_3_3 : public base::Interface_declare<0xe5ad29e9, 0x90d2, 0x4946, 0xbe, 0xe6, 0x99,
                       0x7d, 0x41, 0xe2, 0x4d, 0x45, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Float64_3_3_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Float64_3_3_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Float64_3_3_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 3 x 4 matrix of %Float64.
///
/// \see #mi::Float64_3_4_struct
class IFloat64_3_4 : public base::Interface_declare<0xe69208dc, 0x34a5, 0x4740, 0x99, 0x85, 0xad,
                       0x7f, 0x0a, 0xc3, 0xb5, 0xe5, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Float64_3_4_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Float64_3_4_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Float64_3_4_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 4 x 2 matrix of %Float64.
///
/// \see #mi::Float64_4_2_struct
class IFloat64_4_2 : public base::Interface_declare<0xe694e96d, 0x8920, 0x4057, 0xb1, 0xf0, 0xb1,
                       0x92, 0xa0, 0x92, 0xb8, 0x19, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Float64_4_2_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Float64_4_2_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Float64_4_2_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 4 x 3 matrix of %Float64.
///
/// \see #mi::Float64_4_3_struct
class IFloat64_4_3 : public base::Interface_declare<0xea2e5b27, 0x85ac, 0x46a6, 0xb3, 0xda, 0x76,
                       0x84, 0x08, 0xb0, 0x28, 0x3d, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Float64_4_3_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Float64_4_3_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Float64_4_3_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a 4 x 4 matrix of %Float64.
///
/// \see #mi::Float64_4_4_struct
class IFloat64_4_4 : public base::Interface_declare<0xeea73757, 0x48e6, 0x4168, 0x9c, 0x97, 0x81,
                       0x82, 0x52, 0x1f, 0x79, 0xe0, ICompound>
{
public:
  /// Returns the matrix represented by this interface.
  virtual Float64_4_4_struct get_value() const = 0;

  /// Returns the matrix represented by this interface.
  virtual void get_value(Float64_4_4_struct& value) const = 0;

  /// Sets the matrix represented by this interface.
  virtual void set_value(const Float64_4_4_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/*@}*/ // end group mi_neuray_compounds

} // namespace mi

#endif // MI_NEURAYLIB_IMATRIX_H
