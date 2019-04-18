/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Type traits

#ifndef MI_NEURAYLIB_TYPE_TRAITS_H
#define MI_NEURAYLIB_TYPE_TRAITS_H

#include <mi/base/interface_declare.h>
#include <mi/base/uuid.h>
#include <mi/math/bbox.h>
#include <mi/math/color.h>
#include <mi/math/matrix.h>
#include <mi/math/spectrum.h>
#include <mi/math/vector.h>

namespace mi
{

/** \addtogroup mi_neuray_types
@{
*/

class IData;
class IBoolean;
class ISint8;
class ISint16;
class ISint32;
class ISint64;
class IUint8;
class IUint16;
class IUint32;
class IUint64;
class IFloat32;
class IFloat64;
class ISize;
class IDifference;
class IString;
class IUuid;
class IVoid;
class IRef;
class IBoolean_2;
class IBoolean_3;
class IBoolean_4;
class ISint32_2;
class ISint32_3;
class ISint32_4;
class IUint32_2;
class IUint32_3;
class IUint32_4;
class IFloat32_2;
class IFloat32_3;
class IFloat32_4;
class IFloat64_2;
class IFloat64_3;
class IFloat64_4;
class IBoolean_2_2;
class IBoolean_2_3;
class IBoolean_2_4;
class IBoolean_3_2;
class IBoolean_3_3;
class IBoolean_3_4;
class IBoolean_4_2;
class IBoolean_4_3;
class IBoolean_4_4;
class ISint32_2_2;
class ISint32_2_3;
class ISint32_2_4;
class ISint32_3_2;
class ISint32_3_3;
class ISint32_3_4;
class ISint32_4_2;
class ISint32_4_3;
class ISint32_4_4;
class IUint32_2_2;
class IUint32_2_3;
class IUint32_2_4;
class IUint32_3_2;
class IUint32_3_3;
class IUint32_3_4;
class IUint32_4_2;
class IUint32_4_3;
class IUint32_4_4;
class IFloat32_2_2;
class IFloat32_2_3;
class IFloat32_2_4;
class IFloat32_3_2;
class IFloat32_3_3;
class IFloat32_3_4;
class IFloat32_4_2;
class IFloat32_4_3;
class IFloat32_4_4;
class IFloat64_2_2;
class IFloat64_2_3;
class IFloat64_2_4;
class IFloat64_3_2;
class IFloat64_3_3;
class IFloat64_3_4;
class IFloat64_4_2;
class IFloat64_4_3;
class IFloat64_4_4;
class IColor;
class IColor3;
class ISpectrum;
class IBbox3;

/// Type traits relating interfaces, corresponding primitive types, and their type names.
///
/// These type traits are defined for two types of template arguments, interfaces and primitive
/// types.
///
/// If the template parameter is an interface derived from #mi::IData:
/// - The static member function \c get_type_name() returns the type name corresponding to the
///   template parameter, e.g., \c "Sint32" for the template parameter #mi::ISint32.
/// - The member typedef \c Primitive_type is defined to the corresponding primitive type, e.g.,
///   #mi::Sint32 for the template parameter #mi::ISint32.
/// - Not supported are #mi::IEnum, #mi::IPointer, #mi::IConst_pointer, interfaces derived from
///   #mi::IData_collection with the exception of compounds, and all non-leaf interfaces, e.g.,
///   #mi::INumber.
///
/// Note the following special case for the mapping between primitive types:
/// - For both interfaces #mi::IColor and #mi::IColor3 the primitive type is #mi::math::Color.
/// - For both interfaces #mi::IString and #mi::IRef the primitive type is \c const \c char*.
/// - For the interface #mi::ISize the primitive type is #mi::Size which is the same as #mi::Uint64.
/// - For the interface #mi::IDifference the primitive type is #mi::Difference which the same as
///   #mi::Sint64.
///
/// Example:
/// \code
/// template<> struct Type_traits<mi::ISint32>
/// {
///     static const char* get_type_name() { return "Sint32"; }
///     typedef mi::Sint32 Primitive_type;
/// };
/// \endcode
///
/// If the template parameter is a primitive type:
/// - The static member function \c get_type_name() returns the type name corresponding to the
///   template parameter, e.g., \c "Sint32" for the template parameter #mi::Sint32.
/// - The member typedef \c Interface_type is defined to the corresponding interface type, e.g.,
///   #mi::ISint32 for the template parameter #mi::Sint32.
///
/// Since the set of primitive types is smaller than the set of interfaces the following interfaces
/// do not appear as typedef \c Interface_type: #mi::IColor3, #mi::IRef, #mi::ISize, and
/// #mi::IDifference (plus all interfaces which are not supported as template parameter, see above).
/// In particular, a string value does not provide enough to context to decide whether it represents
/// just a string (#mi::IString) or references a DB element (#mi::IRef).
///
/// Example:
/// \code
/// template<> struct Type_traits<mi::Sint32>
/// {
///     static const char* get_type_name() { return "Sint32"; };
///     typedef mi::ISint32 Interface_type;
/// };
/// \endcode
///
/// \note Note that \c const \c char* is the only primitive type that does not have value semantics,
///       but pointer semantics.
template <typename I>
struct Type_traits
{
};

template <>
struct Type_traits<mi::IBoolean>
{
  static const char* get_type_name() { return "Boolean"; }
  typedef bool Primitive_type;
};

template <>
struct Type_traits<mi::ISint8>
{
  static const char* get_type_name() { return "Sint8"; }
  typedef mi::Sint8 Primitive_type;
};

template <>
struct Type_traits<mi::ISint16>
{
  static const char* get_type_name() { return "Sint16"; }
  typedef mi::Sint16 Primitive_type;
};

template <>
struct Type_traits<mi::ISint32>
{
  static const char* get_type_name() { return "Sint32"; }
  typedef mi::Sint32 Primitive_type;
};

template <>
struct Type_traits<mi::ISint64>
{
  static const char* get_type_name() { return "Sint64"; }
  typedef mi::Sint64 Primitive_type;
};

template <>
struct Type_traits<mi::IUint8>
{
  static const char* get_type_name() { return "Uint8"; }
  typedef mi::Uint8 Primitive_type;
};

template <>
struct Type_traits<mi::IUint16>
{
  static const char* get_type_name() { return "Uint16"; }
  typedef mi::Uint16 Primitive_type;
};

template <>
struct Type_traits<mi::IUint32>
{
  static const char* get_type_name() { return "Uint32"; }
  typedef mi::Uint32 Primitive_type;
};

template <>
struct Type_traits<mi::IUint64>
{
  static const char* get_type_name() { return "Uint64"; }
  typedef mi::Uint64 Primitive_type;
};

template <>
struct Type_traits<mi::IFloat32>
{
  static const char* get_type_name() { return "Float32"; }
  typedef mi::Float32 Primitive_type;
};

template <>
struct Type_traits<mi::IFloat64>
{
  static const char* get_type_name() { return "Float64"; }
  typedef mi::Float64 Primitive_type;
};

template <>
struct Type_traits<mi::ISize>
{
  static const char* get_type_name() { return "Size"; }
  typedef mi::Size Primitive_type;
};

template <>
struct Type_traits<mi::IDifference>
{
  static const char* get_type_name() { return "Difference"; }
  typedef mi::Difference Primitive_type;
};

template <>
struct Type_traits<mi::IString>
{
  static const char* get_type_name() { return "String"; }
  typedef const char* Primitive_type;
};

template <>
struct Type_traits<mi::IUuid>
{
  static const char* get_type_name() { return "Uuid"; }
  typedef mi::base::Uuid Primitive_type;
};

template <>
struct Type_traits<mi::IVoid>
{
  static const char* get_type_name() { return "Void"; }
  typedef void Primitive_type;
};

template <>
struct Type_traits<mi::IRef>
{
  static const char* get_type_name() { return "Ref"; }
  typedef const char* Primitive_type;
};

template <>
struct Type_traits<mi::IBoolean_2>
{
  static const char* get_type_name() { return "Boolean<2>"; }
  typedef mi::math::Vector<bool, 2> Primitive_type;
};

template <>
struct Type_traits<mi::IBoolean_3>
{
  static const char* get_type_name() { return "Boolean<3>"; }
  typedef mi::math::Vector<bool, 3> Primitive_type;
};

template <>
struct Type_traits<mi::IBoolean_4>
{
  static const char* get_type_name() { return "Boolean<4>"; }
  typedef mi::math::Vector<bool, 4> Primitive_type;
};

template <>
struct Type_traits<mi::ISint32_2>
{
  static const char* get_type_name() { return "Sint32<2>"; }
  typedef mi::math::Vector<mi::Sint32, 2> Primitive_type;
};

template <>
struct Type_traits<mi::ISint32_3>
{
  static const char* get_type_name() { return "Sint32<3>"; }
  typedef mi::math::Vector<mi::Sint32, 3> Primitive_type;
};

template <>
struct Type_traits<mi::ISint32_4>
{
  static const char* get_type_name() { return "Sint32<4>"; }
  typedef mi::math::Vector<mi::Sint32, 4> Primitive_type;
};

template <>
struct Type_traits<mi::IUint32_2>
{
  static const char* get_type_name() { return "Uint32<2>"; }
  typedef mi::math::Vector<mi::Uint32, 2> Primitive_type;
};

template <>
struct Type_traits<mi::IUint32_3>
{
  static const char* get_type_name() { return "Uint32<3>"; }
  typedef mi::math::Vector<mi::Uint32, 3> Primitive_type;
};

template <>
struct Type_traits<mi::IUint32_4>
{
  static const char* get_type_name() { return "Uint32<4>"; }
  typedef mi::math::Vector<mi::Uint32, 4> Primitive_type;
};

template <>
struct Type_traits<mi::IFloat32_2>
{
  static const char* get_type_name() { return "Float32<2>"; }
  typedef mi::math::Vector<mi::Float32, 2> Primitive_type;
};

template <>
struct Type_traits<mi::IFloat32_3>
{
  static const char* get_type_name() { return "Float32<3>"; }
  typedef mi::math::Vector<mi::Float32, 3> Primitive_type;
};

template <>
struct Type_traits<mi::IFloat32_4>
{
  static const char* get_type_name() { return "Float32<4>"; }
  typedef mi::math::Vector<mi::Float32, 4> Primitive_type;
};

template <>
struct Type_traits<mi::IFloat64_2>
{
  static const char* get_type_name() { return "Float64<2>"; }
  typedef mi::math::Vector<mi::Float64, 2> Primitive_type;
};

template <>
struct Type_traits<mi::IFloat64_3>
{
  static const char* get_type_name() { return "Float64<3>"; }
  typedef mi::math::Vector<mi::Float64, 3> Primitive_type;
};

template <>
struct Type_traits<mi::IFloat64_4>
{
  static const char* get_type_name() { return "Float64<4>"; }
  typedef mi::math::Vector<mi::Float64, 4> Primitive_type;
};

template <>
struct Type_traits<mi::IBoolean_2_2>
{
  static const char* get_type_name() { return "Boolean<2,2>"; }
  typedef mi::math::Matrix<bool, 2, 2> Primitive_type;
};

template <>
struct Type_traits<mi::IBoolean_2_3>
{
  static const char* get_type_name() { return "Boolean<2,3>"; }
  typedef mi::math::Matrix<bool, 2, 3> Primitive_type;
};

template <>
struct Type_traits<mi::IBoolean_2_4>
{
  static const char* get_type_name() { return "Boolean<2,4>"; }
  typedef mi::math::Matrix<bool, 2, 4> Primitive_type;
};

template <>
struct Type_traits<mi::IBoolean_3_2>
{
  static const char* get_type_name() { return "Boolean<3,2>"; }
  typedef mi::math::Matrix<bool, 3, 2> Primitive_type;
};

template <>
struct Type_traits<mi::IBoolean_3_3>
{
  static const char* get_type_name() { return "Boolean<3,3>"; }
  typedef mi::math::Matrix<bool, 3, 3> Primitive_type;
};

template <>
struct Type_traits<mi::IBoolean_3_4>
{
  static const char* get_type_name() { return "Boolean<3,4>"; }
  typedef mi::math::Matrix<bool, 3, 4> Primitive_type;
};

template <>
struct Type_traits<mi::IBoolean_4_2>
{
  static const char* get_type_name() { return "Boolean<4,2>"; }
  typedef mi::math::Matrix<bool, 4, 2> Primitive_type;
};

template <>
struct Type_traits<mi::IBoolean_4_3>
{
  static const char* get_type_name() { return "Boolean<4,3>"; }
  typedef mi::math::Matrix<bool, 4, 3> Primitive_type;
};

template <>
struct Type_traits<mi::IBoolean_4_4>
{
  static const char* get_type_name() { return "Boolean<4,4>"; }
  typedef mi::math::Matrix<bool, 4, 4> Primitive_type;
};

template <>
struct Type_traits<mi::ISint32_2_2>
{
  static const char* get_type_name() { return "Sint32<2,2>"; }
  typedef mi::math::Matrix<mi::Sint32, 2, 2> Primitive_type;
};

template <>
struct Type_traits<mi::ISint32_2_3>
{
  static const char* get_type_name() { return "Sint32<2,3>"; }
  typedef mi::math::Matrix<mi::Sint32, 2, 3> Primitive_type;
};

template <>
struct Type_traits<mi::ISint32_2_4>
{
  static const char* get_type_name() { return "Sint32<2,4>"; }
  typedef mi::math::Matrix<mi::Sint32, 2, 4> Primitive_type;
};

template <>
struct Type_traits<mi::ISint32_3_2>
{
  static const char* get_type_name() { return "Sint32<3,2>"; }
  typedef mi::math::Matrix<mi::Sint32, 3, 2> Primitive_type;
};

template <>
struct Type_traits<mi::ISint32_3_3>
{
  static const char* get_type_name() { return "Sint32<3,3>"; }
  typedef mi::math::Matrix<mi::Sint32, 3, 3> Primitive_type;
};

template <>
struct Type_traits<mi::ISint32_3_4>
{
  static const char* get_type_name() { return "Sint32<3,4>"; }
  typedef mi::math::Matrix<mi::Sint32, 3, 4> Primitive_type;
};

template <>
struct Type_traits<mi::ISint32_4_2>
{
  static const char* get_type_name() { return "Sint32<4,2>"; }
  typedef mi::math::Matrix<mi::Sint32, 4, 2> Primitive_type;
};

template <>
struct Type_traits<mi::ISint32_4_3>
{
  static const char* get_type_name() { return "Sint32<4,3>"; }
  typedef mi::math::Matrix<mi::Sint32, 4, 3> Primitive_type;
};

template <>
struct Type_traits<mi::ISint32_4_4>
{
  static const char* get_type_name() { return "Sint32<4,4>"; }
  typedef mi::math::Matrix<mi::Sint32, 4, 4> Primitive_type;
};

template <>
struct Type_traits<mi::IUint32_2_2>
{
  static const char* get_type_name() { return "Uint32<2,2>"; }
  typedef mi::math::Matrix<mi::Uint32, 2, 2> Primitive_type;
};

template <>
struct Type_traits<mi::IUint32_2_3>
{
  static const char* get_type_name() { return "Uint32<2,3>"; }
  typedef mi::math::Matrix<mi::Uint32, 2, 3> Primitive_type;
};

template <>
struct Type_traits<mi::IUint32_2_4>
{
  static const char* get_type_name() { return "Uint32<2,4>"; }
  typedef mi::math::Matrix<mi::Uint32, 2, 4> Primitive_type;
};

template <>
struct Type_traits<mi::IUint32_3_2>
{
  static const char* get_type_name() { return "Uint32<3,2>"; }
  typedef mi::math::Matrix<mi::Uint32, 3, 2> Primitive_type;
};

template <>
struct Type_traits<mi::IUint32_3_3>
{
  static const char* get_type_name() { return "Uint32<3,3>"; }
  typedef mi::math::Matrix<mi::Uint32, 3, 3> Primitive_type;
};

template <>
struct Type_traits<mi::IUint32_3_4>
{
  static const char* get_type_name() { return "Uint32<3,4>"; }
  typedef mi::math::Matrix<mi::Uint32, 3, 4> Primitive_type;
};

template <>
struct Type_traits<mi::IUint32_4_2>
{
  static const char* get_type_name() { return "Uint32<4,2>"; }
  typedef mi::math::Matrix<mi::Uint32, 4, 2> Primitive_type;
};

template <>
struct Type_traits<mi::IUint32_4_3>
{
  static const char* get_type_name() { return "Uint32<4,3>"; }
  typedef mi::math::Matrix<mi::Uint32, 4, 3> Primitive_type;
};

template <>
struct Type_traits<mi::IUint32_4_4>
{
  static const char* get_type_name() { return "Uint32<4,4>"; }
  typedef mi::math::Matrix<mi::Uint32, 4, 4> Primitive_type;
};

template <>
struct Type_traits<mi::IFloat32_2_2>
{
  static const char* get_type_name() { return "Float32<2,2>"; }
  typedef mi::math::Matrix<mi::Float32, 2, 2> Primitive_type;
};

template <>
struct Type_traits<mi::IFloat32_2_3>
{
  static const char* get_type_name() { return "Float32<2,3>"; }
  typedef mi::math::Matrix<mi::Float32, 2, 3> Primitive_type;
};

template <>
struct Type_traits<mi::IFloat32_2_4>
{
  static const char* get_type_name() { return "Float32<2,4>"; }
  typedef mi::math::Matrix<mi::Float32, 2, 4> Primitive_type;
};

template <>
struct Type_traits<mi::IFloat32_3_2>
{
  static const char* get_type_name() { return "Float32<3,2>"; }
  typedef mi::math::Matrix<mi::Float32, 3, 2> Primitive_type;
};

template <>
struct Type_traits<mi::IFloat32_3_3>
{
  static const char* get_type_name() { return "Float32<3,3>"; }
  typedef mi::math::Matrix<mi::Float32, 3, 3> Primitive_type;
};

template <>
struct Type_traits<mi::IFloat32_3_4>
{
  static const char* get_type_name() { return "Float32<3,4>"; }
  typedef mi::math::Matrix<mi::Float32, 3, 4> Primitive_type;
};

template <>
struct Type_traits<mi::IFloat32_4_2>
{
  static const char* get_type_name() { return "Float32<4,2>"; }
  typedef mi::math::Matrix<mi::Float32, 4, 2> Primitive_type;
};

template <>
struct Type_traits<mi::IFloat32_4_3>
{
  static const char* get_type_name() { return "Float32<4,3>"; }
  typedef mi::math::Matrix<mi::Float32, 4, 3> Primitive_type;
};

template <>
struct Type_traits<mi::IFloat32_4_4>
{
  static const char* get_type_name() { return "Float32<4,4>"; }
  typedef mi::math::Matrix<mi::Float32, 4, 4> Primitive_type;
};

template <>
struct Type_traits<mi::IFloat64_2_2>
{
  static const char* get_type_name() { return "Float64<2,2>"; }
  typedef mi::math::Matrix<mi::Float64, 2, 2> Primitive_type;
};

template <>
struct Type_traits<mi::IFloat64_2_3>
{
  static const char* get_type_name() { return "Float64<2,3>"; }
  typedef mi::math::Matrix<mi::Float64, 2, 3> Primitive_type;
};

template <>
struct Type_traits<mi::IFloat64_2_4>
{
  static const char* get_type_name() { return "Float64<2,4>"; }
  typedef mi::math::Matrix<mi::Float64, 2, 4> Primitive_type;
};

template <>
struct Type_traits<mi::IFloat64_3_2>
{
  static const char* get_type_name() { return "Float64<3,2>"; }
  typedef mi::math::Matrix<mi::Float64, 3, 2> Primitive_type;
};

template <>
struct Type_traits<mi::IFloat64_3_3>
{
  static const char* get_type_name() { return "Float64<3,3>"; }
  typedef mi::math::Matrix<mi::Float64, 3, 3> Primitive_type;
};

template <>
struct Type_traits<mi::IFloat64_3_4>
{
  static const char* get_type_name() { return "Float64<3,4>"; }
  typedef mi::math::Matrix<mi::Float64, 3, 4> Primitive_type;
};

template <>
struct Type_traits<mi::IFloat64_4_2>
{
  static const char* get_type_name() { return "Float64<4,2>"; }
  typedef mi::math::Matrix<mi::Float64, 4, 2> Primitive_type;
};

template <>
struct Type_traits<mi::IFloat64_4_3>
{
  static const char* get_type_name() { return "Float64<4,3>"; }
  typedef mi::math::Matrix<mi::Float64, 4, 3> Primitive_type;
};

template <>
struct Type_traits<mi::IFloat64_4_4>
{
  static const char* get_type_name() { return "Float64<4,4>"; }
  typedef mi::math::Matrix<mi::Float64, 4, 4> Primitive_type;
};

template <>
struct Type_traits<mi::IColor>
{
  static const char* get_type_name() { return "Color"; }
  typedef mi::math::Color Primitive_type;
};

template <>
struct Type_traits<mi::IColor3>
{
  static const char* get_type_name() { return "Color3"; }
  typedef mi::math::Color Primitive_type;
};

template <>
struct Type_traits<mi::ISpectrum>
{
  static const char* get_type_name() { return "Spectrum"; }
  typedef mi::math::Spectrum Primitive_type;
};

template <>
struct Type_traits<mi::IBbox3>
{
  static const char* get_type_name() { return "Bbox3"; }
  typedef mi::math::Bbox<mi::Float32, 3> Primitive_type;
};

template <>
struct Type_traits<bool>
{
  static const char* get_type_name() { return "Boolean"; };
  typedef mi::IBoolean Interface_type;
};

template <>
struct Type_traits<mi::Sint8>
{
  static const char* get_type_name() { return "Sint8"; };
  typedef mi::ISint8 Interface_type;
};

template <>
struct Type_traits<mi::Sint16>
{
  static const char* get_type_name() { return "Sint16"; };
  typedef mi::ISint16 Interface_type;
};

template <>
struct Type_traits<mi::Sint32>
{
  static const char* get_type_name() { return "Sint32"; };
  typedef mi::ISint32 Interface_type;
};

template <>
struct Type_traits<mi::Sint64>
{
  static const char* get_type_name() { return "Sint64"; };
  typedef mi::ISint64 Interface_type;
};

template <>
struct Type_traits<mi::Uint8>
{
  static const char* get_type_name() { return "Uint8"; };
  typedef mi::IUint8 Interface_type;
};

template <>
struct Type_traits<mi::Uint16>
{
  static const char* get_type_name() { return "Uint16"; };
  typedef mi::IUint16 Interface_type;
};

template <>
struct Type_traits<mi::Uint32>
{
  static const char* get_type_name() { return "Uint32"; };
  typedef mi::IUint32 Interface_type;
};

template <>
struct Type_traits<mi::Uint64>
{
  static const char* get_type_name() { return "Uint64"; };
  typedef mi::IUint64 Interface_type;
};

template <>
struct Type_traits<mi::Float32>
{
  static const char* get_type_name() { return "Float32"; };
  typedef mi::IFloat32 Interface_type;
};

template <>
struct Type_traits<mi::Float64>
{
  static const char* get_type_name() { return "Float64"; };
  typedef mi::IFloat64 Interface_type;
};

template <>
struct Type_traits<const char*>
{
  static const char* get_type_name() { return "String"; };
  typedef mi::IString Interface_type;
};

template <>
struct Type_traits<mi::base::Uuid>
{
  static const char* get_type_name() { return "Uuid"; };
  typedef mi::IUuid Interface_type;
};

template <>
struct Type_traits<void>
{
  static const char* get_type_name() { return "Void"; };
  typedef mi::IVoid Interface_type;
};

template <>
struct Type_traits<mi::math::Vector<bool, 2> >
{
  static const char* get_type_name() { return "Boolean<2>"; };
  typedef mi::IBoolean_2 Interface_type;
};

template <>
struct Type_traits<mi::math::Vector<bool, 3> >
{
  static const char* get_type_name() { return "Boolean<3>"; };
  typedef mi::IBoolean_3 Interface_type;
};

template <>
struct Type_traits<mi::math::Vector<bool, 4> >
{
  static const char* get_type_name() { return "Boolean<4>"; };
  typedef mi::IBoolean_4 Interface_type;
};

template <>
struct Type_traits<mi::math::Vector<mi::Sint32, 2> >
{
  static const char* get_type_name() { return "Sint32<2>"; };
  typedef mi::ISint32_2 Interface_type;
};

template <>
struct Type_traits<mi::math::Vector<mi::Sint32, 3> >
{
  static const char* get_type_name() { return "Sint32<3>"; };
  typedef mi::ISint32_3 Interface_type;
};

template <>
struct Type_traits<mi::math::Vector<mi::Sint32, 4> >
{
  static const char* get_type_name() { return "Sint32<4>"; };
  typedef mi::ISint32_4 Interface_type;
};

template <>
struct Type_traits<mi::math::Vector<mi::Uint32, 2> >
{
  static const char* get_type_name() { return "Uint32<2>"; };
  typedef mi::IUint32_2 Interface_type;
};

template <>
struct Type_traits<mi::math::Vector<mi::Uint32, 3> >
{
  static const char* get_type_name() { return "Uint32<3>"; };
  typedef mi::IUint32_3 Interface_type;
};

template <>
struct Type_traits<mi::math::Vector<mi::Uint32, 4> >
{
  static const char* get_type_name() { return "Uint32<4>"; };
  typedef mi::IUint32_4 Interface_type;
};

template <>
struct Type_traits<mi::math::Vector<mi::Float32, 2> >
{
  static const char* get_type_name() { return "Float32<2>"; };
  typedef mi::IFloat32_2 Interface_type;
};

template <>
struct Type_traits<mi::math::Vector<mi::Float32, 3> >
{
  static const char* get_type_name() { return "Float32<3>"; };
  typedef mi::IFloat32_3 Interface_type;
};

template <>
struct Type_traits<mi::math::Vector<mi::Float32, 4> >
{
  static const char* get_type_name() { return "Float32<4>"; };
  typedef mi::IFloat32_4 Interface_type;
};

template <>
struct Type_traits<mi::math::Vector<mi::Float64, 2> >
{
  static const char* get_type_name() { return "Float64<2>"; };
  typedef mi::IFloat64_2 Interface_type;
};

template <>
struct Type_traits<mi::math::Vector<mi::Float64, 3> >
{
  static const char* get_type_name() { return "Float64<3>"; };
  typedef mi::IFloat64_3 Interface_type;
};

template <>
struct Type_traits<mi::math::Vector<mi::Float64, 4> >
{
  static const char* get_type_name() { return "Float64<4>"; };
  typedef mi::IFloat64_4 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<bool, 2, 2> >
{
  static const char* get_type_name() { return "Boolean<2,2>"; };
  typedef mi::IBoolean_2_2 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<bool, 2, 3> >
{
  static const char* get_type_name() { return "Boolean<2,3>"; };
  typedef mi::IBoolean_2_3 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<bool, 2, 4> >
{
  static const char* get_type_name() { return "Boolean<2,4>"; };
  typedef mi::IBoolean_2_4 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<bool, 3, 2> >
{
  static const char* get_type_name() { return "Boolean<3,2>"; };
  typedef mi::IBoolean_3_2 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<bool, 3, 3> >
{
  static const char* get_type_name() { return "Boolean<3,3>"; };
  typedef mi::IBoolean_3_3 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<bool, 3, 4> >
{
  static const char* get_type_name() { return "Boolean<3,4>"; };
  typedef mi::IBoolean_3_4 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<bool, 4, 2> >
{
  static const char* get_type_name() { return "Boolean<4,2>"; };
  typedef mi::IBoolean_4_2 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<bool, 4, 3> >
{
  static const char* get_type_name() { return "Boolean<4,3>"; };
  typedef mi::IBoolean_4_3 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<bool, 4, 4> >
{
  static const char* get_type_name() { return "Boolean<4,4>"; };
  typedef mi::IBoolean_4_4 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Sint32, 2, 2> >
{
  static const char* get_type_name() { return "Sint32<2,2>"; };
  typedef mi::ISint32_2_2 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Sint32, 2, 3> >
{
  static const char* get_type_name() { return "Sint32<2,3>"; };
  typedef mi::ISint32_2_3 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Sint32, 2, 4> >
{
  static const char* get_type_name() { return "Sint32<2,4>"; };
  typedef mi::ISint32_2_4 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Sint32, 3, 2> >
{
  static const char* get_type_name() { return "Sint32<3,2>"; };
  typedef mi::ISint32_3_2 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Sint32, 3, 3> >
{
  static const char* get_type_name() { return "Sint32<3,3>"; };
  typedef mi::ISint32_3_3 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Sint32, 3, 4> >
{
  static const char* get_type_name() { return "Sint32<3,4>"; };
  typedef mi::ISint32_3_4 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Sint32, 4, 2> >
{
  static const char* get_type_name() { return "Sint32<4,2>"; };
  typedef mi::ISint32_4_2 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Sint32, 4, 3> >
{
  static const char* get_type_name() { return "Sint32<4,3>"; };
  typedef mi::ISint32_4_3 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Sint32, 4, 4> >
{
  static const char* get_type_name() { return "Sint32<4,4>"; };
  typedef mi::ISint32_4_4 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Uint32, 2, 2> >
{
  static const char* get_type_name() { return "Uint32<2,2>"; };
  typedef mi::IUint32_2_2 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Uint32, 2, 3> >
{
  static const char* get_type_name() { return "Uint32<2,3>"; };
  typedef mi::IUint32_2_3 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Uint32, 2, 4> >
{
  static const char* get_type_name() { return "Uint32<2,4>"; };
  typedef mi::IUint32_2_4 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Uint32, 3, 2> >
{
  static const char* get_type_name() { return "Uint32<3,2>"; };
  typedef mi::IUint32_3_2 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Uint32, 3, 3> >
{
  static const char* get_type_name() { return "Uint32<3,3>"; };
  typedef mi::IUint32_3_3 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Uint32, 3, 4> >
{
  static const char* get_type_name() { return "Uint32<3,4>"; };
  typedef mi::IUint32_3_4 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Uint32, 4, 2> >
{
  static const char* get_type_name() { return "Uint32<4,2>"; };
  typedef mi::IUint32_4_2 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Uint32, 4, 3> >
{
  static const char* get_type_name() { return "Uint32<4,3>"; };
  typedef mi::IUint32_4_3 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Uint32, 4, 4> >
{
  static const char* get_type_name() { return "Uint32<4,4>"; };
  typedef mi::IUint32_4_4 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Float32, 2, 2> >
{
  static const char* get_type_name() { return "Float32<2,2>"; };
  typedef mi::IFloat32_2_2 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Float32, 2, 3> >
{
  static const char* get_type_name() { return "Float32<2,3>"; };
  typedef mi::IFloat32_2_3 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Float32, 2, 4> >
{
  static const char* get_type_name() { return "Float32<2,4>"; };
  typedef mi::IFloat32_2_4 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Float32, 3, 2> >
{
  static const char* get_type_name() { return "Float32<3,2>"; };
  typedef mi::IFloat32_3_2 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Float32, 3, 3> >
{
  static const char* get_type_name() { return "Float32<3,3>"; };
  typedef mi::IFloat32_3_3 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Float32, 3, 4> >
{
  static const char* get_type_name() { return "Float32<3,4>"; };
  typedef mi::IFloat32_3_4 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Float32, 4, 2> >
{
  static const char* get_type_name() { return "Float32<4,2>"; };
  typedef mi::IFloat32_4_2 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Float32, 4, 3> >
{
  static const char* get_type_name() { return "Float32<4,3>"; };
  typedef mi::IFloat32_4_3 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Float32, 4, 4> >
{
  static const char* get_type_name() { return "Float32<4,4>"; };
  typedef mi::IFloat32_4_4 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Float64, 2, 2> >
{
  static const char* get_type_name() { return "Float64<2,2>"; };
  typedef mi::IFloat64_2_2 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Float64, 2, 3> >
{
  static const char* get_type_name() { return "Float64<2,3>"; };
  typedef mi::IFloat64_2_3 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Float64, 2, 4> >
{
  static const char* get_type_name() { return "Float64<2,4>"; };
  typedef mi::IFloat64_2_4 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Float64, 3, 2> >
{
  static const char* get_type_name() { return "Float64<3,2>"; };
  typedef mi::IFloat64_3_2 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Float64, 3, 3> >
{
  static const char* get_type_name() { return "Float64<3,3>"; };
  typedef mi::IFloat64_3_3 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Float64, 3, 4> >
{
  static const char* get_type_name() { return "Float64<3,4>"; };
  typedef mi::IFloat64_3_4 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Float64, 4, 2> >
{
  static const char* get_type_name() { return "Float64<4,2>"; };
  typedef mi::IFloat64_4_2 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Float64, 4, 3> >
{
  static const char* get_type_name() { return "Float64<4,3>"; };
  typedef mi::IFloat64_4_3 Interface_type;
};

template <>
struct Type_traits<mi::math::Matrix<mi::Float64, 4, 4> >
{
  static const char* get_type_name() { return "Float64<4,4>"; };
  typedef mi::IFloat64_4_4 Interface_type;
};

template <>
struct Type_traits<mi::math::Color>
{
  static const char* get_type_name() { return "Color"; };
  typedef mi::IColor Interface_type;
};

template <>
struct Type_traits<mi::math::Spectrum>
{
  static const char* get_type_name() { return "Spectrum"; };
  typedef mi::ISpectrum Interface_type;
};

template <>
struct Type_traits<mi::math::Bbox<mi::Float32, 3> >
{
  static const char* get_type_name() { return "Bbox3"; };
  typedef mi::IBbox3 Interface_type;
};

template <typename I, Size DIM>
struct Vector_type_traits
{
};

template <>
struct Vector_type_traits<bool, 2>
{
  typedef mi::IBoolean_2 Interface_type;
};

template <>
struct Vector_type_traits<bool, 3>
{
  typedef mi::IBoolean_3 Interface_type;
};

template <>
struct Vector_type_traits<bool, 4>
{
  typedef mi::IBoolean_4 Interface_type;
};

template <>
struct Vector_type_traits<mi::Sint32, 2>
{
  typedef mi::ISint32_2 Interface_type;
};

template <>
struct Vector_type_traits<mi::Sint32, 3>
{
  typedef mi::ISint32_3 Interface_type;
};

template <>
struct Vector_type_traits<mi::Sint32, 4>
{
  typedef mi::ISint32_4 Interface_type;
};

template <>
struct Vector_type_traits<mi::Float32, 2>
{
  typedef mi::IFloat32_2 Interface_type;
};

template <>
struct Vector_type_traits<mi::Float32, 3>
{
  typedef mi::IFloat32_3 Interface_type;
};

template <>
struct Vector_type_traits<mi::Float32, 4>
{
  typedef mi::IFloat32_4 Interface_type;
};

template <>
struct Vector_type_traits<mi::Float64, 2>
{
  typedef mi::IFloat64_2 Interface_type;
};

template <>
struct Vector_type_traits<mi::Float64, 3>
{
  typedef mi::IFloat64_3 Interface_type;
};

template <>
struct Vector_type_traits<mi::Float64, 4>
{
  typedef mi::IFloat64_4 Interface_type;
};

template <typename I, Size ROW, Size COL>
struct Matrix_type_traits
{
};

template <>
struct Matrix_type_traits<bool, 2, 2>
{
  typedef mi::IBoolean_2_2 Interface_type;
};

template <>
struct Matrix_type_traits<bool, 2, 3>
{
  typedef mi::IBoolean_2_3 Interface_type;
};

template <>
struct Matrix_type_traits<bool, 2, 4>
{
  typedef mi::IBoolean_2_4 Interface_type;
};

template <>
struct Matrix_type_traits<bool, 3, 2>
{
  typedef mi::IBoolean_3_2 Interface_type;
};

template <>
struct Matrix_type_traits<bool, 3, 3>
{
  typedef mi::IBoolean_3_3 Interface_type;
};

template <>
struct Matrix_type_traits<bool, 3, 4>
{
  typedef mi::IBoolean_3_4 Interface_type;
};

template <>
struct Matrix_type_traits<bool, 4, 2>
{
  typedef mi::IBoolean_4_2 Interface_type;
};

template <>
struct Matrix_type_traits<bool, 4, 3>
{
  typedef mi::IBoolean_4_3 Interface_type;
};

template <>
struct Matrix_type_traits<bool, 4, 4>
{
  typedef mi::IBoolean_4_4 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Sint32, 2, 2>
{
  typedef mi::ISint32_2_2 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Sint32, 2, 3>
{
  typedef mi::ISint32_2_3 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Sint32, 2, 4>
{
  typedef mi::ISint32_2_4 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Sint32, 3, 2>
{
  typedef mi::ISint32_3_2 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Sint32, 3, 3>
{
  typedef mi::ISint32_3_3 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Sint32, 3, 4>
{
  typedef mi::ISint32_3_4 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Sint32, 4, 2>
{
  typedef mi::ISint32_4_2 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Sint32, 4, 3>
{
  typedef mi::ISint32_4_3 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Sint32, 4, 4>
{
  typedef mi::ISint32_4_4 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Float32, 2, 2>
{
  typedef mi::IFloat32_2_2 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Float32, 2, 3>
{
  typedef mi::IFloat32_2_3 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Float32, 2, 4>
{
  typedef mi::IFloat32_2_4 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Float32, 3, 2>
{
  typedef mi::IFloat32_3_2 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Float32, 3, 3>
{
  typedef mi::IFloat32_3_3 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Float32, 3, 4>
{
  typedef mi::IFloat32_3_4 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Float32, 4, 2>
{
  typedef mi::IFloat32_4_2 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Float32, 4, 3>
{
  typedef mi::IFloat32_4_3 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Float32, 4, 4>
{
  typedef mi::IFloat32_4_4 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Float64, 2, 2>
{
  typedef mi::IFloat64_2_2 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Float64, 2, 3>
{
  typedef mi::IFloat64_2_3 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Float64, 2, 4>
{
  typedef mi::IFloat64_2_4 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Float64, 3, 2>
{
  typedef mi::IFloat64_3_2 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Float64, 3, 3>
{
  typedef mi::IFloat64_3_3 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Float64, 3, 4>
{
  typedef mi::IFloat64_3_4 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Float64, 4, 2>
{
  typedef mi::IFloat64_4_2 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Float64, 4, 3>
{
  typedef mi::IFloat64_4_3 Interface_type;
};

template <>
struct Matrix_type_traits<mi::Float64, 4, 4>
{
  typedef mi::IFloat64_4_4 Interface_type;
};

/*@}*/ // end group mi_neuray_types

} // namespace mi

#endif // MI_NEURAYLIB_TYPE_TRAITS_H
