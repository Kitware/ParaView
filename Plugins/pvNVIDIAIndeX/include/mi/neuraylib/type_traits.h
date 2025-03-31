/***************************************************************************************************
 * Copyright 2025 NVIDIA Corporation. All rights reserved.
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

#include <string>

namespace mi {

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
template<typename I> struct Type_traits {};

template<> struct Type_traits<mi::IBoolean>
{
    static const char* get_type_name() { return "Boolean"; }
    using Primitive_type = bool;
};

template<> struct Type_traits<mi::ISint8>
{
    static const char* get_type_name() { return "Sint8"; }
    using Primitive_type = mi::Sint8;
};

template<> struct Type_traits<mi::ISint16>
{
    static const char* get_type_name() { return "Sint16"; }
    using Primitive_type = mi::Sint16;
};

template<> struct Type_traits<mi::ISint32>
{
    static const char* get_type_name() { return "Sint32"; }
    using Primitive_type = mi::Sint32;
};

template<> struct Type_traits<mi::ISint64>
{
    static const char* get_type_name() { return "Sint64"; }
    using Primitive_type = mi::Sint64;
};

template<> struct Type_traits<mi::IUint8>
{
    static const char* get_type_name() { return "Uint8"; }
    using Primitive_type = mi::Uint8;
};

template<> struct Type_traits<mi::IUint16>
{
    static const char* get_type_name() { return "Uint16"; }
    using Primitive_type = mi::Uint16;
};

template<> struct Type_traits<mi::IUint32>
{
    static const char* get_type_name() { return "Uint32"; }
    using Primitive_type = mi::Uint32;
};

template<> struct Type_traits<mi::IUint64>
{
    static const char* get_type_name() { return "Uint64"; }
    using Primitive_type = mi::Uint64;
};

template<> struct Type_traits<mi::IFloat32>
{
    static const char* get_type_name() { return "Float32"; }
    using Primitive_type = mi::Float32;
};

template<> struct Type_traits<mi::IFloat64>
{
    static const char* get_type_name() { return "Float64"; }
    using Primitive_type = mi::Float64;
};

template<> struct Type_traits<mi::ISize>
{
    static const char* get_type_name() { return "Size"; }
    using Primitive_type = mi::Size;
};

template<> struct Type_traits<mi::IDifference>
{
    static const char* get_type_name() { return "Difference"; }
    using Primitive_type = mi::Difference;
};

template<> struct Type_traits<mi::IString>
{
    static const char* get_type_name() { return "String"; }
    using Primitive_type = const char*;
};

template<> struct Type_traits<mi::IUuid>
{
    static const char* get_type_name() { return "Uuid"; }
    using Primitive_type = mi::base::Uuid;
};

template<> struct Type_traits<mi::IVoid>
{
    static const char* get_type_name() { return "Void"; }
    using Primitive_type = void;
};

template<> struct Type_traits<mi::IRef>
{
    static const char* get_type_name() { return "Ref"; }
    using Primitive_type = const char*;
};

template<> struct Type_traits<mi::IBoolean_2>
{
    static const char* get_type_name() { return "Boolean<2>"; }
    using Primitive_type = mi::math::Vector<bool,2>;
};

template<> struct Type_traits<mi::IBoolean_3>
{
    static const char* get_type_name() { return "Boolean<3>"; }
    using Primitive_type = mi::math::Vector<bool,3>;
};

template<> struct Type_traits<mi::IBoolean_4>
{
    static const char* get_type_name() { return "Boolean<4>"; }
    using Primitive_type = mi::math::Vector<bool,4>;
};

template<> struct Type_traits<mi::ISint32_2>
{
    static const char* get_type_name() { return "Sint32<2>"; }
    using Primitive_type = mi::math::Vector<mi::Sint32,2>;
};

template<> struct Type_traits<mi::ISint32_3>
{
    static const char* get_type_name() { return "Sint32<3>"; }
    using Primitive_type = mi::math::Vector<mi::Sint32,3>;
};

template<> struct Type_traits<mi::ISint32_4>
{
    static const char* get_type_name() { return "Sint32<4>"; }
    using Primitive_type = mi::math::Vector<mi::Sint32,4>;
};

template<> struct Type_traits<mi::IUint32_2>
{
    static const char* get_type_name() { return "Uint32<2>"; }
    using Primitive_type = mi::math::Vector<mi::Uint32,2>;
};

template<> struct Type_traits<mi::IUint32_3>
{
    static const char* get_type_name() { return "Uint32<3>"; }
    using Primitive_type = mi::math::Vector<mi::Uint32,3>;
};

template<> struct Type_traits<mi::IUint32_4>
{
    static const char* get_type_name() { return "Uint32<4>"; }
    using Primitive_type = mi::math::Vector<mi::Uint32,4>;
};

template<> struct Type_traits<mi::IFloat32_2>
{
    static const char* get_type_name() { return "Float32<2>"; }
    using Primitive_type = mi::math::Vector<mi::Float32,2>;
};

template<> struct Type_traits<mi::IFloat32_3>
{
    static const char* get_type_name() { return "Float32<3>"; }
    using Primitive_type = mi::math::Vector<mi::Float32,3>;
};

template<> struct Type_traits<mi::IFloat32_4>
{
    static const char* get_type_name() { return "Float32<4>"; }
    using Primitive_type = mi::math::Vector<mi::Float32,4>;
};

template<> struct Type_traits<mi::IFloat64_2>
{
    static const char* get_type_name() { return "Float64<2>"; }
    using Primitive_type = mi::math::Vector<mi::Float64,2>;
};

template<> struct Type_traits<mi::IFloat64_3>
{
    static const char* get_type_name() { return "Float64<3>"; }
    using Primitive_type = mi::math::Vector<mi::Float64,3>;
};

template<> struct Type_traits<mi::IFloat64_4>
{
    static const char* get_type_name() { return "Float64<4>"; }
    using Primitive_type = mi::math::Vector<mi::Float64,4>;
};

template<> struct Type_traits<mi::IBoolean_2_2>
{
    static const char* get_type_name() { return "Boolean<2,2>"; }
    using Primitive_type = mi::math::Matrix<bool,2,2>;
};

template<> struct Type_traits<mi::IBoolean_2_3>
{
    static const char* get_type_name() { return "Boolean<2,3>"; }
    using Primitive_type = mi::math::Matrix<bool,2,3>;
};

template<> struct Type_traits<mi::IBoolean_2_4>
{
    static const char* get_type_name() { return "Boolean<2,4>"; }
    using Primitive_type = mi::math::Matrix<bool,2,4>;
};

template<> struct Type_traits<mi::IBoolean_3_2>
{
    static const char* get_type_name() { return "Boolean<3,2>"; }
    using Primitive_type = mi::math::Matrix<bool,3,2>;
};

template<> struct Type_traits<mi::IBoolean_3_3>
{
    static const char* get_type_name() { return "Boolean<3,3>"; }
    using Primitive_type = mi::math::Matrix<bool,3,3>;
};

template<> struct Type_traits<mi::IBoolean_3_4>
{
    static const char* get_type_name() { return "Boolean<3,4>"; }
    using Primitive_type = mi::math::Matrix<bool,3,4>;
};

template<> struct Type_traits<mi::IBoolean_4_2>
{
    static const char* get_type_name() { return "Boolean<4,2>"; }
    using Primitive_type = mi::math::Matrix<bool,4,2>;
};

template<> struct Type_traits<mi::IBoolean_4_3>
{
    static const char* get_type_name() { return "Boolean<4,3>"; }
    using Primitive_type = mi::math::Matrix<bool,4,3>;
};

template<> struct Type_traits<mi::IBoolean_4_4>
{
    static const char* get_type_name() { return "Boolean<4,4>"; }
    using Primitive_type = mi::math::Matrix<bool,4,4>;
};

template<> struct Type_traits<mi::ISint32_2_2>
{
    static const char* get_type_name() { return "Sint32<2,2>"; }
    using Primitive_type = mi::math::Matrix<mi::Sint32,2,2>;
};

template<> struct Type_traits<mi::ISint32_2_3>
{
    static const char* get_type_name() { return "Sint32<2,3>"; }
    using Primitive_type = mi::math::Matrix<mi::Sint32,2,3>;
};

template<> struct Type_traits<mi::ISint32_2_4>
{
    static const char* get_type_name() { return "Sint32<2,4>"; }
    using Primitive_type = mi::math::Matrix<mi::Sint32,2,4>;
};

template<> struct Type_traits<mi::ISint32_3_2>
{
    static const char* get_type_name() { return "Sint32<3,2>"; }
    using Primitive_type = mi::math::Matrix<mi::Sint32,3,2>;
};

template<> struct Type_traits<mi::ISint32_3_3>
{
    static const char* get_type_name() { return "Sint32<3,3>"; }
    using Primitive_type = mi::math::Matrix<mi::Sint32,3,3>;
};

template<> struct Type_traits<mi::ISint32_3_4>
{
    static const char* get_type_name() { return "Sint32<3,4>"; }
    using Primitive_type = mi::math::Matrix<mi::Sint32,3,4>;
};

template<> struct Type_traits<mi::ISint32_4_2>
{
    static const char* get_type_name() { return "Sint32<4,2>"; }
    using Primitive_type = mi::math::Matrix<mi::Sint32,4,2>;
};

template<> struct Type_traits<mi::ISint32_4_3>
{
    static const char* get_type_name() { return "Sint32<4,3>"; }
    using Primitive_type = mi::math::Matrix<mi::Sint32,4,3>;
};

template<> struct Type_traits<mi::ISint32_4_4>
{
    static const char* get_type_name() { return "Sint32<4,4>"; }
    using Primitive_type = mi::math::Matrix<mi::Sint32,4,4>;
};

template<> struct Type_traits<mi::IUint32_2_2>
{
    static const char* get_type_name() { return "Uint32<2,2>"; }
    using Primitive_type = mi::math::Matrix<mi::Uint32,2,2>;
};

template<> struct Type_traits<mi::IUint32_2_3>
{
    static const char* get_type_name() { return "Uint32<2,3>"; }
    using Primitive_type = mi::math::Matrix<mi::Uint32,2,3>;
};

template<> struct Type_traits<mi::IUint32_2_4>
{
    static const char* get_type_name() { return "Uint32<2,4>"; }
    using Primitive_type = mi::math::Matrix<mi::Uint32,2,4>;
};

template<> struct Type_traits<mi::IUint32_3_2>
{
    static const char* get_type_name() { return "Uint32<3,2>"; }
    using Primitive_type = mi::math::Matrix<mi::Uint32,3,2>;
};

template<> struct Type_traits<mi::IUint32_3_3>
{
    static const char* get_type_name() { return "Uint32<3,3>"; }
    using Primitive_type = mi::math::Matrix<mi::Uint32,3,3>;
};

template<> struct Type_traits<mi::IUint32_3_4>
{
    static const char* get_type_name() { return "Uint32<3,4>"; }
    using Primitive_type = mi::math::Matrix<mi::Uint32,3,4>;
};

template<> struct Type_traits<mi::IUint32_4_2>
{
    static const char* get_type_name() { return "Uint32<4,2>"; }
    using Primitive_type = mi::math::Matrix<mi::Uint32,4,2>;
};

template<> struct Type_traits<mi::IUint32_4_3>
{
    static const char* get_type_name() { return "Uint32<4,3>"; }
    using Primitive_type = mi::math::Matrix<mi::Uint32,4,3>;
};

template<> struct Type_traits<mi::IUint32_4_4>
{
    static const char* get_type_name() { return "Uint32<4,4>"; }
    using Primitive_type = mi::math::Matrix<mi::Uint32,4,4>;
};

template<> struct Type_traits<mi::IFloat32_2_2>
{
    static const char* get_type_name() { return "Float32<2,2>"; }
    using Primitive_type = mi::math::Matrix<mi::Float32,2,2>;
};

template<> struct Type_traits<mi::IFloat32_2_3>
{
    static const char* get_type_name() { return "Float32<2,3>"; }
    using Primitive_type = mi::math::Matrix<mi::Float32,2,3>;
};

template<> struct Type_traits<mi::IFloat32_2_4>
{
    static const char* get_type_name() { return "Float32<2,4>"; }
    using Primitive_type = mi::math::Matrix<mi::Float32,2,4>;
};

template<> struct Type_traits<mi::IFloat32_3_2>
{
    static const char* get_type_name() { return "Float32<3,2>"; }
    using Primitive_type = mi::math::Matrix<mi::Float32,3,2>;
};

template<> struct Type_traits<mi::IFloat32_3_3>
{
    static const char* get_type_name() { return "Float32<3,3>"; }
    using Primitive_type = mi::math::Matrix<mi::Float32,3,3>;
};

template<> struct Type_traits<mi::IFloat32_3_4>
{
    static const char* get_type_name() { return "Float32<3,4>"; }
    using Primitive_type = mi::math::Matrix<mi::Float32,3,4>;
};

template<> struct Type_traits<mi::IFloat32_4_2>
{
    static const char* get_type_name() { return "Float32<4,2>"; }
    using Primitive_type = mi::math::Matrix<mi::Float32,4,2>;
};

template<> struct Type_traits<mi::IFloat32_4_3>
{
    static const char* get_type_name() { return "Float32<4,3>"; }
    using Primitive_type = mi::math::Matrix<mi::Float32,4,3>;
};

template<> struct Type_traits<mi::IFloat32_4_4>
{
    static const char* get_type_name() { return "Float32<4,4>"; }
    using Primitive_type = mi::math::Matrix<mi::Float32,4,4>;
};

template<> struct Type_traits<mi::IFloat64_2_2>
{
    static const char* get_type_name() { return "Float64<2,2>"; }
    using Primitive_type = mi::math::Matrix<mi::Float64,2,2>;
};

template<> struct Type_traits<mi::IFloat64_2_3>
{
    static const char* get_type_name() { return "Float64<2,3>"; }
    using Primitive_type = mi::math::Matrix<mi::Float64,2,3>;
};

template<> struct Type_traits<mi::IFloat64_2_4>
{
    static const char* get_type_name() { return "Float64<2,4>"; }
    using Primitive_type = mi::math::Matrix<mi::Float64,2,4>;
};

template<> struct Type_traits<mi::IFloat64_3_2>
{
    static const char* get_type_name() { return "Float64<3,2>"; }
    using Primitive_type = mi::math::Matrix<mi::Float64,3,2>;
};

template<> struct Type_traits<mi::IFloat64_3_3>
{
    static const char* get_type_name() { return "Float64<3,3>"; }
    using Primitive_type = mi::math::Matrix<mi::Float64,3,3>;
};

template<> struct Type_traits<mi::IFloat64_3_4>
{
    static const char* get_type_name() { return "Float64<3,4>"; }
    using Primitive_type = mi::math::Matrix<mi::Float64,3,4>;
};

template<> struct Type_traits<mi::IFloat64_4_2>
{
    static const char* get_type_name() { return "Float64<4,2>"; }
    using Primitive_type = mi::math::Matrix<mi::Float64,4,2>;
};

template<> struct Type_traits<mi::IFloat64_4_3>
{
    static const char* get_type_name() { return "Float64<4,3>"; }
    using Primitive_type = mi::math::Matrix<mi::Float64,4,3>;
};

template<> struct Type_traits<mi::IFloat64_4_4>
{
    static const char* get_type_name() { return "Float64<4,4>"; }
    using Primitive_type = mi::math::Matrix<mi::Float64,4,4>;
};

template<> struct Type_traits<mi::IColor>
{
    static const char* get_type_name() { return "Color"; }
    using Primitive_type = mi::math::Color;
};

template<> struct Type_traits<mi::IColor3>
{
    static const char* get_type_name() { return "Color3"; }
    using Primitive_type = mi::math::Color;
};

template<> struct Type_traits<mi::ISpectrum>
{
    static const char* get_type_name() { return "Spectrum"; }
    using Primitive_type = mi::math::Spectrum;
};

template<> struct Type_traits<mi::IBbox3>
{
    static const char* get_type_name() { return "Bbox3"; }
    using Primitive_type = mi::math::Bbox<mi::Float32,3>;
};


template<> struct Type_traits<bool>
{
    static const char* get_type_name() { return "Boolean"; };
    using Interface_type = mi::IBoolean;
};

template<> struct Type_traits<mi::Sint8>
{
    static const char* get_type_name() { return "Sint8"; };
    using Interface_type = mi::ISint8;
};

template<> struct Type_traits<mi::Sint16>
{
    static const char* get_type_name() { return "Sint16"; };
    using Interface_type = mi::ISint16;
};

template<> struct Type_traits<mi::Sint32>
{
    static const char* get_type_name() { return "Sint32"; };
    using Interface_type = mi::ISint32;
};

template<> struct Type_traits<mi::Sint64>
{
    static const char* get_type_name() { return "Sint64"; };
    using Interface_type = mi::ISint64;
};

template<> struct Type_traits<mi::Uint8>
{
    static const char* get_type_name() { return "Uint8"; };
    using Interface_type = mi::IUint8;
};

template<> struct Type_traits<mi::Uint16>
{
    static const char* get_type_name() { return "Uint16"; };
    using Interface_type = mi::IUint16;
};

template<> struct Type_traits<mi::Uint32>
{
    static const char* get_type_name() { return "Uint32"; };
    using Interface_type = mi::IUint32;
};

template<> struct Type_traits<mi::Uint64>
{
    static const char* get_type_name() { return "Uint64"; };
    using Interface_type = mi::IUint64;
};

template<> struct Type_traits<mi::Float32>
{
    static const char* get_type_name() { return "Float32"; };
    using Interface_type = mi::IFloat32;
};

template<> struct Type_traits<mi::Float64>
{
    static const char* get_type_name() { return "Float64"; };
    using Interface_type = mi::IFloat64;
};

template<> struct Type_traits<const char*>
{
    static const char* get_type_name() { return "String"; };
    using Interface_type = mi::IString;
};

template<> struct Type_traits<std::string>
{
    static const char* get_type_name() { return "String"; };
    using Interface_type = mi::IString;
};

template<std::size_t N> struct Type_traits<char[N]>
{
    static const char* get_type_name() { return "String"; };
    using Interface_type = mi::IString;
};

template<> struct Type_traits<mi::base::Uuid>
{
    static const char* get_type_name() { return "Uuid"; };
    using Interface_type = mi::IUuid;
};

template<> struct Type_traits<void>
{
    static const char* get_type_name() { return "Void"; };
    using Interface_type = mi::IVoid;
};

template<> struct Type_traits<mi::math::Vector<bool,2> >
{
    static const char* get_type_name() { return "Boolean<2>"; };
    using Interface_type = mi::IBoolean_2;
};

template<> struct Type_traits<mi::math::Vector<bool,3> >
{
    static const char* get_type_name() { return "Boolean<3>"; };
    using Interface_type = mi::IBoolean_3;
};

template<> struct Type_traits<mi::math::Vector<bool,4> >
{
    static const char* get_type_name() { return "Boolean<4>"; };
    using Interface_type = mi::IBoolean_4;
};

template<> struct Type_traits<mi::math::Vector<mi::Sint32,2> >
{
    static const char* get_type_name() { return "Sint32<2>"; };
    using Interface_type = mi::ISint32_2;
};

template<> struct Type_traits<mi::math::Vector<mi::Sint32,3> >
{
    static const char* get_type_name() { return "Sint32<3>"; };
    using Interface_type = mi::ISint32_3;
};

template<> struct Type_traits<mi::math::Vector<mi::Sint32,4> >
{
    static const char* get_type_name() { return "Sint32<4>"; };
    using Interface_type = mi::ISint32_4;
};

template<> struct Type_traits<mi::math::Vector<mi::Uint32,2> >
{
    static const char* get_type_name() { return "Uint32<2>"; };
    using Interface_type = mi::IUint32_2;
};

template<> struct Type_traits<mi::math::Vector<mi::Uint32,3> >
{
    static const char* get_type_name() { return "Uint32<3>"; };
    using Interface_type = mi::IUint32_3;
};

template<> struct Type_traits<mi::math::Vector<mi::Uint32,4> >
{
    static const char* get_type_name() { return "Uint32<4>"; };
    using Interface_type = mi::IUint32_4;
};

template<> struct Type_traits<mi::math::Vector<mi::Float32,2> >
{
    static const char* get_type_name() { return "Float32<2>"; };
    using Interface_type = mi::IFloat32_2;
};

template<> struct Type_traits<mi::math::Vector<mi::Float32,3> >
{
    static const char* get_type_name() { return "Float32<3>"; };
    using Interface_type = mi::IFloat32_3;
};

template<> struct Type_traits<mi::math::Vector<mi::Float32,4> >
{
    static const char* get_type_name() { return "Float32<4>"; };
    using Interface_type = mi::IFloat32_4;
};

template<> struct Type_traits<mi::math::Vector<mi::Float64,2> >
{
    static const char* get_type_name() { return "Float64<2>"; };
    using Interface_type = mi::IFloat64_2;
};

template<> struct Type_traits<mi::math::Vector<mi::Float64,3> >
{
    static const char* get_type_name() { return "Float64<3>"; };
    using Interface_type = mi::IFloat64_3;
};

template<> struct Type_traits<mi::math::Vector<mi::Float64,4> >
{
    static const char* get_type_name() { return "Float64<4>"; };
    using Interface_type = mi::IFloat64_4;
};

template<> struct Type_traits<mi::math::Matrix<bool,2,2> >
{
    static const char* get_type_name() { return "Boolean<2,2>"; };
    using Interface_type = mi::IBoolean_2_2;
};

template<> struct Type_traits<mi::math::Matrix<bool,2,3> >
{
    static const char* get_type_name() { return "Boolean<2,3>"; };
    using Interface_type = mi::IBoolean_2_3;
};

template<> struct Type_traits<mi::math::Matrix<bool,2,4> >
{
    static const char* get_type_name() { return "Boolean<2,4>"; };
    using Interface_type = mi::IBoolean_2_4;
};

template<> struct Type_traits<mi::math::Matrix<bool,3,2> >
{
    static const char* get_type_name() { return "Boolean<3,2>"; };
    using Interface_type = mi::IBoolean_3_2;
};

template<> struct Type_traits<mi::math::Matrix<bool,3,3> >
{
    static const char* get_type_name() { return "Boolean<3,3>"; };
    using Interface_type = mi::IBoolean_3_3;
};

template<> struct Type_traits<mi::math::Matrix<bool,3,4> >
{
    static const char* get_type_name() { return "Boolean<3,4>"; };
    using Interface_type = mi::IBoolean_3_4;
};

template<> struct Type_traits<mi::math::Matrix<bool,4,2> >
{
    static const char* get_type_name() { return "Boolean<4,2>"; };
    using Interface_type = mi::IBoolean_4_2;
};

template<> struct Type_traits<mi::math::Matrix<bool,4,3> >
{
    static const char* get_type_name() { return "Boolean<4,3>"; };
    using Interface_type = mi::IBoolean_4_3;
};

template<> struct Type_traits<mi::math::Matrix<bool,4,4> >
{
    static const char* get_type_name() { return "Boolean<4,4>"; };
    using Interface_type = mi::IBoolean_4_4;
};

template<> struct Type_traits<mi::math::Matrix<mi::Sint32,2,2> >
{
    static const char* get_type_name() { return "Sint32<2,2>"; };
    using Interface_type = mi::ISint32_2_2;
};

template<> struct Type_traits<mi::math::Matrix<mi::Sint32,2,3> >
{
    static const char* get_type_name() { return "Sint32<2,3>"; };
    using Interface_type = mi::ISint32_2_3;
};

template<> struct Type_traits<mi::math::Matrix<mi::Sint32,2,4> >
{
    static const char* get_type_name() { return "Sint32<2,4>"; };
    using Interface_type = mi::ISint32_2_4;
};

template<> struct Type_traits<mi::math::Matrix<mi::Sint32,3,2> >
{
    static const char* get_type_name() { return "Sint32<3,2>"; };
    using Interface_type = mi::ISint32_3_2;
};

template<> struct Type_traits<mi::math::Matrix<mi::Sint32,3,3> >
{
    static const char* get_type_name() { return "Sint32<3,3>"; };
    using Interface_type = mi::ISint32_3_3;
};

template<> struct Type_traits<mi::math::Matrix<mi::Sint32,3,4> >
{
    static const char* get_type_name() { return "Sint32<3,4>"; };
    using Interface_type = mi::ISint32_3_4;
};

template<> struct Type_traits<mi::math::Matrix<mi::Sint32,4,2> >
{
    static const char* get_type_name() { return "Sint32<4,2>"; };
    using Interface_type = mi::ISint32_4_2;
};

template<> struct Type_traits<mi::math::Matrix<mi::Sint32,4,3> >
{
    static const char* get_type_name() { return "Sint32<4,3>"; };
    using Interface_type = mi::ISint32_4_3;
};

template<> struct Type_traits<mi::math::Matrix<mi::Sint32,4,4> >
{
    static const char* get_type_name() { return "Sint32<4,4>"; };
    using Interface_type = mi::ISint32_4_4;
};

template<> struct Type_traits<mi::math::Matrix<mi::Uint32,2,2> >
{
    static const char* get_type_name() { return "Uint32<2,2>"; };
    using Interface_type = mi::IUint32_2_2;
};

template<> struct Type_traits<mi::math::Matrix<mi::Uint32,2,3> >
{
    static const char* get_type_name() { return "Uint32<2,3>"; };
    using Interface_type = mi::IUint32_2_3;
};

template<> struct Type_traits<mi::math::Matrix<mi::Uint32,2,4> >
{
    static const char* get_type_name() { return "Uint32<2,4>"; };
    using Interface_type = mi::IUint32_2_4;
};

template<> struct Type_traits<mi::math::Matrix<mi::Uint32,3,2> >
{
    static const char* get_type_name() { return "Uint32<3,2>"; };
    using Interface_type = mi::IUint32_3_2;
};

template<> struct Type_traits<mi::math::Matrix<mi::Uint32,3,3> >
{
    static const char* get_type_name() { return "Uint32<3,3>"; };
    using Interface_type = mi::IUint32_3_3;
};

template<> struct Type_traits<mi::math::Matrix<mi::Uint32,3,4> >
{
    static const char* get_type_name() { return "Uint32<3,4>"; };
    using Interface_type = mi::IUint32_3_4;
};

template<> struct Type_traits<mi::math::Matrix<mi::Uint32,4,2> >
{
    static const char* get_type_name() { return "Uint32<4,2>"; };
    using Interface_type = mi::IUint32_4_2;
};

template<> struct Type_traits<mi::math::Matrix<mi::Uint32,4,3> >
{
    static const char* get_type_name() { return "Uint32<4,3>"; };
    using Interface_type = mi::IUint32_4_3;
};

template<> struct Type_traits<mi::math::Matrix<mi::Uint32,4,4> >
{
    static const char* get_type_name() { return "Uint32<4,4>"; };
    using Interface_type = mi::IUint32_4_4;
};

template<> struct Type_traits<mi::math::Matrix<mi::Float32,2,2> >
{
    static const char* get_type_name() { return "Float32<2,2>"; };
    using Interface_type = mi::IFloat32_2_2;
};

template<> struct Type_traits<mi::math::Matrix<mi::Float32,2,3> >
{
    static const char* get_type_name() { return "Float32<2,3>"; };
    using Interface_type = mi::IFloat32_2_3;
};

template<> struct Type_traits<mi::math::Matrix<mi::Float32,2,4> >
{
    static const char* get_type_name() { return "Float32<2,4>"; };
    using Interface_type = mi::IFloat32_2_4;
};

template<> struct Type_traits<mi::math::Matrix<mi::Float32,3,2> >
{
    static const char* get_type_name() { return "Float32<3,2>"; };
    using Interface_type = mi::IFloat32_3_2;
};

template<> struct Type_traits<mi::math::Matrix<mi::Float32,3,3> >
{
    static const char* get_type_name() { return "Float32<3,3>"; };
    using Interface_type = mi::IFloat32_3_3;
};

template<> struct Type_traits<mi::math::Matrix<mi::Float32,3,4> >
{
    static const char* get_type_name() { return "Float32<3,4>"; };
    using Interface_type = mi::IFloat32_3_4;
};

template<> struct Type_traits<mi::math::Matrix<mi::Float32,4,2> >
{
    static const char* get_type_name() { return "Float32<4,2>"; };
    using Interface_type = mi::IFloat32_4_2;
};

template<> struct Type_traits<mi::math::Matrix<mi::Float32,4,3> >
{
    static const char* get_type_name() { return "Float32<4,3>"; };
    using Interface_type = mi::IFloat32_4_3;
};

template<> struct Type_traits<mi::math::Matrix<mi::Float32,4,4> >
{
    static const char* get_type_name() { return "Float32<4,4>"; };
    using Interface_type = mi::IFloat32_4_4;
};

template<> struct Type_traits<mi::math::Matrix<mi::Float64,2,2> >
{
    static const char* get_type_name() { return "Float64<2,2>"; };
    using Interface_type = mi::IFloat64_2_2;
};

template<> struct Type_traits<mi::math::Matrix<mi::Float64,2,3> >
{
    static const char* get_type_name() { return "Float64<2,3>"; };
    using Interface_type = mi::IFloat64_2_3;
};

template<> struct Type_traits<mi::math::Matrix<mi::Float64,2,4> >
{
    static const char* get_type_name() { return "Float64<2,4>"; };
    using Interface_type = mi::IFloat64_2_4;
};

template<> struct Type_traits<mi::math::Matrix<mi::Float64,3,2> >
{
    static const char* get_type_name() { return "Float64<3,2>"; };
    using Interface_type = mi::IFloat64_3_2;
};

template<> struct Type_traits<mi::math::Matrix<mi::Float64,3,3> >
{
    static const char* get_type_name() { return "Float64<3,3>"; };
    using Interface_type = mi::IFloat64_3_3;
};

template<> struct Type_traits<mi::math::Matrix<mi::Float64,3,4> >
{
    static const char* get_type_name() { return "Float64<3,4>"; };
    using Interface_type = mi::IFloat64_3_4;
};

template<> struct Type_traits<mi::math::Matrix<mi::Float64,4,2> >
{
    static const char* get_type_name() { return "Float64<4,2>"; };
    using Interface_type = mi::IFloat64_4_2;
};

template<> struct Type_traits<mi::math::Matrix<mi::Float64,4,3> >
{
    static const char* get_type_name() { return "Float64<4,3>"; };
    using Interface_type = mi::IFloat64_4_3;
};

template<> struct Type_traits<mi::math::Matrix<mi::Float64,4,4> >
{
    static const char* get_type_name() { return "Float64<4,4>"; };
    using Interface_type = mi::IFloat64_4_4;
};

template<> struct Type_traits<mi::math::Color>
{
    static const char* get_type_name() { return "Color"; };
    using Interface_type = mi::IColor;
};

template<> struct Type_traits<mi::math::Spectrum>
{
    static const char* get_type_name() { return "Spectrum"; };
    using Interface_type = mi::ISpectrum;
};

template<> struct Type_traits<mi::math::Bbox<mi::Float32,3> >
{
    static const char* get_type_name() { return "Bbox3"; };
    using Interface_type = mi::IBbox3;
};


template<typename I, Size DIM> struct Vector_type_traits {};

template<> struct Vector_type_traits<bool, 2>
{ using Interface_type = mi::IBoolean_2; };

template<> struct Vector_type_traits<bool, 3>
{ using Interface_type = mi::IBoolean_3; };

template<> struct Vector_type_traits<bool, 4>
{ using Interface_type = mi::IBoolean_4; };

template<> struct Vector_type_traits<mi::Sint32, 2>
{ using Interface_type = mi::ISint32_2; };

template<> struct Vector_type_traits<mi::Sint32, 3>
{ using Interface_type = mi::ISint32_3; };

template<> struct Vector_type_traits<mi::Sint32, 4>
{ using Interface_type = mi::ISint32_4; };

template<> struct Vector_type_traits<mi::Uint32, 2>
{ using Interface_type = mi::IUint32_2; };

template<> struct Vector_type_traits<mi::Uint32, 3>
{ using Interface_type = mi::IUint32_3; };

template<> struct Vector_type_traits<mi::Uint32, 4>
{ using Interface_type = mi::IUint32_4; };

template<> struct Vector_type_traits<mi::Float32, 2>
{ using Interface_type = mi::IFloat32_2; };

template<> struct Vector_type_traits<mi::Float32, 3>
{ using Interface_type = mi::IFloat32_3; };

template<> struct Vector_type_traits<mi::Float32, 4>
{ using Interface_type = mi::IFloat32_4; };

template<> struct Vector_type_traits<mi::Float64, 2>
{ using Interface_type = mi::IFloat64_2; };

template<> struct Vector_type_traits<mi::Float64, 3>
{ using Interface_type = mi::IFloat64_3; };

template<> struct Vector_type_traits<mi::Float64, 4>
{ using Interface_type = mi::IFloat64_4; };


template<typename I, Size ROW, Size COL> struct Matrix_type_traits {};

template<> struct Matrix_type_traits<bool, 2, 2>
{ using Interface_type = mi::IBoolean_2_2; };

template<> struct Matrix_type_traits<bool, 2, 3>
{ using Interface_type = mi::IBoolean_2_3; };

template<> struct Matrix_type_traits<bool, 2, 4>
{ using Interface_type = mi::IBoolean_2_4; };

template<> struct Matrix_type_traits<bool, 3, 2>
{ using Interface_type = mi::IBoolean_3_2; };

template<> struct Matrix_type_traits<bool, 3, 3>
{ using Interface_type = mi::IBoolean_3_3; };

template<> struct Matrix_type_traits<bool, 3, 4>
{ using Interface_type = mi::IBoolean_3_4; };

template<> struct Matrix_type_traits<bool, 4, 2>
{ using Interface_type = mi::IBoolean_4_2; };

template<> struct Matrix_type_traits<bool, 4, 3>
{ using Interface_type = mi::IBoolean_4_3; };

template<> struct Matrix_type_traits<bool, 4, 4>
{ using Interface_type = mi::IBoolean_4_4; };

template<> struct Matrix_type_traits<mi::Sint32, 2, 2>
{ using Interface_type = mi::ISint32_2_2; };

template<> struct Matrix_type_traits<mi::Sint32, 2, 3>
{ using Interface_type = mi::ISint32_2_3; };

template<> struct Matrix_type_traits<mi::Sint32, 2, 4>
{ using Interface_type = mi::ISint32_2_4; };

template<> struct Matrix_type_traits<mi::Sint32, 3, 2>
{ using Interface_type = mi::ISint32_3_2; };

template<> struct Matrix_type_traits<mi::Sint32, 3, 3>
{ using Interface_type = mi::ISint32_3_3; };

template<> struct Matrix_type_traits<mi::Sint32, 3, 4>
{ using Interface_type = mi::ISint32_3_4; };

template<> struct Matrix_type_traits<mi::Sint32, 4, 2>
{ using Interface_type = mi::ISint32_4_2; };

template<> struct Matrix_type_traits<mi::Sint32, 4, 3>
{ using Interface_type = mi::ISint32_4_3; };

template<> struct Matrix_type_traits<mi::Sint32, 4, 4>
{ using Interface_type = mi::ISint32_4_4; };

template<> struct Matrix_type_traits<mi::Float32, 2, 2>
{ using Interface_type = mi::IFloat32_2_2; };

template<> struct Matrix_type_traits<mi::Float32, 2, 3>
{ using Interface_type = mi::IFloat32_2_3; };

template<> struct Matrix_type_traits<mi::Float32, 2, 4>
{ using Interface_type = mi::IFloat32_2_4; };

template<> struct Matrix_type_traits<mi::Float32, 3, 2>
{ using Interface_type = mi::IFloat32_3_2; };

template<> struct Matrix_type_traits<mi::Float32, 3, 3>
{ using Interface_type = mi::IFloat32_3_3; };

template<> struct Matrix_type_traits<mi::Float32, 3, 4>
{ using Interface_type = mi::IFloat32_3_4; };

template<> struct Matrix_type_traits<mi::Float32, 4, 2>
{ using Interface_type = mi::IFloat32_4_2; };

template<> struct Matrix_type_traits<mi::Float32, 4, 3>
{ using Interface_type = mi::IFloat32_4_3; };

template<> struct Matrix_type_traits<mi::Float32, 4, 4>
{ using Interface_type = mi::IFloat32_4_4; };

template<> struct Matrix_type_traits<mi::Float64, 2, 2>
{ using Interface_type = mi::IFloat64_2_2; };

template<> struct Matrix_type_traits<mi::Float64, 2, 3>
{ using Interface_type = mi::IFloat64_2_3; };

template<> struct Matrix_type_traits<mi::Float64, 2, 4>
{ using Interface_type = mi::IFloat64_2_4; };

template<> struct Matrix_type_traits<mi::Float64, 3, 2>
{ using Interface_type = mi::IFloat64_3_2; };

template<> struct Matrix_type_traits<mi::Float64, 3, 3>
{ using Interface_type = mi::IFloat64_3_3; };

template<> struct Matrix_type_traits<mi::Float64, 3, 4>
{ using Interface_type = mi::IFloat64_3_4; };

template<> struct Matrix_type_traits<mi::Float64, 4, 2>
{ using Interface_type = mi::IFloat64_4_2; };

template<> struct Matrix_type_traits<mi::Float64, 4, 3>
{ using Interface_type = mi::IFloat64_4_3; };

template<> struct Matrix_type_traits<mi::Float64, 4, 4>
{ using Interface_type = mi::IFloat64_4_4; };


// Returns the name of interface types which have a type trait
template <typename I, typename = decltype(Type_traits<I>::get_type_name())>
const char* get_type_name()
{
    return Type_traits<I>::get_type_name();
}

// Returns the name of types which implement an interface which has a type trait
template <
    typename T,
    typename I = typename T::Interface,
    typename = decltype(Type_traits<I>::get_type_name())>
const char* get_type_name()
{
    return get_type_name<I>();
}

/**@}*/ // end group mi_neuray_types

} // namespace mi

#endif // MI_NEURAYLIB_TYPE_TRAITS_H
