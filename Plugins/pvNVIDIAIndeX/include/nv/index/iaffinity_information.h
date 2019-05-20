/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Defines the affinity of spatial areas to machines/GPUs in the cluster.

#ifndef NVIDIA_INDEX_IAFFINITY_INFORMATION_H
#define NVIDIA_INDEX_IAFFINITY_INFORMATION_H

#include <mi/math/bbox.h>
#include <mi/neuraylib/dice.h>

namespace nv
{
namespace index
{

/// Application-side control of data distribution to cluster machines and GPUs.
///
/// NVIDIA IndeX applies a sort-last spatial decomposition for rendering that is based on a
/// given subregion size (see \c IConfig_settings::set_subcube_configuration)
/// and that may be different to an application's decomposition scheme. This
/// interface class enables the user to steer the data distribution and assign a
/// subregion (or spatial region) to a certain machine and to a certain GPU in the cluster.
///
/// As a requirement for correct affinity usage the domain-specific subdivision
/// and the spatial subdivision from NVIDIA IndeX have to result in the same
/// spatial regions (bounding boxes). At least the domain-specific spatial regions
/// for which an explicit affinity is given have to match or include (cover) one or a
/// subset of bounding boxes that are created by NVIDIA IndeX. The sub cube
/// configuration defines the size of the bounding boxes created by NVIDIA IndeX
/// and allows to optimize the alignment of the internal spatial decomposition to
/// a given domain-specific spatial decomposition.
///
/// @ingroup nv_index_data_storage
///
class IAffinity_information
  : public mi::base::Interface_declare<0x3fbeb811, 0xffd1, 0x4521, 0x33, 0x21, 0xbd, 0x67, 0x12,
      0x21, 0x11, 0x69, mi::neuraylib::ISerializable>
{
public:
  /// Flags for setting specific affinity modes.
  enum Affinity_flags
  {
    ANY_GPU = 0xffffffff ///< The GPU should be assigned automatically.
  };

  /// Defines the affinity for a subregion with the given bounding box. The
  /// affinity includes the cluster machine as well as the CUDA device id on that
  /// machine. If neither the id of the cluster machine nor the id of the CUDA device
  /// is valid (e.g., the host is not listed in cluster) then the affinity
  /// definition is ignored.
  ///
  /// \param[in]  subregion       The bounding box of the subregion. The affinity for this
  ///                             subregion is to be provided by the user.
  ///
  /// \param[out] host_id         The cluster machine (host) where the subregion is supposed to be
  ///                             stored, processed and rendered.
  ///
  /// \param[out] device_id       The CUDA device id on the given cluster machine where the
  /// subregion is
  ///                             supposed to be stored, processed and rendered. If set to \c
  ///                             ANY_GPU
  ///                             then the GPU will be chosen automatically.
  ///
  /// \return true if affinity information is supplied, and false if \c host_id
  ///         and \c gpu_id should be ignored.
  ///
  virtual bool get_affinity(const mi::math::Bbox_struct<mi::Float32, 3>& subregion,
    mi::Uint32& host_id, mi::Uint32& device_id) const = 0;
};

/// Domain specific subdivision and data distribution.
///
/// Compute algorithms usually define their own domain-specific spatial subdivision
/// and data distribution schemes to efficiently run, for instance, compute algorithms.
/// In general, NVIDIA IndeX applies an internal sort-last approach that subdivides
/// space into an spatial decomposition that is optimal for rendering.
/// The present interface class allows to adjust the internal subdivision scheme
/// in such a way that an external decomposition scheme is covered by NVIDIA IndeX's
/// internal representation.
///
/// The application-domain is expected to generate 3D spatial regions. These regions
/// are supposed to be disjoint. In case of an hierarchical decomposition scheme, such as a
/// kd-tree based spatial decomposition, these regions typically represent the tress, leaf
/// nodes. The interface class shall provide the regions as 3D bounding boxes so that
/// NVIDIA IndeX can align its internal representation.
///
/// This interface class also extents the affinity class that is the application is
/// also able to direct the NVIDIA IndeX's spatial regions to machines and GPUs
/// in the cluster where application/algorithm data is already stored.
///
/// EXPERIMENTAL This feature is in an experimental state. Decent testing is
/// required and interfaces might change in the future.
///
/// @ingroup nv_index_data_storage
///
class IDomain_specific_subdivision
  : public mi::base::Interface_declare<0x1fefb212, 0xffe1, 0x1431, 0x13, 0x67, 0xbd, 0x17, 0x13,
      0x31, 0x41, 0x6e, nv::index::IAffinity_information>
{
public:
  /// Provides the number of spatial regions that span the application's decomposition scheme.
  /// NVIDIA IndeX requires the number of regions to iterate through them using
  /// \c get_subregion().
  ///
  /// \return         The number of the domain-specific spatial regions.
  ///
  virtual mi::Uint32 get_nb_subregions() const = 0;

  /// Provides each spatial regions according to on the given index.
  /// Each regions is shall be represented as a bounding box. NVIDIA IndeX
  /// aligns its internal spatial subdivision scheme to the application-supplied
  /// bounding box to cover the entire space.
  ///
  /// \param[in]  index       An index that allows accessing the application-supplied
  ///                         spatial areas. The index must be in the range
  ///                         [0,\c get_nb_subregions()].
  ///
  /// \return Returns the spatial regions as bounding box.
  ///
  virtual mi::math::Bbox_struct<mi::Float32, 3> get_subregion(mi::Uint32 index) const = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IAFFINITY_INFORMATION_H
