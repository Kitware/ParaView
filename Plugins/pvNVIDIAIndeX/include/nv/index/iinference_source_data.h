/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

#ifndef NVIDIA_INDEX_IINFERENCE_SOURCE_DATA_H
#define NVIDIA_INDEX_IINFERENCE_SOURCE_DATA_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/iattribute.h>
#include <nv/index/idistributed_data_access.h>
#include <nv/index/idistributed_data_subset.h>

namespace nv
{
namespace index
{

/// Interface class exposes the source data that an inference technique can operate on.
///
/// The interface class is exposed through \c IDistributed_inference_technique.
///
class IInference_source_data : public mi::base::Interface_declare<0xe455b9a0, 0x9b5c, 0x4acb, 0xb6,
                                 0xcf, 0x31, 0x4b, 0x36, 0x70, 0x3a, 0xa7>
{
public:
  /// Get the bounding box of the subregion for which the inference technique has been invoked.
  ///
  /// The method returns the bounding box of the subregion for which
  /// the inference technique has been invoked by the NVIDIA IndeX system.
  /// The NVIDIA IndeX system initializes the bounding box that then can be accessed by an
  /// user-defined technique to control the inference parameter.
  ///
  /// \return     Returns the bounding box of the subregion that is defined in the subdivision
  /// space.
  ///
  virtual mi::math::Bbox_struct<mi::Float32, 3> get_subregion_bbox() const = 0;

  /// Get distributed data subset representing the data stored locally inside the subregion.
  ///
  /// Returns a subset of the distribute data that is contained in the local subregion.
  /// Subregions typically only store part subset of the entire distributed data. An inference
  /// technique
  /// may perform well on just the present subset.
  /// If the inference technique requires a larger extend or even the entire dataset, then the
  /// distributed data access needs to be called first (see \c create_distributed_data_access()).
  ///
  /// \return     Returns an interface pointer to an instance of \c IDistributed_data_subset.
  ///
  virtual const IDistributed_data_subset* get_distributed_data_subset() const = 0;

  /// Get interface for accessing distributed data.
  ///
  /// Returns s distribute data access interface for retrieve part or all of a dataset distributed
  /// in the cluster environment.
  ///
  /// \return     Returns an interface pointer to an instance of \c IDistributed_data_access.
  ///
  virtual IDistributed_data_access* get_distributed_data_access() const = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IINFERENCE_SOURCE_DATA_H
