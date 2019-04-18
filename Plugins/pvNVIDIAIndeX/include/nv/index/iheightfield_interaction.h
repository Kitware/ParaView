/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Cluster-wide interaction and computing tasks operating on heightfield data

#ifndef NVIDIA_INDEX_IHEIGHTFIELD_INTERACTION_H
#define NVIDIA_INDEX_IHEIGHTFIELD_INTERACTION_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/idistributed_compute_algorithm.h>
#include <nv/index/isnapping_algorithm.h>

namespace nv
{
namespace index
{

class IIndex_canvas;

/// Abstract base class enabling heightfield related workflow functionality
class IHeightfield_interaction : public mi::base::Interface_declare<0x6bf54fd7, 0x65cb, 0x4f88,
                                   0x8c, 0x19, 0x6f, 0xe7, 0x07, 0x81, 0x2e, 0x83>
{
public:
  /// Access the heightfield tag for which the present interface has been constructed.
  /// \return                         Unique tag that references the heightfield scene element.
  virtual mi::neuraylib::Tag_struct get_heightfield() const = 0;

  /// Manual pick operation applied to the referenced heightfield.
  ///
  /// \param[in] pixel_location       Pixel (x,y) coordinates
  /// \param[in] pick_canvas          The picking canvas for passing viewport information.
  /// \param[in] volume_tag           Volume scene element to which the picked slice belongs
  /// \param[in] snapping_algorithm   The snapping algorithm that calibrates the pick location
  ///                                 according to the volume amplitude values below and above
  ///                                 the intersection
  /// \param[in] dice_transaction     The DiCE transaction to use for performing the cluster-wide
  ///                                 pick operation
  virtual void pick(const mi::math::Vector_struct<mi::Uint32, 2>& pixel_location,
    const nv::index::IIndex_canvas* pick_canvas, mi::neuraylib::Tag_struct volume_tag,
    ISnapping_algorithm* snapping_algorithm,
    mi::neuraylib::IDice_transaction* dice_transaction) const = 0;

  /// Manual pick operation applied to the referenced heightfield and a specific slice.
  ///
  /// \param[in] pixel_location       Pixel (x,y) coordinates
  /// \param[in] pick_canvas          The picking canvas for passing viewport information.
  /// \param[in] slice_scene_element  Slice scene element to pick on
  /// \param[in] volume_tag           Volume scene element to which the slice belongs
  /// \param[in] snapping_algorithm   The snapping algorithm that calibrates the pick location
  ///                                 according to the volume amplitude values below and above
  ///                                 the intersection
  /// \param[in] dice_transaction     The DiCE transaction to use for performing the cluster-wide
  ///                                 pick operation
  virtual void pick(const mi::math::Vector_struct<mi::Uint32, 2>& pixel_location,
    const nv::index::IIndex_canvas* pick_canvas, mi::neuraylib::Tag_struct slice_scene_element,
    mi::neuraylib::Tag_struct volume_tag, ISnapping_algorithm* snapping_algorithm,
    mi::neuraylib::IDice_transaction* dice_transaction) const = 0;

  /// Invoke a user-defined (external) compute tasks that shall operate
  /// on the given heightfield.
  ///
  /// \param[in] distributed_compute_algorithm    Heightfield compute task
  ///                                             that shall operate on the given heightfield.
  /// \param[in] dice_transaction                 The DiCE transaction to use for performing the
  ///                                             cluster-wide execution of the compute task
  virtual void invoke_computing(IDistributed_compute_algorithm* distributed_compute_algorithm,
    mi::neuraylib::IDice_transaction* dice_transaction) const = 0;
};
}
} // namespace index / nv

#endif // NVIDIA_INDEX_IHEIGHTFIELD_INTERACTION_H
