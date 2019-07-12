/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief API component for creation, assignment, and cloning of instances of types.

#ifndef MI_NEURAYLIB_IFACTORY_H
#define MI_NEURAYLIB_IFACTORY_H

#include <mi/base/interface_declare.h>
#include <mi/neuraylib/idata.h>
#include <mi/neuraylib/type_traits.h>

namespace mi {

class IEnum_decl;
class IString;
class IStructure_decl;

namespace neuraylib {

class ITransaction;

/** \addtogroup mi_neuray_types
@{
*/

/// This API component allows the creation, assignment, and cloning of instances of types.
/// It also provides a comparison function for types.
///
/// Types are interfaces derived from #mi::IData. See \ref mi_neuray_types for an explanation
/// of the type system.
class IFactory : public
    mi::base::Interface_declare<0x8afad838,0xe597,0x4a81,0x92,0x34,0x51,0xfe,0xa4,0xff,0x04,0x31>
{
public:
    /// Creates an object of the type \p type_name.
    ///
    /// The arguments passed to this method are passed to the constructor of the created object.
    ///
    /// This factory allows the creation of instances of #mi::IData and derived interfaces.
    /// \ifnot DICE_API For other kind of objects like DB elements use
    /// #mi::neuraylib::ITransaction::create(). \endif It is not possible to create instances of
    /// #mi::IRef or collections of references. Use \ifnot DICE_API
    /// #mi::neuraylib::ITransaction::create() \else #mi::neuraylib::IDice_transaction \endif
    /// instead.
    ///
    /// The created object will be initialized in a manner dependent upon the passed type
    /// name. Each class has its own policy on initialization. So, one should not make any
    /// assumptions on the values of the various class members.
    ///
    /// \ifnot DICE_API \see #mi::neuraylib::ITransaction::create() \endif
    ///
    /// \param type_name    The type name of the object to create. See \ref mi_neuray_types for
    ///                     possible type names.
    /// \param argc         The number of elements in \p argv.
    /// \param argv         The array of arguments passed to the constructor.
    /// \return             A pointer to the created object on success,
    ///                     or \c NULL on failure (e.g., invalid type name).
    virtual base::IInterface* create(
        const char* type_name,
        Uint32 argc = 0,
        const base::IInterface* argv[] = 0) = 0;

    /// Creates an object of the type \p type_name.
    ///
    /// The arguments passed to this method are passed to the constructor of the created object.
    ///
    /// This factory allows the creation of instances of #mi::IData and derived interfaces.
    /// \ifnot DICE_API For other kind of objects like DB elements use
    /// #mi::neuraylib::ITransaction::create(). \endif It is not possible to create instances of
    /// #mi::IRef or collections of references. Use \ifnot DICE_API
    /// #mi::neuraylib::ITransaction::create() \else #mi::neuraylib::IDice_transaction \endif
    /// instead.
    ///
    /// The created object will be initialized in a manner dependent upon the passed type
    /// name. Each class has its own policy on initialization. So, one should not make any
    /// assumptions on the values of the various class members.
    ///
    /// Note that there are two versions of this templated member function, one that takes no
    /// arguments, and another one that takes one or three arguments (the type name, and two
    /// optional arguments passed to the factory class). The version with no arguments can only be
    /// used to create a subset of supported types: it supports only those types where the type name
    /// can be deduced from the template parameter, i.e., it does not support arrays, structures,
    /// maps, and pointers. The version with one or three arguments can be used to create any type
    /// (but requires the type name as parameter, which is redundant for many types). Attempts to
    /// use the version with no arguments with a template parameter where the type name can not be
    /// deduced results in compiler errors.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience. It eliminates the need to call
    /// #mi::base::IInterface::get_interface(const Uuid&)
    /// on the returned pointer, since the return type already is a pointer to the type \p T
    /// specified as template parameter.
    ///
    /// \ifnot DICE_API \see #mi::neuraylib::ITransaction::create() \endif
    ///
    /// \param type_name    The type name of the object to create
    /// \param argc         The number of elements in \p argv
    /// \param argv         The array of arguments passed to the constructor
    /// \tparam T           The interface type of the class to create
    /// \return             A pointer to the created object on success,
    ///                     or \c NULL on failure (e.g., invalid type name).
    template<class T>
    T* create(
        const char* type_name,
        Uint32 argc = 0,
        const base::IInterface* argv[] = 0)
    {
        base::IInterface* ptr_iinterface = create( type_name, argc, argv);
        if ( !ptr_iinterface)
            return 0;
        T* ptr_T = static_cast<T*>( ptr_iinterface->get_interface( typename T::IID()));
        ptr_iinterface->release();
        return ptr_T;
    }

    /// Creates an object of the type \p T.
    ///
    /// This factory allows the creation of instances of #mi::IData and derived interfaces.
    /// \ifnot DICE_API For other kind of objects like DB elements use
    /// #mi::neuraylib::ITransaction::create(). \endif It is not possible to create instances of
    /// #mi::IRef or collections of references. Use \ifnot DICE_API
    /// #mi::neuraylib::ITransaction::create() \else #mi::neuraylib::IDice_transaction \endif
    /// instead.
    ///
    /// The created object will be initialized in a manner dependent upon the passed type
    /// name. Each class has its own policy on initialization. So, one should not make any
    /// assumptions on the values of the various class members.
    ///
    /// Note that there are two versions of this templated member function, one that takes no
    /// arguments, and another one that takes one or three arguments (the type name, and two
    /// optional arguments passed to the factory class). The version with no arguments can only be
    /// used to create a subset of supported types: it supports only those types where the type name
    /// can be deduced from the template parameter, i.e., it does not support arrays, structures,
    /// maps, and pointers. The version with one or three arguments can be used to create any type
    /// (but requires the type name as parameter, which is redundant for many types). Attempts to
    /// use the version with no arguments with a template parameter where the type name can not be
    /// deduced results in compiler errors.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience. It eliminates the need to call
    /// #mi::base::IInterface::get_interface(const Uuid&)
    /// on the returned pointer, since the return type already is a pointer to the type \p T
    /// specified as template parameter.
    ///
    /// \ifnot DICE_API \see #mi::neuraylib::ITransaction::create() \endif
    ///
    /// \tparam T           The interface type of the class to create
    /// \return             A pointer to the created object on success,
    ///                     or \c NULL on failure (e.g., invalid type name).
    template<class T>
    T* create()
    {
        return create<T>( Type_traits<T>::get_type_name());
    }

    /// This enum represents possible events that can happen during assignment of types.
    ///
    /// The enum values are powers of two such that they can be combined in a bitmask. Such a
    /// bitmask is returned by #assign_from_to().
    enum Assign_result
    {
        /// One of the arguments \p source or \p target is \c NULL. Alternatively, a deep assignment
        /// to an instance of #mi::IPointer failed due to a \c NULL value.
        NULL_POINTER                     =  1,

        /// There is a structural mismatch between \p source and \p target.
        /// One of the two arguments is of type #mi::IData_simple and the other one is of
        /// #mi::IData_collection, or alternatively, both arguments are of type
        /// #mi::IData_collection and there is at least one key which is of type #mi::IData_simple
        /// in one argument, and of type #mi::IData_collection in the other argument.
        STRUCTURAL_MISMATCH              =  2,

        /// Both arguments (or at least one key thereof) are of different interfaces derived from
        /// #mi::IData_simple and there is no conversion between them. This happens for example
        /// if an instance of #mi::ISint32 is assigned to an instance of #mi::IString).
        NO_CONVERSION                    =  4,

        /// There is at least one key in \p source which does not exist in \p target.
        TARGET_KEY_MISSING               =  8,

        /// There is at least one key in \p target which does not exist in \p source.
        SOURCE_KEY_MISSING               = 16,

        /// Both arguments \p source and \p target (or the types of at least one key thereof) are
        /// collections, but of different type. For example, this flag is set when assigning from
        /// static to dynamic arrays, from arrays to maps, or from arrays or maps to compounds, or
        /// vice versa.
        DIFFERENT_COLLECTIONS            = 32,

        /// The argument \p source is a collection and contains at least one key which is not of
        /// type #mi::IData, e.g., in untyped arrays or maps. Or \p source is a pointer or contains
        /// a key that is a pointer that wraps an instance not derived from #mi::IData. Such types
        /// cannot get assigned with this method.
        NON_IDATA_VALUES                 = 64,

        /// Shallow assignment (the default) was requested and the assignment failed due to
        /// incompatible pointer types, e.g., assigning from #mi::IConst_pointer to
        /// #mi::IPointer, or between different typed pointers (#mi::IPointer::set_pointer() or
        /// #mi::IConst_pointer::set_pointer() failed).
        INCOMPATIBLE_POINTER_TYPES       = 128,

        /// Deep assignment was requested and \p target is an instance of #mi::IConst_pointer (or
        /// a key of \p target is an instance of #mi::IConst_pointer).
        DEEP_ASSIGNMENT_TO_CONST_POINTER = 256,

        /// The assignment failed due to incompatible privacy levels, i.e.,
        /// #mi::IRef::set_reference() returned error code -4.
        INCOMPATIBLE_PRIVACY_LEVELS      = 1024,

        /// The assignment failed due to incompatible enum types.
        INCOMPATIBLE_ENUM_TYPES          = 2048,

        /// The assignment failed due to incompatible options.
        INCOMPATIBLE_OPTIONS             = 4096,

        //  Undocumented, for alignment only
        FORCE_32_BIT_RESULT              = 0xffffffffU
    };

    /// This enum represents various options for the assignment or cloning of types.
    ///
    /// The enum values are powers of two such that they can be combined in a bitmask. Such a
    /// bitmask van be is passed to #assign_from_to() and #clone().
    enum Assign_clone_options
    {
        /// By default, assignment or cloning of instances of #mi::IPointer and #mi::IConst_pointer
        /// is shallow, i.e., the pointer represented by these interfaces is assigned or cloned as a
        /// plain value and both pointers point to the same interface afterwards. If this option is
        /// given the assignment or cloning is deep, i.e., the operations happens on the wrapped
        /// pointers themselves.
        DEEP_ASSIGNMENT_OR_CLONE = 1,

        /// By default, assignment might change the set of keys if the for target is a dynamic array
        /// or map.  If this option is given the set of keys in the target remains fixed.
        FIX_SET_OF_TARGET_KEYS = 4,

        //  Undocumented, for alignment only
        FORCE_32_BIT_OPTIONS     = 0xffffffffU
    };

    /// Assigns the value(s) of \p source to \p target.
    ///
    /// Assignment succeeds if both \p source and \p target are of the same type. If not, the
    /// method tries to perform an assignment on a best effort basis. For example, an assignment
    /// between interfaces derived from #mi::INumber might require a conversion as defined in the
    /// C/C++ standard. If both arguments are of type #mi::IData_collection, assignment happens
    /// for all keys existing in both parameters.
    ///
    /// \param source   The instance to assign from.
    /// \param target   The instance to assign to.
    /// \param options  See #Assign_clone_options for possible options.
    /// \return         The return value is a bit field that indicates which events occurred during
    ///                 assignment. See #Assign_result for possible events.
    ///                 \par
    ///                 The value 0 indicates that the assignment took place without any problems.
    ///                 Note that a non-zero value is not necessarily an error, it is up to the
    ///                 caller to interpret the result according to the context. For example,
    ///                 assigning from an instance of #mi::IFloat32_3 to an instance of
    ///                 #mi::IFloat32_2 will result in #TARGET_KEY_MISSING because there is no
    ///                 key named \c "z" in #mi::IFloat32_2. It is up to the caller to decide
    ///                 whether this is the intended behavior or not.
    virtual Uint32 assign_from_to( const IData* source, IData* target, Uint32 options = 0) = 0;

    /// Creates a clone of a type.
    ///
    /// It is not possible to clone untyped pointers and collections that wrap/contain interface
    /// pointers that are not of type #mi::IData.
    ///
    /// \param source   The instance to clone.
    /// \param options  See #Assign_clone_options for possible options.
    virtual IData* clone( const IData* source, Uint32 options = 0) = 0;

    /// Creates a clone of a type.
    ///
    /// It is not possible to clone untyped pointers and collections that wrap/contain interface
    /// pointers that are not of type #mi::IData.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience. It eliminates the need to call
    /// #mi::base::IInterface::get_interface(const Uuid&)
    /// on the returned pointer, since the return type already is a pointer to the type \p T
    /// specified as template parameter.
    ///
    /// \param source   The instance to clone.
    /// \param options  See #Assign_clone_options for possible options.
    /// \tparam T       The interface type of the class to create
    template<class T>
    T* clone( const IData* source, Uint32 options = 0)
    {
        mi::base::IInterface* ptr_iinterface = clone( source, options);
        if ( !ptr_iinterface)
            return 0;
        T* ptr_T = static_cast<T*>( ptr_iinterface->get_interface( typename T::IID()));
        ptr_iinterface->release();
        return ptr_T;
    }

    /// Compares two instances of #mi::IData.
    ///
    /// The comparison operator for instances of #mi::IData is defined as follows:
    /// - If \p lhs or \p rhs is \c NULL, the result is the lexicographic comparison of
    ///   the pointer addresses themselves.
    /// - Otherwise, the type names of \p lhs and \p rhs are considered. If they are different,
    ///   the result is determined by \c strcmp() on the type names. For example, this method
    ///   never returns 0 if you compare a dynamic array and a static array, even if they have
    ///   the same element type, length, and equal element values.
    /// - Finally, the values of the elements are compared lexicographically, as defined by
    ///   \c operator< or \c strcmp() with the following special cases for selected interfaces:
    ///   - #mi::IRef: If #mi::IRef::get_reference_name() returns \c NULL for at least one operand,
    ///     the result is the lexicographic comparison of the pointer addresses. Otherwise, the
    ///     result is determined by \c strcmp() on the names of the referenced DB elements.
    ///   - #mi::IPointer and #mi::IConst_pointer: If #mi::IPointer::get_pointer() or
    ///     #mi::IConst_pointer::get_pointer() returns \c NULL for at least one operand, the result
    ///     is the lexicographic comparison of the pointer addresses. Otherwise, if at least one
    ///     pointer is not of the type #mi::IData, the result is the lexicographic comparison of the
    ///     pointer addresses. Finally (both pointers are of type #mi::IData), the result is
    ///     determined by the recursive invocation of this method on these pointers.
    ///   - #mi::IData_collection: First, if the collections have different length, this decides
    ///     the comparison. Next, the elements are compared pairwise in lexicographic order of
    ///     the element index. If the element keys are different, \c strcmp() on the element keys
    ///     determines the result. Otherwise, the element values are compared by recursive
    ///     invocation of this method (or lexicographic comparison of the pointer addresses if one
    ///     element is not of type #mi::IData). If they are different, this decides the result,
    ///     otherwise the comparison continues with the next element. If all elements are equal,
    ///     the result is 0.
    ///
    /// \param lhs   The left-hand side operand for the comparison.
    /// \param rhs   The right-hand side operand for the comparison.
    /// \return      -1 if \c lhs < \c rhs, 0 if \c lhs == \c rhs, and +1 if \c lhs > \c rhs.
    virtual Sint32 compare( const IData* lhs, const IData* rhs) = 0;

    /// Returns a textual representation of a type.
    ///
    /// The textual representation is of the form "type name = value" if \p name is not \c NULL,
    /// and of the form "value" if \p name is \c NULL. The representation of the value might
    /// contain line breaks, for example for structures and arrays. Subsequent lines have a
    /// suitable indentation. The assumed indentation level of the first line is specified by
    /// \p depth.
    ///
    /// \ifnot DICE_API
    /// \see #mi::neuraylib::IFactory::dump(ITransaction*,const IData*,const char*,Size)
    /// \endif
    virtual const IString* dump( const IData* data, const char* name = 0, Size depth = 0) = 0;

    /// Returns a textual representation of a type.
    ///
    /// This overload of #mi::neuraylib::IFactory::dump(const IData*,const char*,Size) requires an
    /// additional transaction pointer. This is necessary to support the dumping of possibly nested
    /// non-#mi::IData types.
    virtual const IString* dump(
        neuraylib::ITransaction* transaction,
        const IData* data,
        const char* name = 0,
        Size depth = 0) = 0;

    /// Returns a registered structure declaration.
    ///
    /// \param structure_name   The name of the structure declaration to return.
    /// \return                 The structure declaration for \p name, or \c NULL if there is no
    ///                         structure declaration for that name.
    virtual const IStructure_decl* get_structure_decl( const char* structure_name) const = 0;

    /// Returns a registered enum declaration.
    ///
    /// \param enum_name        The name of the enum declaration to return.
    /// \return                 The enum declaration for \p name, or \c NULL if there is no
    ///                         enum declaration for that name.
    virtual const IEnum_decl* get_enum_decl( const char* enum_name) const = 0;
};

mi_static_assert( sizeof( IFactory::Assign_result) == sizeof( Uint32));
mi_static_assert( sizeof( IFactory::Assign_clone_options) == sizeof( Uint32));

/*@}*/ // end group mi_neuray_types

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IFACTORY_H
