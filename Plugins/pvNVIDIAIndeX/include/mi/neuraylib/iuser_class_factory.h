/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Abstract interface for factories for user-defined class.

#ifndef MI_NEURAYLIB_IUSER_CLASS_FACTORY_H
#define MI_NEURAYLIB_IUSER_CLASS_FACTORY_H

#include <mi/base/interface_declare.h>
#include <mi/base/interface_implement.h>

namespace mi
{

namespace neuraylib
{

/** \addtogroup mi_neuray_plugins
@{
*/

class ITransaction;

/// Abstract interface for user class factories.
///
/// The registration of a user-defined class requires a factory which constructs an instance of the
/// class. The factory is passed during class
/// registration, see \if IRAY_API #mi::neuraylib::IExtension_api::register_class(). \else
/// #mi::neuraylib::IDice_configuration::register_serializable_class(). \endif
class IUser_class_factory : public mi::base::Interface_declare<0x37355ece, 0x2ed7, 0x4158, 0x88,
                              0x35, 0xb8, 0x60, 0xaf, 0x75, 0x6a, 0x64>
{
public:
  /// Creates an instance of the class for which the factory was registered.
  ///
  /// Each class factory is free to decide which values and types of \p argc and \p argv are
  /// valid. However, it must handle the case \p transaction = \c NULL, \p argc = 0, and
  /// \p argv = \c NULL which is needed for deserialization. \if IRAY_API In that case, the
  /// method #mi::neuraylib::ISerializable::deserialize() is called afterwards. \endif
  ///
  /// \param transaction   The transaction in which the instance is created. The transaction can
  ///                      be passed to the class constructor in case you want to create other
  ///                      class instances. Note that the transaction pointer must not be
  ///                      serialized/deserialized \if IRAY_API and it must not be used in
  ///                      #mi::neuraylib::IUser_class::copy() (because the copy might live in
  ///                      another transaction). \endif
  /// \param argc          The size of the \p argv array.
  /// \param argv          An array of optional arguments.
  /// \return              An instance of the class, or \c NULL on failure.
  virtual base::IInterface* create(
    ITransaction* transaction, Uint32 argc, const base::IInterface* argv[]) = 0;
};

/// This mixin class provides a default implementation of the \c %IUser_class_factory interface.
///
/// This default implementation of #mi::neuraylib::IUser_class_factory simply calls the default
/// constructor of T without arguments.
///
/// The default implementation is used implicitly by some variants of the class registration,
/// see \if IRAY_API #mi::neuraylib::IExtension_api::register_class(const char*). \else
/// #mi::neuraylib::IDice_configuration::register_serializable_class(). \endif
template <class T>
class User_class_factory : public mi::base::Interface_implement<neuraylib::IUser_class_factory>
{
public:
  /// Creates an instance of the class for which the factory was registered.
  ///
  /// This default implementation simply calls the default constructor of T without arguments.
  /// It does not accept any parameters, i.e., it requires \c argc = 0.
  ///
  /// \param transaction   The transaction (ignored).
  /// \param argc          The size of the \p argv array (must be 0).
  /// \param argv          An array of optional arguments (ignored).
  /// \return              An instance of the class, or \c NULL on failure.
  base::IInterface* create(ITransaction* transaction, Uint32 argc, const base::IInterface* argv[])
  {
    // avoid warnings
    (void)transaction;
    (void)argc;
    (void)argv;

    if (argc != 0)
      return 0;
    return new T;
  }
};

/*@}*/ // end group mi_neuray_plugins

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IUSER_CLASS_FACTORY_H
