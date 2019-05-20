/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief The scene query API of the NVIDIA IndeX library.

#ifndef NVIDIA_INDEX_IINDEX_SCENE_QUERY_H
#define NVIDIA_INDEX_IINDEX_SCENE_QUERY_H

#include <nv/index/iscene_query_results.h>

namespace nv
{
namespace index
{

class IIndex_canvas;
class IViewport_list;

/// @ingroup scene_queries

/// Interface class that enables application writers to query the scene's contents.
/// Scene queries include, for instance, the pick operation, which is the task of
/// determining which rendered scene element a user has clicked on, or
/// entry lookup, which returns a single entry of a dataset.
///
class IIndex_scene_query : public mi::base::Interface_declare<0xd71c4a09, 0xd2ac, 0x428e, 0xbf,
                             0x6e, 0x01, 0x5f, 0x9f, 0xe3, 0xfa, 0x35>
{
public:
  /// Returns all picked scene elements at a certain pixel location and
  /// their intersection with the ray cast from the center of the camera through
  /// the pixel.
  ///
  /// \param[in] pixel_location           Pixel location on the canvas in screen space.
  /// \param[in] pick_canvas              Canvas that should be picked.
  /// \param[in] session_tag              \c ISession tag.
  /// \param[in] dice_transaction         DiCE transaction.
  ///
  /// \return                             Pick results in front-to-back order.
  ///
  virtual IScene_pick_results* pick(const mi::math::Vector_struct<mi::Uint32, 2>& pixel_location,
    const nv::index::IIndex_canvas* pick_canvas, mi::neuraylib::Tag_struct session_tag,
    mi::neuraylib::IDice_transaction* dice_transaction) const = 0;

  /// Returns a list of all picked scene elements at a certain pixel location and
  /// their intersection with the ray cast from the center of the camera through
  /// the pixel. When the picking location is covered by multiple
  /// overlapping viewports, then the multiple \c IScene_lookup_result entries will
  /// be returned.
  ///
  /// \param[in] pixel_location           Pixel location on the canvas in screen space.
  /// \param[in] pick_canvas              Canvas that should be picked.
  /// \param[in] viewport_list            Viewport list for the picking viewport information.
  /// \param[in] session_tag              The \c ISession tag.
  ///
  /// \return                             List with pick results for each viewport.
  ///
  virtual IScene_pick_results_list* pick(
    const mi::math::Vector_struct<mi::Sint32, 2>& pixel_location,
    const nv::index::IIndex_canvas* pick_canvas, IViewport_list* viewport_list,
    mi::neuraylib::Tag_struct session_tag) const = 0;

  /// Returns all intersections of a ray cast from the center of the camera
  /// through the pixel with the given scene element.
  ///
  /// \param[in] pixel_location           Pixel location on the canvas in screen space.
  /// \param[in] pick_canvas              Canvas that should be picked.
  /// \param[in] session_tag              \c ISession tag.
  /// \param[in] query_scene_element_tag  Scene element to pick.
  /// \param[in] dice_transaction         DiCE transaction.
  ///
  /// \return                             Pick results in front-to-back order.
  ///
  /// \deprecated
  virtual IScene_pick_results* pick(const mi::math::Vector_struct<mi::Uint32, 2>& pixel_location,
    const nv::index::IIndex_canvas* pick_canvas, mi::neuraylib::Tag_struct session_tag,
    mi::neuraylib::Tag_struct query_scene_element_tag,
    mi::neuraylib::IDice_transaction* dice_transaction) const = 0;

  /// Enables querying single entries of a scene dataset.
  /// The method returns the entry that the index value refers to in the dataset.
  /// For instance, the index could represent the triangle of a triangle mesh
  /// that is distributed in the cluster environment. The lookup operation would
  /// then return all data related to the triangle such as the per-vertex
  /// information.
  ///
  /// \param[in] entry_index              The index that refers to the entry query.
  /// \param[in] scene_element_tag        The given scene element.
  /// \param[in] session_tag              The \c ISession tag.
  /// \param[in] dice_transaction         DiCE transaction.
  ///
  /// \return                             Entry result.
  ///
  virtual IScene_lookup_result* entry_lookup(mi::Uint64 entry_index,
    mi::neuraylib::Tag_struct scene_element_tag, mi::neuraylib::Tag_struct session_tag,
    mi::neuraylib::IDice_transaction* dice_transaction) const = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IINDEX_SCENE_QUERY_H
