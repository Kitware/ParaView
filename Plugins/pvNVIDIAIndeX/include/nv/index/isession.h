/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Interface for a session

#ifndef NVIDIA_INDEX_ISESSION_H
#define NVIDIA_INDEX_ISESSION_H

#include <mi/base/handle.h>
#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <mi/math/matrix.h>
#include <mi/math/vector.h>

#include <nv/index/icamera.h>
#include <nv/index/iindex.h>
#include <nv/index/iscene_convenience_manipulation.h>
#include <nv/index/iscene_visitor.h>

namespace nv
{

namespace index
{

class IViewport;
class IViewport_list;

/// @ingroup nv_index

/// The abstract interface class representing a working session with the system.
/// Such a \e session contains a configuration (\c IConfig_settings) and an scene
/// (\c IScene).
///
/// In addition, this class makes available several factory methods, e.g., for creating a scene
/// or a camera.
///
class ISession : public mi::base::Interface_declare<0x21638be2, 0xab6b, 0x4396, 0x9a, 0xb7, 0x3b,
                   0xa5, 0xe2, 0xc9, 0xe0, 0xe6, mi::neuraylib::IElement>
{
public:
  ///////////////////////////////////////////////////////////////////////////////////
  /// \name Creating a camera.
  ///@{
  /// Creates a new camera object.
  ///
  /// \param[in]  dice_transaction    The DiCE transaction to store the new camera in
  ///                                 DiCE's distributed database.
  /// \param[in]  camera_type_iid     The UUID of the requested camera to be created.
  ///                                 By default a \c IPerspective_camera instance
  ///                                 is created.
  /// \param[in]  camera_name         The name for this camera. This name can be used
  ///                                 to lookup the camera tag using
  ///                                 IDice_transaction::name_to_tag().
  ///                                 When it is 0, no name is assigned.
  ///
  /// \return     Returns the tag that references the created instance of the \c ICamera interface
  /// camera.
  ///             The caller takes ownership of the camera and is
  ///             responsible for freeing it from the database by calling \c
  ///             IDice_transaction::remove() when it is not needed anymore.
  ///
  virtual mi::neuraylib::Tag_struct create_camera(
    mi::neuraylib::IDice_transaction* dice_transaction,
    const mi::base::Uuid& camera_type_uuid = IPerspective_camera::IID(),
    const char* camera_name = 0) const = 0;
  ///@}

  ///////////////////////////////////////////////////////////////////////////////////
  /// \name Multi-view support.
  ///@{
  /// Creates a new viewport.
  ///
  /// \return     Returns the new \c IViewport object. The caller
  ///             takes ownership of the viewport object and is
  ///             responsible for freeing it when it is not needed
  ///             anymore, i.e. by storing it in a \c mi::base::Handle.
  ///
  virtual IViewport* create_viewport() const = 0;

  /// Creates a new viewport list.
  ///
  /// \return     Returns the new \c IViewport_list object. The caller
  ///             takes ownership of the viewport object and is
  ///             responsible for freeing it when it is not needed
  ///             anymore, i.e. by storing it in a \c mi::base::Handle.
  ///
  virtual IViewport_list* create_viewport_list() const = 0;

  /// Creates a new list of canvas/viewport-list pairs.
  ///
  /// \return     Returns the new \c ICanvas_viewport_list object. The caller
  ///             takes ownership of the returned object and is
  ///             responsible for freeing it when it is not needed
  ///             anymore, i.e. by storing it in a \c mi::base::Handle.
  ///
  virtual ICanvas_viewport_list* create_canvas_viewport_list() const = 0;

  ///@}

  ///////////////////////////////////////////////////////////////////////////////////
  /// \name Creating a scene.
  ///@{
  /// Creates a new scene for the current session.
  ///
  /// \note You can only create a single scene for each session.
  ///
  /// \param[in] dice_transaction     The DiCE transaction to store the scene in DiCE's
  ///                                 distributed database.
  ///
  /// \return                         Returns the tag that references the created instance
  ///                                 of a \c IScene interface class.
  ///
  virtual mi::neuraylib::Tag_struct create_scene(
    mi::neuraylib::IDice_transaction* dice_transaction) = 0;

  /// Returns the scene of the current session.
  /// \return Tag of an \c IScene
  ///
  virtual mi::neuraylib::Tag_struct get_scene() const = 0;
  ///@}

  ///////////////////////////////////////////////////////////////////////////////////
  /// \name Traversing the scene.
  ///@{

  /// Visiting the scene by traversing through the entire scene description.
  ///
  /// \param[in] visitor              The application-side visitor traverses through
  ///                                 the scene description and evaluates all scene
  ///                                 elements.
  ///
  ///                                 \c TODO: The traversal order such as depth or
  ///                                 breadth first shall be provided by the application
  ///                                 in the future.
  ///
  /// \param[in] dice_transaction     The DiCE transaction that should be used for the
  ///                                 traversal.
  ///
  virtual void visit(
    IScene_visitor* visitor, mi::neuraylib::IDice_transaction* dice_transaction) const = 0;
  ///@}

  ///////////////////////////////////////////////////////////////////////////////////
  /// \name Convenience interface class.
  ///@{
  /// The convenience interface class allows an application-writer to perform common manipulations
  /// on a scene's configuration and, thus, reduces the efforts for tinker with the hierarchical
  /// scene description.
  /// Nonetheless, defining the scene description explicitly and managing the scene in the
  /// application logic represents the method of choice.
  ///
  /// \return Returns an instance of the interface class \c IScene_convenience_manipulation, which
  ///         exposes means for conveniently manipulating a scene's configuration.
  ///
  virtual IScene_convenience_manipulation* get_conveniences() const = 0;
  ///@}

  ///////////////////////////////////////////////////////////////////////////////////
  /// \name Data distribution.
  ///@{
  /// Returns the factory that exposes access to the distributed large-scale datasets.
  ///
  /// \return     Returns the tag that references an instance of the \c
  /// IDistributed_data_access_factory
  ///             interface class.
  ///
  virtual mi::neuraylib::Tag_struct get_data_access_factory() const = 0;

  /// Returns the object that exposes the distribution of large-scale data in the cluster
  /// environment.
  ///
  /// \return     Returns the tag that reference an instance of the \c IData_distribution
  ///             interface class.
  ///
  virtual mi::neuraylib::Tag_struct get_distribution_layout() const = 0;
  ///@}

  ///////////////////////////////////////////////////////////////////////////////////
  /// \name Global configurations and states.
  ///@{
  /// Returns the configuration settings of this  session.
  /// \return Tag of an \c IConfig_settings object
  ///
  virtual mi::neuraylib::Tag_struct get_config() const = 0;

  /// Controls how the export will be performed by export_session().
  enum Export_mode
  {
    /// .prj output format for use with the NVIDIA IndeX demo viewer
    EXPORT_FORMAT_PRJ = 0x01,
    /// JSON output format
    EXPORT_FORMAT_JSON = 0x02,
    /// Show system information
    EXPORT_SYSINFO = 0x10,
    /// Add memory statistics to system information
    EXPORT_MEMORY = 0x20,
    /// Add additional comments
    EXPORT_VERBOSE = 0x40,
    /// Add additional debug information as comments
    EXPORT_DEBUG = 0x80,
    /// Add type and range hints to properties (for building a UI)
    EXPORT_HINTS = 0x100,
    /// Add detailed output of user-defined data (e.g. source code and parameters from rendering
    /// kernel programs)
    EXPORT_USER_DATA = 0x200,
    /// Default mode
    EXPORT_DEFAULT =
      EXPORT_FORMAT_PRJ | EXPORT_SYSINFO | EXPORT_MEMORY | EXPORT_VERBOSE | EXPORT_DEBUG
  };

  /// Returns a textual representation of the internal state of the scene and
  /// the rendering parameters that make up this session. The output format
  /// and the amount of details are controlled by the export_mode parameter.
  ///
  /// \param[in] export_mode      Combination (bitwise or) of values from Export_mode.
  /// \param[in] output           String where the information should be written to.
  /// \param[in] dice_transaction DiCE transaction to use.
  /// \param[in] viewport_list    The scene of each viewport will be export, or, if set
  ///                             to 0, only the scene in the global scope.
  ///
  /// \par Usage example:
  ///
  /// Consider that \c m_session_tag contains the tag of an ISession and
  /// \c m_index points to an instance of IIndex.
  ///
  /// \code
  /// // Access session
  /// mi::base::Handle<const nv::index::ISession> session(
  ///     dice_transaction->access<const nv::index::ISession>(m_session_tag));
  ///
  /// // Create string object that should be filled with the state information
  /// mi::base::Handle<mi::neuraylib::IFactory> factory(
  ///     m_index->get_api_component<mi::neuraylib::IFactory>());
  /// mi::base::Handle<mi::IString> str(factory->create<mi::IString>());
  ///
  /// // Write state to the string object
  /// session->export_session(nv::index::ISession::EXPORT_DEFAULT, str.get(),
  /// dice_transaction.get());
  /// std::cout << str->get_c_str() << std::endl;
  /// \endcode
  ///
  virtual void export_session(mi::Uint32 export_mode, mi::IString* output,
    mi::neuraylib::IDice_transaction* dice_transaction,
    const IViewport_list* viewport_list = 0) const = 0;

  /// Returns a textual representation of the internal state of the given
  /// scene element.
  ///
  /// \see export_session()
  ///
  /// \param[in] scene_element_tag Tag of the IScene_element to be exported.
  /// \param[in] export_mode       Combination (bitwise or) of values from Export_mode.
  /// \param[in] output            String where the information should be written to.
  /// \param[in] dice_transaction  DiCE transaction to use.
  //
  virtual void export_scene_element(mi::neuraylib::Tag_struct scene_element_tag,
    mi::Uint32 export_mode, mi::IString* output,
    mi::neuraylib::IDice_transaction* dice_transaction) const = 0;
  ///@}
};
}
} // namespace index / nv

#endif // NVIDIA_INDEX_ISESSION_H
