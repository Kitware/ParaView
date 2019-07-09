/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Interface class representing NVIDIA IndeX's distributed datasets.

#ifndef NVIDIA_INDEX_IDISTRIBUTED_DATA_H
#define NVIDIA_INDEX_IDISTRIBUTED_DATA_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <mi/math/bbox.h>
#include <mi/math/matrix.h>

#include <nv/index/iscene_element.h>

namespace nv
{
namespace index
{

/// @ingroup nv_index_scene_description_shape
///
/// The base class of all distributed datasets.
///
class IDistributed_data :
    public mi::base::Interface_declare<0xe7159219,0x4b05,0x4201,0x82,0x70,0x5a,0xaf,0xfc,0x8b,0xf4,0xdb,
                                       nv::index::IScene_element>
{
public:
    /// A distributed data has an extent in 3D space and thus defines a 3D bounding box.
    /// The bounding box is always defined in the distributed dataset's
    /// local coordinate system. The rendering traversals require the bounding box
    /// for accurate and efficient rendering.
    ///
    /// \return     The bounding box of the dataset in its local coordinate system.
    ///
    virtual mi::math::Bbox_struct<mi::Float32, 3> get_bounding_box() const = 0;

    /// Returns the transformation matrix applied to the distributed data.
    ///
    /// \return     Transformation matrix from the local to the global space.
    ///
    virtual mi::math::Matrix_struct<mi::Float32, 4, 4> get_transform() const
    {
        mi::math::Matrix_struct<mi::Float32, 4, 4> mat_struct = mi::math::Matrix<mi::Float32, 4, 4>(1.f);
        return mat_struct;
    };

    /// A distributed dataset's extent in 3D space that shall be considered for
    /// rendering or computing can be restricted by a clip region.
    /// The NVIDIA IndeX library only takes into account thoses parts of the
    /// distributed dataset that lie inside of this. All parts outside of the
    /// region will be ignored.
    /// Changing the clip region, e.g., increasing its size, might result
    /// in a repartitioning of the spatial subdivision or might trigger
    /// importer calls.
    ///
    /// \return     The clip region of the dataset defined in its local
    ///            coordinate system.
    ///
    virtual void set_clip_region(const mi::math::Bbox_struct<mi::Float32, 3>&) {};

    /// A distributed dataset's extent in 3D space that shall be considered for
    /// rendering or computing can be restricted by a clip region.
    ///
    /// \return     The clip region of the dataset in its local
    ///            coordinate system.
    ///
    virtual mi::math::Bbox_struct<mi::Float32, 3> get_clip_region() const
    {
        return mi::math::Bbox<mi::Float32, 3>(); // defaults to invalid bbox
    }

    /// Each distributed dataset may be pickable, so that the ray cast through the scene can
    /// intersect and query the data. The intersection information will be returned by the
    /// pick query operation.
    ///
    /// \return     The distributed data may or may not be pickable.
    ///
    virtual bool get_pickable() const = 0;

    /// Sets the pickable state of the distributed data scene element.
    ///
    /// \param[in] pickable    The distributed data is pickable.
    ///
    virtual void set_pickable(bool pickable) = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_IDISTRIBUTED_DATA_H
