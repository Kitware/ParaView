/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Vector types.

#ifndef MI_NEURAYLIB_IVECTOR_H
#define MI_NEURAYLIB_IVECTOR_H

#include <mi/math/vector.h>
#include <mi/neuraylib/icompound.h>
#include <mi/neuraylib/typedefs.h>

namespace mi
{

/** \addtogroup mi_neuray_compounds
@{
*/

/// This interface represents a vector of two bool.
///
/// \see #mi::Boolean_2_struct
class IBoolean_2 : public base::Interface_declare<0x237695a3, 0x8e73, 0x4d6b, 0x83, 0xd5, 0xd0,
                     0xfe, 0x46, 0x04, 0x35, 0x8b, ICompound>
{
public:
  /// Returns the vector represented by this interface.
  virtual Boolean_2_struct get_value() const = 0;

  /// Returns the vector represented by this interface.
  virtual void get_value(Boolean_2_struct& value) const = 0;

  /// Sets the vector represented by this interface.
  virtual void set_value(const Boolean_2_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a vector of three bool.
///
/// \see #mi::Boolean_3_struct
class IBoolean_3 : public base::Interface_declare<0x255bdb3b, 0xa22d, 0x4079, 0xb2, 0xcc, 0xb3,
                     0x4d, 0x4d, 0xe1, 0xeb, 0x0c, ICompound>
{
public:
  /// Returns the vector represented by this interface.
  virtual Boolean_3_struct get_value() const = 0;

  /// Returns the vector represented by this interface.
  virtual void get_value(Boolean_3_struct& value) const = 0;

  /// Sets the vector represented by this interface.
  virtual void set_value(const Boolean_3_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a vector of four bool.
///
/// \see #mi::Boolean_4_struct
class IBoolean_4 : public base::Interface_declare<0x2ae980c6, 0xab7c, 0x4d76, 0x9d, 0xdf, 0xa2,
                     0xc8, 0x0e, 0x01, 0xa5, 0xf9, ICompound>
{
public:
  /// Returns the vector represented by this interface.
  virtual Boolean_4_struct get_value() const = 0;

  /// Returns the vector represented by this interface.
  virtual void get_value(Boolean_4_struct& value) const = 0;

  /// Sets the vector represented by this interface.
  virtual void set_value(const Boolean_4_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a vector of two %Sint32.
///
/// \see #mi::Sint32_2_struct
class ISint32_2 : public base::Interface_declare<0x2c32de8d, 0xa2dd, 0x4236, 0x80, 0xef, 0x95, 0xea,
                    0xee, 0xc5, 0xa8, 0x4a, ICompound>
{
public:
  /// Returns the vector represented by this interface.
  virtual Sint32_2_struct get_value() const = 0;

  /// Returns the vector represented by this interface.
  virtual void get_value(Sint32_2_struct& value) const = 0;

  /// Sets the vector represented by this interface.
  virtual void set_value(const Sint32_2_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a vector of three %Sint32.
///
/// \see #mi::Sint32_3_struct
class ISint32_3 : public base::Interface_declare<0x3c778aa4, 0x0641, 0x4bea, 0xb2, 0x82, 0xe4, 0xae,
                    0x8f, 0xc0, 0x98, 0x16, ICompound>
{
public:
  /// Returns the vector represented by this interface.
  virtual Sint32_3_struct get_value() const = 0;

  /// Returns the vector represented by this interface.
  virtual void get_value(Sint32_3_struct& value) const = 0;

  /// Sets the vector represented by this interface.
  virtual void set_value(const Sint32_3_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a vector of four %Sint32.
///
/// \see #mi::Sint32_4_struct
class ISint32_4 : public base::Interface_declare<0x3e7dace9, 0x0295, 0x42db, 0x82, 0x17, 0x95, 0x62,
                    0x24, 0x6d, 0x09, 0xf9, ICompound>
{
public:
  /// Returns the vector represented by this interface.
  virtual Sint32_4_struct get_value() const = 0;

  /// Returns the vector represented by this interface.
  virtual void get_value(Sint32_4_struct& value) const = 0;

  /// Sets the vector represented by this interface.
  virtual void set_value(const Sint32_4_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a vector of two %Uint32.
///
/// \see #mi::Uint32_2_struct
class IUint32_2 : public base::Interface_declare<0x3ee8938e, 0x690f, 0x4932, 0x8a, 0xad, 0x54, 0x41,
                    0x46, 0xc2, 0x10, 0x5c, ICompound>
{
public:
  /// Returns the vector represented by this interface.
  virtual Uint32_2_struct get_value() const = 0;

  /// Returns the vector represented by this interface.
  virtual void get_value(Uint32_2_struct& value) const = 0;

  /// Sets the vector represented by this interface.
  virtual void set_value(const Uint32_2_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a vector of three %Uint32.
///
/// \see #mi::Uint32_3_struct
class IUint32_3 : public base::Interface_declare<0x3f559cde, 0xd898, 0x493a, 0x92, 0x5d, 0x52, 0x9e,
                    0xfa, 0x1f, 0xf7, 0xa9, ICompound>
{
public:
  /// Returns the vector represented by this interface.
  virtual Uint32_3_struct get_value() const = 0;

  /// Returns the vector represented by this interface.
  virtual void get_value(Uint32_3_struct& value) const = 0;

  /// Sets the vector represented by this interface.
  virtual void set_value(const Uint32_3_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a vector of four %Uint32.
///
/// \see #mi::Uint32_4_struct
class IUint32_4 : public base::Interface_declare<0x44ba66a0, 0x38ec, 0x4512, 0x90, 0x85, 0x6a, 0x1f,
                    0xdb, 0xdc, 0x81, 0x2b, ICompound>
{
public:
  /// Returns the vector represented by this interface.
  virtual Uint32_4_struct get_value() const = 0;

  /// Returns the vector represented by this interface.
  virtual void get_value(Uint32_4_struct& value) const = 0;

  /// Sets the vector represented by this interface.
  virtual void set_value(const Uint32_4_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a vector of two %Float32.
///
/// \see #mi::Float32_2_struct
class IFloat32_2 : public base::Interface_declare<0x452bc5ae, 0x1acf, 0x4e0b, 0x99, 0x6e, 0x93,
                     0xc6, 0x4f, 0xab, 0xc1, 0x5e, ICompound>
{
public:
  /// Returns the vector represented by this interface.
  virtual Float32_2_struct get_value() const = 0;

  /// Returns the vector represented by this interface.
  virtual void get_value(Float32_2_struct& value) const = 0;

  /// Sets the vector represented by this interface.
  virtual void set_value(const Float32_2_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a vector of three %Float32.
///
/// \see #mi::Float32_3_struct
class IFloat32_3 : public base::Interface_declare<0x4bebd304, 0x311a, 0x402b, 0x99, 0xae, 0x6d,
                     0x51, 0x42, 0x2c, 0x98, 0xc4, ICompound>
{
public:
  /// Returns the vector represented by this interface.
  virtual Float32_3_struct get_value() const = 0;

  /// Returns the vector represented by this interface.
  virtual void get_value(Float32_3_struct& value) const = 0;

  /// Sets the vector represented by this interface.
  virtual void set_value(const Float32_3_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a vector of four %Float32.
///
/// \see #mi::Float32_4_struct
class IFloat32_4 : public base::Interface_declare<0x525d7b84, 0x384d, 0x4a60, 0x9a, 0xf9, 0x9a,
                     0xa7, 0x33, 0xac, 0xb1, 0xdb, ICompound>
{
public:
  /// Returns the vector represented by this interface.
  virtual Float32_4_struct get_value() const = 0;

  /// Returns the vector represented by this interface.
  virtual void get_value(Float32_4_struct& value) const = 0;

  /// Sets the vector represented by this interface.
  virtual void set_value(const Float32_4_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a vector of two %Float64.
///
/// \see #mi::Float64_2_struct
class IFloat64_2 : public base::Interface_declare<0x53d8e9cc, 0x7156, 0x4805, 0x8c, 0xad, 0x88,
                     0x22, 0xcc, 0x42, 0x17, 0xce, ICompound>
{
public:
  /// Returns the vector represented by this interface.
  virtual Float64_2_struct get_value() const = 0;

  /// Returns the vector represented by this interface.
  virtual void get_value(Float64_2_struct& value) const = 0;

  /// Sets the vector represented by this interface.
  virtual void set_value(const Float64_2_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a vector of three %Float64.
///
/// \see #mi::Float64_3_struct
class IFloat64_3 : public base::Interface_declare<0x5acf22f8, 0x5834, 0x4608, 0x92, 0xc9, 0x91,
                     0x4e, 0x6b, 0x41, 0xf0, 0x06, ICompound>
{
public:
  /// Returns the vector represented by this interface.
  virtual Float64_3_struct get_value() const = 0;

  /// Returns the vector represented by this interface.
  virtual void get_value(Float64_3_struct& value) const = 0;

  /// Sets the vector represented by this interface.
  virtual void set_value(const Float64_3_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/// This interface represents a vector of four %Float64.
///
/// \see #mi::Float64_4_struct
class IFloat64_4 : public base::Interface_declare<0x5bd710b6, 0xdd62, 0x4915, 0x9c, 0x31, 0x28,
                     0x0c, 0x93, 0x46, 0x0d, 0x0b, ICompound>
{
public:
  /// Returns the vector represented by this interface.
  virtual Float64_4_struct get_value() const = 0;

  /// Returns the vector represented by this interface.
  virtual void get_value(Float64_4_struct& value) const = 0;

  /// Sets the vector represented by this interface.
  virtual void set_value(const Float64_4_struct& value) = 0;

  using ICompound::get_value;

  using ICompound::set_value;
};

/*@}*/ // end group mi_neuray_compounds

} // namespace mi

#endif // MI_NEURAYLIB_IVECTOR_H
