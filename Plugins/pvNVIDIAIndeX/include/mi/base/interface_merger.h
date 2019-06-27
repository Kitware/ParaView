/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file mi/base/interface_merger.h
/// \brief Mixin class template to merge an interface with an implementation.

#ifndef MI_BASE_INTERFACE_MERGER_H
#define MI_BASE_INTERFACE_MERGER_H

#include <mi/base/iinterface.h>

namespace mi {
namespace base {

/** \addtogroup mi_base_iinterface
@{
*/

/// This mixin merges the implementation of one interface with a second interface.
///
/// Multiple inheritance from interfaces is not possible due to ambiguities. This mixin resolves
/// the ambiguities by re-implementing the methods of #mi::base::IInterface.
///
/// Note that this mixin class derives from an implementation and an interface, in contrast to
/// #mi::base::Interface_implement_2, which derives from two interfaces. Basically,
/// \c %Interface_merger<Interface_implement<I1>,I2> is equivalent to \c Interface_implement<I1,I2>.
/// The benefit of this mixin class is that you are not bound to use \c %Interface_implement, any
/// valid implementation of an interface will do.
///
/// The template parameters representing the base classes are called MAJOR and MINOR. Note that
/// the derivation is not symmetric:
/// - MAJOR has to be an implementation of an interface, MINOR has to be just an interface
/// - \c retain() and \c release() are redirected to the MAJOR %base class
/// - \c compared_iid() and \c get_interface() are redirected to the MAJOR %base class first, the
///   MINOR %base class is only queried on failure. For example,
///   \c get_interface<mi::base::IInterface>() will always return the MAJOR %base class.
/// - \c get_iid() is redirected to the MAJOR %base class.
///
/// Note that some versions of Microsoft Visual C++ are known to produce the bogus warning
/// \code disable C4505: <class::method>: unreferenced local function has been removed \endcode
/// in conjunction with this class. This warning can be disabled with
/// \code
/// #include <mi/base/config.h>
/// #ifdef MI_COMPILER_MSC
/// #pragma warning( disable : 4505 )
/// #endif
/// \endcode
/// Note that the pragma affects the entire translation unit. Therefore, the warning is not disabled
/// by default.
///
/// \tparam MAJOR   the implementation %base class
/// \tparam MINOR   the interface %base class
template <typename MAJOR, typename MINOR>
class Interface_merger
  : public MAJOR,
    public MINOR
{
public:
    /// Typedef for the MAJOR %base class
    typedef MAJOR MAJOR_BASE;
    
    /// Typedef for the MINOR %base class
    typedef MINOR MINOR_BASE;

    /// Reimplements #mi::base::IInterface::compare_iid().
    ///
    /// Forwards the call to the MAJOR %base class, and then, in case of failure, to the
    /// MINOR %base class.
    static bool compare_iid( const Uuid& iid) {
        if( MAJOR::compare_iid( iid))
            return true;
        return MINOR::compare_iid( iid);
    }

    /// Reimplements #mi::base::IInterface::get_interface(const Uuid&) const.
    ///
    /// Forwards the call to the MAJOR %base class, and then, in case of failure, to the
    /// MINOR %base class.
    const IInterface* get_interface( const Uuid& interface_id) const;

    /// Reimplements #mi::base::IInterface::get_interface() const.
    ///
    /// The implementation is identical, but needed for visibility reasons.
    template <class T>
    const T* get_interface() const {
        return static_cast<const T*>( get_interface( typename T::IID()));
    }

    /// Reimplements #mi::base::IInterface::get_interface(const Uuid&).
    ///
    /// Forwards the call to the MAJOR %base class, and then, in case of failure, to the
    /// MINOR %base class.
    IInterface* get_interface( const Uuid& interface_id);

    /// Reimplements #mi::base::IInterface::get_interface().
    ///
    /// The implementation is identical, but needed for visibility reasons.
    template <class T>
    T* get_interface() {
        return static_cast<T*>( get_interface( typename T::IID()));
    }

    /// Reimplements #mi::base::IInterface::get_iid().
    ///
    /// Forwards the call to the MAJOR %base class.
    Uuid get_iid() const {
        return MAJOR::get_iid();
    }

    /// Reimplements #mi::base::IInterface::retain().
    ///
    /// Forwards the call to the MAJOR %base class.
    mi::Uint32 retain() const {
        return MAJOR::retain();
    }

    /// Reimplements #mi::base::IInterface::release().
    ///
    /// Forwards the call to the MAJOR %base class.
    mi::Uint32 release() const {
        return MAJOR::release();
    }

    /// Returns a pointer to the MAJOR %base class.
    ///
    /// Note that #mi::base::IInterface is an ambiguous %base class. Often you just need a pointer
    /// to #mi::base::IInterface but do not really care to which %base class it actually points to.
    /// This method is intended to be used in these cases. It returns the pointer to the MAJOR
    /// %base class, which can then be statically casted to #mi::base::IInterface (unless
    /// Interface_merger is nested).
    ///
    /// \note The name \c cast_to_major() of this method emphasizes that it behaves similar to a
    ///       static cast, in particular, it does not increase the reference count.
    const MAJOR* cast_to_major() const {
        return static_cast<const MAJOR*>( this);
    }

    /// Returns a pointer to the MAJOR %base class.
    ///
    /// Note that #mi::base::IInterface is an ambiguous %base class. Often you just need a pointer
    /// to #mi::base::IInterface but do not really care to which %base class it actually points to.
    /// This method is intended to be used in these cases. It returns the pointer to the MAJOR
    /// %base class, which can then be statically casted to #mi::base::IInterface (unless
    /// Interface_merger is nested).
    ///
    /// \note The name \c cast_to_major() of this method emphasizes that it behaves similar to a
    ///       static cast, in particular, it does not increase the reference count.
    MAJOR* cast_to_major() {
        return static_cast<MAJOR*>( this);
    }
};

template <typename MAJOR, typename MINOR>
const IInterface* Interface_merger<MAJOR,MINOR>::get_interface(
    const Uuid& interface_id) const
{
    const IInterface* iinterface = MAJOR::get_interface( interface_id);
    if( iinterface)
        return iinterface;
    return MINOR::get_interface_static( static_cast<const MINOR*>( this), interface_id);
}

template <typename MAJOR, typename MINOR>
IInterface* Interface_merger<MAJOR,MINOR>::get_interface(
    const Uuid& interface_id)
{
    IInterface* iinterface = MAJOR::get_interface( interface_id);
    if( iinterface)
        return iinterface;
    return MINOR::get_interface_static( static_cast<MINOR*>( this), interface_id);
}

/*@}*/ // end group mi_base_iinterface

} // namespace base
} // namespace mi

#endif // MI_BASE_INTERFACE_MERGER_H
