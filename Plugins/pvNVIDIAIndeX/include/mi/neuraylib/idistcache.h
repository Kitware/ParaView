/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Distributed cache service.

#ifndef MI_NEURAYLIB_IDISTCACHE_H
#define MI_NEURAYLIB_IDISTCACHE_H

#include <mi/base/interface_declare.h>

namespace mi
{

namespace neuraylib
{

class IDeserializer;
class ISerializable;

/** \addtogroup mi_neuray_dice
@{
*/

/// The object receiver is responsible for creation and destruction of objects on the remote
/// host.
///
/// The distributed cache can handle an arbitrary number of such receivers, but at most one
/// receiver per class ID.
class IObject_receiver : public base::Interface_declare<0x61820375, 0xbc2e, 0x423a, 0x9f, 0x12,
                           0x4b, 0xff, 0xb2, 0x25, 0xea, 0x0d>
{
public:
  /// Callback for object creation.
  ///
  /// This method is called on each remote host in the cluster or network when some host stores
  /// an object in the distributed cache.
  ///
  /// Note that the data available from the deserializer was written by calling the \c serialize()
  /// function on the object passed to the #mi::neuraylib::IDistributed_cache::store_object()
  /// function. That means that the cache guarantees that the deserializer here is in a state that
  /// a call to the \c deserialize() function of the correct class for the given class ID would
  /// work correctly.
  ///
  /// \param class_id         The class ID of the object, used to find the receiver and to tell
  ///                         the receiver which object to create.
  /// \param deserializer     The receiver can read the data from this deserializer.
  /// \return                 A unique object ID assigned by the creator which identifies the
  ///                         object and will be passed into the destroy_object() call.
  virtual Uint64 create_object(base::Uuid class_id, IDeserializer* deserializer) = 0;

  /// Callback for object destruction.
  ///
  /// This method is called on those remote hosts which have received a corresponding
  /// #create_object() call before. It will be called when either the owner removes the object
  /// or when the owner leaves the network.
  ///
  /// \param id               The object ID assigned by the receiver and returned from a previous
  ///                         create_object() call.
  /// \param owner_died       A flag indicating whether the object was removed or the owner died.
  virtual void destroy_object(Uint64 id, bool owner_died) = 0;
};

/// The distributed cache service can be used to create temporary objects in a cluster.
///
///
/// The distributed cache manages the distribution of objects and triggers the destruction of
/// objects. The objects will exist on every host in the cluster. The creating host is the owner of
/// an object, each object can have only one owner. It will be destroyed on remote hosts when the
/// owner removes the object or when the owner leaves the network. A stored object can be changed
/// but no synchronization of changes with other hosts is done.
class IDistributed_cache : public base::Interface_declare<0x29582fa9, 0xb13e, 0x4a48, 0xa2, 0x2d,
                             0xfd, 0x22, 0x33, 0x70, 0x40, 0x88>
{
public:
  /// Registers a receiver with the distributed cache.
  ///
  /// Each receiver will be responsible for one or more class IDs. It will be invoked when a
  /// remote host stored an object with one of the class IDs the receiver is responsible for.
  /// It will also be invoked when the object needs to be destroyed.
  ///
  /// A previously registered receiver for this class ID will be unregistered.
  ///
  /// \param class_id   ID of the class this receiver will handle
  /// \param receiver   The receiver object
  virtual void register_receiver(base::Uuid class_id, IObject_receiver* receiver) = 0;

  /// Unregisters a receiver with the distributed cache.
  ///
  /// The cache will then remove all information about all objects of the given class ID to avoid
  /// calling the receiver in a later \c destroy_object() call. It will not call the
  /// \c destroy_object() function for any one of the removed objects during removal of the
  /// information about these class IDs.
  ///
  /// \param class_id   ID of the class this receiver handled
  virtual void unregister_receiver(base::Uuid class_id) = 0;

  /// Stores an object in the distributed cache.
  ///
  /// The distributed cache notifies all remote hosts in either the whole network or in the
  /// cluster only (default) provided they registered a receiver for the corresponding class ID.
  /// Hosts which join after the object was stored but before the object was removed will also
  /// get notified. \note Due to a current limitation this notification is currently missing.
  ///
  /// \param serializable   The object to store
  /// \param cluster_only   The flag indicates if the object is stored globally or only in the
  ///                       cluster.
  /// \return
  ///                       -  0: Success.
  ///                       - -1: The serializable has not been registered (see
  ///                         #mi::neuraylib::IDice_configuration::register_serializable_class()).
  virtual Sint32 store_object(ISerializable* serializable, bool cluster_only = true) = 0;

  /// Removes an object from the cache.
  ///
  /// The distributed cache notifies all hosts which received the notification from the
  /// #store_object() call. The object itself is not destroyed or otherwise touched in any way.
  ///
  /// \param serializable         The object to remove
  virtual void remove_object(ISerializable* serializable) = 0;
};

/*@}*/ // end group mi_neuray_dice

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IDISTCACHE_H
