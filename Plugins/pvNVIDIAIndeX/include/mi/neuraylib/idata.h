/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief      Types

#ifndef MI_NEURAYLIB_IDATA_H
#define MI_NEURAYLIB_IDATA_H

#include <mi/base/interface_declare.h>

namespace mi {

/** \defgroup mi_neuray_types Types
    \ingroup mi_neuray
*/

/** \defgroup mi_neuray_simple_types Simple Types
    \ingroup mi_neuray_types

    This module lists all interfaces related to simple types.

    It only exists to split the very high number of interfaces related to types into smaller
    subsets. See \ref mi_neuray_types for an explanation of the type system.
*/

/** \defgroup mi_neuray_collections Collections
    \ingroup mi_neuray_types

    This module and its submodule Compounds list all interfaces related to collections.

    Both modules only exist to split the very high number of interfaces related to types into
    smaller subsets. See \ref mi_neuray_types for an explanation of the type system.

    \see \ref mi_neuray_compounds
*/

/** \addtogroup mi_neuray_types

    Types are organized in a hierarchy based on the basic interface #mi::IData.

    The hierarchy of types based on #mi::IData is split into two branches, <em>simple types</em>
    (derived from #mi::IData_simple) and <em>collections</em> (derived from #mi::IData_collection).
    Simple types are e.g. numbers and strings. Collections are e.g. arrays, structures, maps,
    vectors, and matrices.

    Types are identified by type names. The method #mi::IData::get_type_name() returns the type
    name of an instance of #mi::IData. Most notably type names are used to create instances of
    types via #mi::neuraylib::IFactory::create()\ifnot DICE_API and
    #mi::neuraylib::ITransaction::create()\endif, and to create attributes or to query their type.

    Since there is a very high number of interfaces related to types they are not listed in this
    module. Rather they have been split into three submodules \ref mi_neuray_simple_types,
    \ref mi_neuray_collections, and \ref mi_neuray_compounds (compounds are a subset of the
    collections).

    \ifnot DICE_API
    The free functions #mi::set_value() and #mi::get_value() including the various specializations
    may help to write/read values to/from instances of #mi::IData.
    \endif

    \par Simple types

    Simple types are booleans, integral and floating-point numbers, strings, UUIDs, \c void,
    pointers, and references. All interfaces of the simple types are derived from #mi::IData_simple.
    For most simple types there is an equivalent C++ class. With the exception of pointers,
    references, and enums the type name of simple types equals the interface name without the
    leading \c "I". See also the module \ref mi_neuray_simple_types.


    <table>
    <tr>
        <th>Type name</th>
        <th>Attribute type</th>
        <th>Interface</th>
        <th>C++ class</th>
        <th>Description</th>
    </tr>
    <tr><td>\c "Boolean"           </td><td>yes</td><td>#mi::IBoolean</td><td>bool</td>
                                        <td>Boolean</td></tr>
    <tr><td>\c "Sint8"             </td><td>yes</td><td>#mi::ISint8</td><td>mi::Sint8</td>
                                        <td>Signed 8-bit integer</td></tr>
    <tr><td>\c "Sint16"            </td><td>yes</td><td>#mi::ISint16</td><td>mi::Sint16</td>
                                        <td>Signed 16-bit integer</td></tr>
    <tr><td>\c "Sint32"            </td><td>yes</td><td>#mi::ISint32</td><td>mi::Sint32</td>
                                        <td>Signed 32-bit integer</td></tr>
    <tr><td>\c "Sint64"            </td><td>yes</td><td>#mi::ISint64</td><td>mi::Sint64</td>
                                        <td>Signed 64-bit integer</td></tr>
    <tr><td>\c "Uint8"             </td><td>no</td><td>#mi::IUint8</td><td>mi::Uint8</td>
                                        <td>Unsigned 8-bit integer</td></tr>
    <tr><td>\c "Uint16"            </td><td>no</td><td>#mi::IUint16</td><td>mi::Uint16</td>
                                        <td>Unsigned 16-bit integer</td></tr>
    <tr><td>\c "Uint32"            </td><td>no</td><td>#mi::IUint32</td><td>mi::Uint32</td>
                                        <td>Unsigned 32-bit integer</td></tr>
    <tr><td>\c "Uint64"            </td><td>no</td><td>#mi::IUint64</td><td>mi::Uint64</td>
                                        <td>Unsigned 64-bit integer</td></tr>
    <tr><td>\c "Float32"           </td><td>yes</td><td>#mi::IFloat32 </td><td>mi::Float32 </td>
                                        <td>32-bit IEEE-754 single-precision floating-point number
                                        </td></tr>
    <tr><td>\c "Float64"           </td><td>yes</td><td>#mi::IFloat64 </td><td>mi::Float64 </td>
                                        <td>64-bit IEEE-754 double-precision floating-point number
                                        </td></tr>
    <tr><td>\c "Difference"        </td><td>no</td><td>#mi::IDifference</td><td>mi::Difference</td>
                                        <td>Signed 32-bit or 64-bit integer, depending on the
                                        architecture</td></tr>
    <tr><td>\c "Size"              </td><td>no</td><td>#mi::ISize</td><td>mi::Size</td>
                                        <td>Unsigned 32-bit or 64-bit integer, depending on the
                                        architecture</td></tr>
    <tr><td>\c "String"            </td><td>yes</td><td>#mi::IString</td><td>const char*</td>
                                        <td>String representation in ISO-8859-1 encoding</td></tr>
    <tr><td>\c "Uuid"              </td><td>no</td><td>#mi::IUuid</td><td>mi::base::Uuid</td>
                                        <td>Universally unique identifier (UUID)</td></tr>
    <tr><td>\c "Void"              </td><td>no</td><td>#mi::IVoid</td><td>void</td>
                                        <td>Void type</td></tr>
    <tr><td>\c "Pointer<T>"        </td><td>no</td><td>#mi::IPointer</td><td>-</td>
                                        <td>Mutable pointer to an instance of type T</td></tr>
    <tr><td>\c "Const_pointer<T>"  </td><td>no</td><td>#mi::IConst_pointer</td><td>-</td>
                                        <td>Const pointer to an instance of type T</td></tr>
    <tr><td>as registered (1)      </td><td>yes</td><td>#mi::IEnum</td>
                                        <td>-</td>
                                        <td>Subsets of mi::Sint32 values, identified by strings
                                        </td></tr>
    \ifnot DICE_API
    <tr><td>\c "Ref"               </td><td>yes</td><td>#mi::IRef</td><td>-</td>
                                        <td>Reference to another database element</td></tr>
    \endif
    </table>
    (1) The type name of an enum is specific to the actual enum type. There is no naming
        pattern as for the other simple types. \ifnot MDL_SDK_API User-defined enum types require
        the registration of a corresponding enum declaration with a type name (see
        #mi::neuraylib::IExtension_api::register_enum_decl()). \endif

    \par Collections

    Collections are static and dynamic arrays, maps, structures, and compounds (for compounds see
    separate section below). All interfaces of collections are derived from #mi::IData_collection.
    For all collection types there are similar constructs in C++, but there are no directly
    equivalent C++ classes as for the simple types. The type names of collections are typically
    constructed according to certain rules, typically involving the type name of the contained
    elements or values. See the table below for examples or the corresponding interfaces for an
    exact description. See also the module \ref mi_neuray_collections.

    <table>
    <tr>
        <th>Type name</th>
        <th>Attribute type</th>
        <th>Interface</th>
        <th>C++ class</th>
        <th>Description</th>
    </tr>
    <tr><td>\c "T[N]"        </td><td>yes (2)(3)</td><td>#mi::IArray</td>
                                  <td>-</td>
                                  <td>Static array of N elements of type T, N > 0</td></tr>
    <tr><td>\c "T[]"         </td><td>yes (2)</td><td>#mi::IDynamic_array</td>
                                  <td>-</td>
                                  <td>Dynamic array of elements of type T</td></tr>
    <tr><td>\c "Map<T>"      </td><td>no</td><td>#mi::IMap</td>
                                  <td>-</td>
                                  <td>Set of key-value pairs, values are of type T</td></tr>
    <tr><td>as registered (4)</td><td>yes</td><td>#mi::IStructure</td>
                                  <td>-</td>
                                  <td>Ordered set of key-value pairs, values are of arbitrary
                                      types</td></tr>
    </table>
    (2) For attributes, the type of array elements must not be an array (but it can be a structure
        with arrays as members). \n
    (3) For attributes, the array length N must not be zero. \n
    (4) The type name of a structure is specific to the actual structure type. There is no naming
        pattern as for arrays or maps. \ifnot MDL_SDK_API User-defined structure types require the
        registration of a corresponding structure declaration with a type name (see
        #mi::neuraylib::IExtension_api::register_structure_decl()). \n \endif \ifnot DICE_API
    \endif

    \par Compounds

    Compound types are vectors, matrices, colors, spectrums, and bounding boxes. Actually, compounds
    belong to the set of collections, they are not a third kind of type besides simple types and
    collections.In the documentation they are split into their own module due to the very high
    number of interfaces and typedefs for compounds. See also the module \ref mi_neuray_compounds.

    For all compound types there is a C++ class from the %math API as counterpart. Type names for
    vectors and matrices are based on the type name of the element followed by the dimension
    in angle brackets (\c "<N>" or \c "<M,N>", N = 2, 3, or 4).

    <table>
    <tr>
        <th>Type name</th>
        <th>Attribute type</th>
        <th>Interface</th>
        <th>C++ class</th>
        <th>Description</th>
    </tr>
    <tr><td>\c "Boolean<N>"  </td><td>yes</td><td>#mi::IBoolean_2, ...</td>
                                  <td>mi::Boolean_2, ...</td>
                                  <td>N x bool vector, N = 2, 3, or 4</td></tr>
    <tr><td>\c "Sint32<N>"   </td><td>yes</td><td>#mi::ISint32_2, ...</td>
                                  <td>mi::Sint32_2, ...</td>
                                  <td>N x Sint32 vector, N = 2, 3, or 4</td></tr>
    <tr><td>\c "Uint32<N>"   </td><td>no</td><td>#mi::IUint32_2, ...</td>
                                  <td>mi::Uint32_2, ...</td>
                                  <td>N x Uint32 vector, N = 2, 3, or 4</td></tr>
    <tr><td>\c "Float32<N>"  </td><td>yes</td><td>#mi::IFloat32_2, ...</td>
                                  <td>mi::Float32_2, ...</td>
                                  <td>N x Float32 vector, N = 2, 3, or 4</td></tr>
    <tr><td>\c "Float64<N>"   </td><td>yes</td><td>#mi::IFloat64_2, ...</td>
                                  <td>mi::Float64_2, ...</td>
                                  <td>N x Float64 vector, N = 2, 3, or 4</td></tr>
    <tr><td>\c "Boolean<M,N>"</td><td>no</td><td>#mi::IBoolean_2_2, ...</td>
                                  <td>mi::Boolean_2_2, ...</td>
                                  <td>M x N matrix of bool, M, N = 2, 3, or 4</td></tr>
    <tr><td>\c "Sint32<M,N>" </td><td>no</td><td>#mi::ISint32_2_2, ...</td>
                                  <td>mi::Sint32_2_2, ...</td>
                                  <td>M x N matrix of Sint32, M, N = 2, 3, or 4</td></tr>
    <tr><td>\c "Uint32<M,N>" </td><td>no</td><td>#mi::IUint32_2_2, ...</td>
                                  <td>mi::Uint32_2_2, ...</td>
                                  <td>M x N matrix of Uint32, M, N = 2, 3, or 4</td></tr>
    <tr><td>\c "Float32<M,N>"</td><td>yes</td><td>#mi::IFloat32_2_2, ...</td>
                                  <td>mi::Float32_2_2, ...</td>
                                  <td>M x N matrix of Float32, M, N = 2, 3, or 4</td></tr>
    <tr><td>\c "Float64<M,N>"</td><td>yes</td><td>#mi::IFloat64_2_2, ...</td>
                                  <td>mi::Float64_2_2, ...</td>
                                  <td>M x N matrix of Float64, M, N = 2, 3, or 4</td></tr>
    <tr><td>\c "Color"       </td><td>yes</td><td>#mi::IColor</td>
                                  <td>mi::Color</td>
                                  <td>4 x Float32 representing RGBA color</td></tr>
    <tr><td>\c "Color3"      </td><td>yes</td><td>#mi::IColor3</td>
                                  <td>mi::Color</td>
                                  <td>3 x Float32 representing RGB color</td></tr>
    <tr><td>\c "Spectrum"    </td><td>yes</td><td>#mi::ISpectrum</td>
                                  <td>mi::Spectrum</td>
                                  <td>3 x Float32 representing three color bands</td></tr>
    <tr><td>\c "Bbox3"       </td><td>no</td><td>#mi::IBbox3</td>
                                  <td>mi::Bbox3</td>
                                  <td>Bounding box, represented by two mi::Float32_3</td></tr>
    </table>
*/

/** \addtogroup mi_neuray_types
    \par Pixel types

    Pixel types denote the type of the pixel of a canvas or image. The table below lists the valid
    pixel types. See also \if IRAY_API #mi::neuraylib::IImage, \endif #mi::neuraylib::ICanvas and
    #mi::neuraylib::ITile.

    <table>
    <tr>
        <th>Type name</th>
        <th>Interface</th>
        <th>C++ class</th>
        <th>Description</th>
    </tr>
    <tr><td>\c "Sint8"     </td><td>#mi::ISint8</td><td>mi::Sint8</td>
                                <td>Signed 8-bit integer (6)</td></tr>
    <tr><td>\c "Sint32"    </td><td>#mi::ISint32</td><td>mi::Sint32</td>
                                <td>Signed 32-bit integer</td></tr>
    <tr><td>\c "Float32"   </td><td>#mi::IFloat32</td><td>mi::Float32</td>
                                <td>32-bit IEEE-754 single-precision floating-point number</td></tr>
    <tr><td>\c "Float32<2>"</td><td>#mi::IFloat32_2</td><td>mi::Float32_2</td>
                                <td>2 x Float32</td></tr>
    <tr><td>\c "Float32<3>"</td><td>#mi::IFloat32_3</td><td>mi::Float32_3</td>
                                <td>3 x Float32</td></tr>
    <tr><td>\c "Float32<4>"</td><td>#mi::IFloat32_4</td><td>mi::Float32_4</td>
                                <td>4 x Float32</td></tr>
    <tr><td>\c "Rgb"       </td><td>-</td><td>-</td>
                                <td>3 x Uint8   representing RGB   color</td></tr>
    <tr><td>\c "Rgba"      </td><td>-</td><td>-</td>
                                <td>4 x Uint8   representing RGBA  color</td></tr>
    <tr><td>\c "Rgbe"      </td><td>-</td><td>-</td>
                                <td>4 x Uint8   representing RGBE  color</td></tr>
    <tr><td>\c "Rgbea"     </td><td>-</td><td>-</td>
                                <td>5 x Uint8   representing RGBEA color</td></tr>
    <tr><td>\c "Rgb_16"    </td><td>-</td><td>-</td>
                                <td>3 x Uint16  representing RGB   color</td></tr>
    <tr><td>\c "Rgba_16"   </td><td>-</td><td>-</td>
                                <td>4 x Uint16  representing RGBA  color</td></tr>
    <tr><td>\c "Rgb_fp"    </td><td>-</td><td>-</td>
                                <td>3 x Float32 representing RGB   color</td></tr>
    <tr><td>\c "Color"     </td><td>mi::IColor</td><td>mi::Color</td>
                                <td>4 x Float32 representing RGBA  color</td></tr>
    </table>
    (6) For most purposes, in particular for pixel type conversion, the data is actually treated as
        \em unsigned 8-bit integer.
*/

/** \addtogroup mi_neuray_types
@{
*/

/// This interface is the %base interface of all types.
///
/// See \ref mi_neuray_types for description of the type system.
///
/// This %base interface provides a single method #get_type_name() to allow dynamic inspection of
/// type names. See \ref mi_neuray_types for possible type names.
class IData :
    public base::Interface_declare<0x2e5f84bc,0x783a,0x4551,0x9f,0xca,0x72,0x2f,0xb8,0x38,0xc4,0x7c>
{
public:
    /// Returns the type name.
    virtual const char* get_type_name() const = 0;
};

/*@}*/ // end group mi_neuray_types

/** \addtogroup mi_neuray_simple_types
@{
*/

/// This interface represents simple types.
///
/// Simple types are numbers, strings, pointers, UUIDs, void, pointers, and references. The
/// interface does not provide any specific methods. It is just a common %base interface for all
/// simple types.
///
/// See \ref mi_neuray_types for description of the type system.
///
/// \see #mi::IData_collection
class IData_simple :
    public base::Interface_declare<0xc33c5a05,0xe7a5,0x4154,0xb8,0x87,0xee,0x1f,0x4d,0x5b,0x02,0x02,
                                   IData>
{
};

/*@}*/ // end group mi_neuray_simple_types

/** \addtogroup mi_neuray_collections
@{
*/

/// This interface represents collections.
///
/// All collections are derived from this interface which exposes a generic way to access the data
/// via a key-value or index-value approach. Collections represent an ordered sequence of values.
/// Collections may be typed (all values are of the same type) or untyped.
///
/// See \ref mi_neuray_types for description of the type system.
///
/// In general, the sets of keys is fixed (additional methods on specific interfaces might insert
/// new keys, e.g., #mi::IMap::insert()) and type-specific.
///
/// Note that the interface pointers returned from all variants of \c get_value() are not distinct
/// copies of the values stored inside the collection, but represent the value actually stored in
/// the collection. It is not specified whether the methods set_value() copy the value in their
/// argument or not.
///
/// \see #mi::IData_simple
class IData_collection :
    public base::Interface_declare<0x1bb2be0f,0x0dc6,0x44b2,0x93,0xb9,0xd1,0xba,0x6a,0x31,0x88,0x1c,
                                   IData>
{
public:
    /// Returns the number of values.
    virtual Size get_length() const = 0;

    /// Returns the key corresponding to \p index.
    ///
    /// \return    The key, or \c NULL in case of failure.
    virtual const char* get_key( Size index) const = 0;

    /// Indicates whether the key \p key exists or not.
    virtual bool has_key( const char* key) const = 0;

    /// Returns the value for key \p key.
    ///
    /// \note     If a literal \c 0 is passed for \p key, the call is ambiguous. You need to
    ///           explicitly cast the argument to \c const \c char*.
    virtual const base::IInterface* get_value( const char* key) const = 0;

    /// Returns the value for key \p key.
    ///
    /// \note     If a literal \c 0 is passed for \p key, the call is ambiguous. You need to
    ///           explicitly cast the argument to \c const \c char*.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience. It eliminates the need to call
    /// #mi::base::IInterface::get_interface(const Uuid&)
    /// on the returned pointer, since the return type already is a pointer to the type \p T
    /// specified as template parameter.
    ///
    /// \tparam T     The interface type of the element to return.
    template<class T>
    const T* get_value( const char* key) const
    {
        const base::IInterface* ptr_iinterface = get_value( key);
        if ( !ptr_iinterface)
            return 0;
        const T* ptr_T = static_cast<const T*>( ptr_iinterface->get_interface( typename T::IID()));
        ptr_iinterface->release();
        return ptr_T;
    }

    /// Returns the value for key \p key.
    ///
    /// \note     If a literal \c 0 is passed for \p key, the call is ambiguous. You need to
    ///           explicitly cast the argument to \c const \c char*.
    virtual base::IInterface* get_value( const char* key) = 0;

    /// Returns the value for key \p key.
    ///
    /// \note     If a literal \c 0 is passed for \p key, the call is ambiguous. You need to
    ///           explicitly cast the argument to \c const \c char*.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience. It eliminates the need to call
    /// #mi::base::IInterface::get_interface(const Uuid&)
    /// on the returned pointer, since the return type already is a pointer to the type \p T
    /// specified as template parameter.
    ///
    /// \tparam T     The interface type of the element to return.
    template<class T>
    T* get_value( const char* key)
    {
        base::IInterface* ptr_iinterface = get_value( key);
        if ( !ptr_iinterface)
            return 0;
        T* ptr_T = static_cast<T*>( ptr_iinterface->get_interface( typename T::IID()));
        ptr_iinterface->release();
        return ptr_T;
    }

    /// Returns the value for index \p index.
    ///
    /// \note     If a literal \c 0 is passed for \p index, the call is ambiguous. You need to
    ///           explicitly cast the argument to #mi::Size.
    virtual const base::IInterface* get_value( Size index) const = 0;

    /// Returns the value for index \p index.
    ///
    /// \note     If a literal \c 0 is passed for \p index, the call is ambiguous. You need to
    ///           explicitly cast the argument to #mi::Size.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience. It eliminates the need to call
    /// #mi::base::IInterface::get_interface(const Uuid&)
    /// on the returned pointer, since the return type already is a pointer to the type \p T
    /// specified as template parameter.
    ///
    /// \tparam T     The interface type of the element to return.
    template<class T>
    const T* get_value( Size index) const
    {
        const base::IInterface* ptr_iinterface = get_value( index);
        if ( !ptr_iinterface)
            return 0;
        const T* ptr_T = static_cast<const T*>( ptr_iinterface->get_interface( typename T::IID()));
        ptr_iinterface->release();
        return ptr_T;
    }

    /// Returns the value for index \p index.
    ///
    /// \note     If a literal \c 0 is passed for \p index, the call is ambiguous. You need to
    ///           explicitly cast the argument to #mi::Size.
    virtual base::IInterface* get_value( Size index) = 0;

    /// Returns the value for index \p index.
    ///
    /// \note     If a literal \c 0 is passed for \p index, the call is ambiguous. You need to
    ///           explicitly cast the argument to #mi::Size.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience. It eliminates the need to call
    /// #mi::base::IInterface::get_interface(const Uuid&)
    /// on the returned pointer, since the return type already is a pointer to the type \p T
    /// specified as template parameter.
    ///
    /// \tparam T     The interface type of the element to return.
    template<class T>
    T* get_value( Size index)
    {
        base::IInterface* ptr_iinterface = get_value( index);
        if ( !ptr_iinterface)
            return 0;
        T* ptr_T = static_cast<T*>( ptr_iinterface->get_interface( typename T::IID()));
        ptr_iinterface->release();
        return ptr_T;
    }

    /// Stores the value for key \p key.
    ///
    /// Note that it is not possible to create new keys.
    ///
    /// \note     If a literal \c 0 is passed for \p key, the call is ambiguous. You need to
    ///           explicitly cast the argument to \c const \c char*.
    ///
    /// \return
    ///           -  0: Success.
    ///           - -1: Invalid parameters (\c NULL pointer).
    ///           - -2: Invalid key.
    ///           - -3: \p value has the wrong type.
    virtual Sint32 set_value( const char* key, base::IInterface* value) = 0;

    /// Stores the value for index \p index.
    ///
    /// Note that it is not possible to create new indices.
    ///
    /// \note     If a literal \c 0 is passed for \p index, the call is ambiguous. You need to
    ///           explicitly cast the argument to #mi::Size.
    ///
    /// \return
    ///           -  0: Success.
    ///           - -1: Invalid parameters (\c NULL pointer).
    ///           - -2: Invalid index.
    ///           - -3: \p value has the wrong type.
    virtual Sint32 set_value( Size index, base::IInterface* value) = 0;
};

/*@}*/ // end group mi_neuray_collections

/** \addtogroup mi_neuray_simple_types
@{
*/

/// This interface represents the void type.
///
/// An instance of this interface does not store any data. The type exists primarily for a matter of
/// completeness to express common C++ types as interfaces. For example, if types derived from
/// #mi::IData are used to represent function or method signatures, this type can be used to
/// represent functions or methods that return \c void.
class IVoid :
    public base::Interface_declare<0x3142c0a4,0xa138,0x472f,0x85,0xe5,0xc0,0x13,0xfc,0xd1,0x04,0x6a,
                                   IData_simple>
{
};

/*@}*/ // end group mi_neuray_simple_types

} // namespace mi

#endif // MI_NEURAYLIB_IDATA_H
