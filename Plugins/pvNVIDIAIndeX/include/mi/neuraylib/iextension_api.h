/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief API component that allows extensions of the \neurayApiName.

#ifndef MI_NEURAYLIB_IEXTENSION_API_H
#define MI_NEURAYLIB_IEXTENSION_API_H

#include <mi/base/handle.h>
#include <mi/base/interface_declare.h>
#include <mi/neuraylib/iuser_class_factory.h>

namespace mi {

class IEnum_decl;
class IStructure_decl;

namespace neuraylib {

/** \addtogroup mi_neuray_configuration
@{
*/

class IExporter;
class IImporter;

/// This interface is used to extent the \neurayApiName.
/// For example it offers methods to load and to set up plugins.
class IExtension_api : public
    mi::base::Interface_declare<0xdf2dd31e,0xeeaf,0x40b2,0x8c,0x5f,0x0a,0xb2,0xad,0x44,0x61,0x91>
{
public:
    /// \name User classes
    //@{

    /// Registers a class with the \neurayApiName.
    ///
    /// All user-defined classes to be used with the \neurayApiName must be registered. The only
    /// exception are classes that never cross the API boundary, for example, classes only used
    /// locally within a plugin. Class registration must be done
    /// \ifnot MDL_SOURCE_RELEASE in #mi::neuraylib::IPlugin::init() or \endif
    /// before \neurayProductName has been started.
    ///
    /// \param class_name   The class name under which the class is to be registered. The class
    ///                     name must consist only of alphanumeric characters or underscores,
    ///                     must not start with an underscore, and must not be the empty string.
    /// \param uuid         The class ID of the class. You can simply pass IID() of your class
    ///                     derived from #mi::neuraylib::User_class.
    /// \param factory      The factory method of the class.
    /// \return
    ///                     -  0: Success.
    ///                     - -1: There is already a class or structure declaration registered
    ///                           under the name \p class_name or UUID \p uuid.
    ///                     - -2: Invalid parameters (\c NULL pointer).
    ///                     - -3: The method may not be called at that point of time.
    ///                     - -4: Invalid class name.
    virtual Sint32 register_class(
        const char* class_name, base::Uuid uuid, IUser_class_factory* factory) = 0;

    /// Registers a class with the \neurayApiName.
    ///
    /// All user-defined classes to be used with the \neurayApiName must be registered. The only
    /// exception are classes that never cross the API boundary, for example, classes only used
    /// locally within a plugin. Class registration must be done
    /// \ifnot MDL_SOURCE_RELEASE in #mi::neuraylib::IPlugin::init() or \endif
    /// before \neurayProductName has been started.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience. It uses the default class factory #mi::neuraylib::User_class_factory
    /// specialized for T.
    ///
    /// \param class_name   The class name under which the class is to be registered. The class
    ///                     name must consist only of alphanumeric characters or underscores,
    ///                     must not start with an underscore, and must not be the empty string.
    /// \return
    ///                     -  0: Success.
    ///                     - -1: There is already a class or structure declaration registered
    ///                           under the name \p class_name or UUID \p T::IID().
    ///                     - -2: Invalid parameters (\c NULL pointer).
    ///                     - -3: The method may not be called at that point of time.
    ///                     - -4: Invalid class name.
    template <class T>
    Sint32 register_class( const char* class_name)
    {
        mi::base::Handle<IUser_class_factory> factory( new User_class_factory<T>());
        return register_class( class_name, typename T::IID(), factory.get());
    }

    //@}
    /// \name Importers and exporters
    //@{

    /// Registers a new importer with the \neurayApiName.
    ///
    /// Importer registration must be done
    /// \ifnot MDL_SOURCE_RELEASE in #mi::neuraylib::IPlugin::init() or \endif 
    /// before \neurayProductName has been started.
    ///
    /// \param importer     The new importer to register.
    /// \return
    ///                     -  0: Success.
    ///                     - -1: Invalid parameters (\c NULL pointer).
    ///                     - -2: The method may not be called at that point of time.
    virtual Sint32 register_importer( IImporter* importer) = 0;

    /// Registers a new exporter with the \neurayApiName.
    ///
    /// Exporter registration must be done
    /// \ifnot MDL_SOURCE_RELEASE in #mi::neuraylib::IPlugin::init() or \endif 
    /// before \neurayProductName has been started.
    ///
    /// \param exporter     The new exporter to register.
    /// \return
    ///                     -  0: Success.
    ///                     - -1: Invalid parameters (\c NULL pointer).
    ///                     - -2: The method may not be called at that point of time.
    virtual Sint32 register_exporter( IExporter* exporter) = 0;

    /// Unregisters an importer registered with the \neurayApiName.
    ///
    /// Unregistration must be done
    /// \ifnot MDL_SOURCE_RELEASE in #mi::neuraylib::IPlugin::exit() or \endif
    /// after \neurayProductName has been shut down.
    ///
    /// \param importer     The importer to unregister.
    /// \return
    ///                     -  0: Success.
    ///                     - -1: Invalid parameters (\c NULL pointer).
    ///                     - -2: The method may not be called at that point of time.
    ///                     - -3: This importer is not registered.
    virtual Sint32 unregister_importer( IImporter* importer) = 0;

    /// Unregisters an exporter registered with the \neurayApiName.
    ///
    /// Unregistration must be done
    /// \ifnot MDL_SOURCE_RELEASE in #mi::neuraylib::IPlugin::exit() or \endif
    /// after \neurayProductName has been shut down.
    ///
    /// \param exporter     The exporter to unregister.
    /// \return
    ///                     -  0: Success.
    ///                     - -1: Invalid parameters (\c NULL pointer).
    ///                     - -2: The method may not be called at that point of time.
    ///                     - -3: This exporter is not registered.
    virtual Sint32 unregister_exporter( IExporter* exporter) = 0;

    //@}
    /// \name Structure declarations
    //@{

    /// Registers a structure declaration with the \neurayApiName.
    ///
    /// Note that the type names of the structure members are not checked for validity here
    /// (except for cycles). Thus, it is possible that this methods succeeds, but creating an
    /// instance of the type \p structure_name will fail.
    ///
    /// \param structure_name   The name to be used to refer to this structure declaration.
    ///                         The name must consist only of alphanumeric characters or
    ///                         underscores, must not start with an underscore, and must not be the
    ///                         empty string.
    /// \param decl             The structure declaration. The declaration is internally cloned such
    ///                         that subsequent changes have no effect on the registered
    ///                         declaration.
    /// \return
    ///                         -  0: Success.
    ///                         - -1: There is already a class, structure, enum, or call
    ///                               declaration registered under the name \p structure_name.
    ///                         - -2: Invalid parameters (\c NULL pointer).
    ///                         - -4: Invalid name for a structure declaration.
    ///                         - -5: A registration under the name \p structure_name would cause an
    ///                               infinite cycle of nested structure types.
    virtual Sint32 register_structure_decl(
        const char* structure_name, const IStructure_decl* decl) = 0;

    /// Unregisters a structure declaration with the \neurayApiName.
    ///
    /// \param structure_name   The name of the structure declaration to be unregistered.
    /// \return
    ///                         -  0: Success.
    ///                         - -1: There is no structure declaration registered under the name
    ///                               \p structure_name.
    ///                         - -2: Invalid parameters (\c NULL pointer).
    ///                         - -4: Invalid name for a structure declaration.
    ///                         - -6: The structure declaration is predefined and cannot be
    ///                               unregistered.
    virtual Sint32 unregister_structure_decl( const char* structure_name) = 0;

    //@}
    /// \name Enum declarations
    //@{

    /// Registers an enum declaration with the \neurayApiName.
    ///
    /// \param enum_name        The name to be used to refer to this enum declaration.
    ///                         The name must consist only of alphanumeric characters or
    ///                         underscores, must not start with an underscore, and must not be the
    ///                         empty string.
    /// \param decl             The enum declaration. The declaration is internally cloned such
    ///                         that subsequent changes have no effect on the registered
    ///                         declaration.
    /// \return
    ///                         -  0: Success.
    ///                         - -1: There is already a class, structure, enum, or call
    ///                               declaration registered under the name \p enum_name.
    ///                         - -2: Invalid parameters (\c NULL pointer).
    ///                         - -4: Invalid name for an enum declaration.
    ///                         - -6: The enum declaration is empty and therefore invalid.
    virtual Sint32 register_enum_decl(
        const char* enum_name, const IEnum_decl* decl) = 0;

    /// Unregisters an enum declaration with the \neurayApiName.
    ///
    /// \param enum_name        The name of the enum declaration to be unregistered.
    /// \return
    ///                         -  0: Success.
    ///                         - -1: There is no enum declaration registered under the name
    ///                               \p enum_name.
    ///                         - -2: Invalid parameters (\c NULL pointer).
    ///                         - -4: Invalid name for an enum declaration.
    virtual Sint32 unregister_enum_decl( const char* enum_name) = 0;

    //@}
};

/*@}*/ // end group mi_neuray_configuration

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IEXTENSION_API_H
