/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Interface for a scene

#ifndef NVIDIA_INDEX_ISCENE_H
#define NVIDIA_INDEX_ISCENE_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>
#include <mi/base/uuid.h>

#include <nv/index/iattribute.h>
#include <nv/index/icorner_point_grid.h>
#include <nv/index/idistributed_data_import_callback.h>
#include <nv/index/iheight_field_scene_element.h>
#include <nv/index/iirregular_volume_scene_element.h>
#include <nv/index/ipipe_set.h>
#include <nv/index/iregular_heightfield.h>
#include <nv/index/iregular_volume.h>
#include <nv/index/iscene_group.h>
#include <nv/index/ishape.h>
#include <nv/index/isparse_volume_scene_element.h>
#include <nv/index/itriangle_mesh_scene_element.h>

namespace nv
{
namespace index
{
/// @ingroup nv_index_scene_description

/// The abstract interface class representing the entire scene. The scene is
/// also the root node of the scene description and therefore implements the
/// IScene_group interface.
///
/// The scene defines the global three-dimensional space bounded by the minimal
/// volume that encloses all scene elements. An associated ICamera controls the
/// viewpoint from which the scene can be rendered.
///
/// The scene manages the scene elements which are the graphical representations
/// of datasets. A scene defines a <em>global region of interest</em>
/// which is the bounding box that can be used to confine rendering to a subset
/// of the entire world space.
///
class IScene :
    public mi::base::Interface_declare<0xaab98430,0xd6c5,0x49c5,0xbf,0x06,0x12,0xae,0xe5,0x17,0x71,0x8c,
                                       nv::index::IScene_group>
{
public:
    ///////////////////////////////////////////////////////////////////////////////////
    /// \name View and transformation.
    ///@{

    /// Sets the camera that defines the viewpoint of the scene.
    ///
    /// \param[in] tag Tag of an \c ICamera
    virtual void set_camera(mi::neuraylib::Tag_struct tag) = 0;

    /// Returns the camera associated with the scene.
    ///
    /// \return Tag of an \c ICamera
    virtual mi::neuraylib::Tag_struct get_camera() const = 0;

    /// Sets the global scene transformation matrix. This transformation is
    /// applied in addition to the camera transformation and can be used, for
    /// example, to adapt to a custom coordinate system.
    ///
    /// \param[in] transform_mat Transformation matrix
    virtual void set_transform_matrix(
        const mi::math::Matrix_struct<mi::Float32, 4, 4>& transform_mat) = 0;

    /// Sets the global scene transformation. This transformation is applied in
    /// addition to the camera transformation and can be used, for example, to
    /// adapt to a custom coordinate system. The transformation is specified by
    /// three components which are applied in this order: translation, rotation,
    /// scaling.
    ///
    /// \param[in] translation  Translation component of the transformation
    /// \param[in] rotation     Rotation component of the transformation
    ///                           (specified as a rotation matrix)
    /// \param[in] scaling      Scaling component of the transformation
    virtual void set_transform_matrix(
        const mi::math::Vector_struct<mi::Float32, 3>&    translation,
        const mi::math::Matrix_struct<mi::Float32, 4, 4>& rotation,
        const mi::math::Vector_struct<mi::Float32, 3>&    scaling) = 0;

    /// Returns the global scene transformation matrix.
    /// \return transformation matrix
    virtual mi::math::Matrix_struct<mi::Float32, 4, 4> get_transform_matrix() const = 0;
    ///@}

    ///////////////////////////////////////////////////////////////////////////////////
    /// \name Creating scene groups.
    ///@{

    /// Creates a new scene group of the given type, but does not yet add it to
    /// the scene description.
    ///
    /// \param[in] class_id Identifier of the type of scene group
    /// \return The new \c IScene_group
    virtual IScene_group* create_scene_group(const mi::base::Uuid& class_id) const = 0;

    /// Creates a new scene group of the given type, but does not yet add it to
    /// the scene description.
    /// \return The new \c IScene_group
    template<class T>
    T* create_scene_group() const
    {
        return static_cast<T*>( create_scene_group(typename T::IID()) );
    }
    ///@}

    ///////////////////////////////////////////////////////////////////////////////////
    /// \name Creating shapes.
    ///@{

    /// Creates a new shape of the given type, but does not yet add it to the
    /// scene description.
    /// \param[in] class_id Identifier of the type of shape
    /// \return The new \c IScene
    virtual IShape* create_shape(const mi::base::Uuid& class_id) const = 0;

    /// Creates a new shape of the given type, but does not yet add it to the
    /// scene description.
    /// \return The new \c IScene
    template<class T>
    T* create_shape() const
    {
        return static_cast<T*>( create_shape(typename T::IID()) );
    }
    ///@}

    ///////////////////////////////////////////////////////////////////////////////////
    /// \name Creating attributes.
    ///@{

    /// Creates a new attribute of the given type, but does not yet add it to
    /// the scene description.
    /// \param[in] class_id Identifier of the type of attribute
    /// \return The new \c IAttribute
    virtual IAttribute* create_attribute(const mi::base::Uuid& class_id) const = 0;

    /// Creates a new attribute of the given type, but does not yet add it to
    /// the scene description.
    /// \return The new \c IAttribute
    template<class T>
    T* create_attribute() const
    {
        return static_cast<T*>( create_attribute(typename T::IID()) );
    }
    ///@}

    /// Creates a new volume scene element from the given import configuration,
    /// but does not yet add it to the scene description.
    ///
    /// The scene element can be transformed by specifying scaling, a rotation
    /// around its local K-axis, and a translation. These transformations
    /// generate the matrix that will be returned by \c
    /// IRegular_volume::get_transform() and is applied to transform the scene
    /// element from IJK to XYZ space.
    ///
    /// \param[in] scaling            Scaling; use (1, 1, 1) to disable.
    /// \param[in] rotation_k         Rotation around the K-axis (in radians).
    /// \param[in] translation        Translation; use (0, 0, 0) to remain at the origin.
    /// \param[in] size               Size of the entire volume data in each dimension
    ///                                 (in voxels).
    /// \param[in] importer_callback  Distributed data import callback function.
    /// \param[in] dice_transaction   The DiCE transaction.
    ///
    /// \return The new \c IRegular_volume instance.
    ///
    virtual IRegular_volume* create_volume(
        const mi::math::Vector_struct<mi::Float32, 3>&          scaling,
        mi::Float32                                             rotation_k,
        const mi::math::Vector_struct<mi::Float32, 3>&          translation,
        const mi::math::Vector_struct<mi::Uint32, 3>&           size,
        nv::index::IDistributed_data_import_callback*           importer_callback,
        mi::neuraylib::IDice_transaction*                       dice_transaction) const = 0;

    /// Creates a new irregular volume scene element from the given import configuration, but does not yet
    /// add it to the scene description.
    ///
    /// \note       The longest mesh edge information is required for correct visual results. It sets an
    ///             important bound for the renderer to consider for setting up internal resources.
    ///
    /// \param[in] ijk_bbox                     The local space bounding box.
    /// \param[in] max_mesh_edge_length         The length of the longest edge in the irregular volume mesh.
    /// \param[in] importer_callback            Distributed data import callback function.
    /// \param[in] dice_transaction             The DiCE transaction.
    ///
    /// \return                                 The new \c IIrregular_volume_scene_element instance.
    ///
    virtual IIrregular_volume_scene_element* create_irregular_volume(
        const mi::math::Bbox_struct<mi::Float32, 3> &               ijk_bbox,
        const mi::Float32                                           max_mesh_edge_length,
        nv::index::IDistributed_data_import_callback*               importer_callback,
        mi::neuraylib::IDice_transaction*                           dice_transaction) const = 0;

    /// Creates a new sparse volume scene element from the given import configuration, but does not yet
    /// add it to the scene description.
    ///
    /// \param[in] ijk_bbox                     The local space bounding box.
    /// \param[in] transform_matrix             Transformation matrix from IJK (local) to XYZ (global) space.
    /// \param[in] importer_callback            Distributed data import callback function.
    /// \param[in] dice_transaction             The DiCE transaction.
    ///
    /// \return                                 The new \c ISparse_volume_scene_element instance.
    ///
    virtual ISparse_volume_scene_element* create_sparse_volume(
        const mi::math::Bbox_struct<mi::Float32, 3> &           ijk_bbox,
        const mi::math::Matrix_struct<mi::Float32, 4, 4>&       transform_matrix,
        nv::index::IDistributed_data_import_callback*           importer_callback,
        mi::neuraylib::IDice_transaction*                       dice_transaction) const = 0;

    /// Creates a new corner-point grid scene element from the given import configuration, but does not yet
    /// add it to the scene description.
    ///
    /// \param[in] grid_dims                    Dimensions of the corner-pointer grid. Layer resolution in
    ///                                         x and y, number of layers in z.
    /// \param[in] bbox                         The local space bounding box.
    /// \param[in] transform_matrix             Transformation matrix.
    /// \param[in] importer_callback            Distributed data import callback function.
    /// \param[in] dice_transaction             The DiCE transaction.
    ///
    /// \return                                 The new \c ISparse_volume_scene_element instance.
    ///
    virtual ICorner_point_grid* create_corner_point_grid(
        const mi::math::Vector_struct<mi::Uint32, 3>&           grid_dims,
        const mi::math::Bbox_struct<mi::Float32, 3>&            bbox,
        const mi::math::Matrix_struct<mi::Float32, 4, 4>&       transform_matrix,
        nv::index::IDistributed_data_import_callback*           importer_callback,
        mi::neuraylib::IDice_transaction*                       dice_transaction) const = 0;

    virtual IPipe_set* create_pipe_set(
        const mi::math::Bbox_struct<mi::Float32, 3>&                bbox,
        nv::index::IDistributed_data_import_callback*               importer_callback,
        mi::neuraylib::IDice_transaction*                           dice_transaction) const = 0;

    /// Creates a new regular heightfield scene element from the given import
    /// configuration.  The created heightfield scene element will be returned
    /// and must then be added to the scene description.
    ///
    /// The scene element can be transformed by specifying scaling, a rotation
    /// around its local K-axis, and a translation. These transformations
    /// generate the matrix that will be returned by
    /// IRegular_heightfield::get_transform() and is applied to transform the
    /// scene element from the heightfield's local space to the global space.
    ///
    /// \param[in] scaling              Scaling; use (1, 1, 1) to disable.
    /// \param[in] rotation_k           Rotation around the K-axis (in radians).
    /// \param[in] translation          Translation; use (0, 0, 0) to remain at the
    ///                                 origin.
    /// \param[in] grid_size            Number of grid points in each direction.
    /// \param[in] k_range              Minimum and maximum K-value (equal to height)
    ///                                 in the heightfield.
    /// \param[in] importer_callback    Distributed data import callback function
    /// \param[in] dice_transaction     The DiCE transaction.
    ///
    /// \return                         The new \c IRegular_heightfield instance.
    ///
    virtual IRegular_heightfield* create_regular_heightfield(
        const mi::math::Vector_struct<mi::Float32, 3>&          scaling,
        mi::Float32                                             rotation_k,
        const mi::math::Vector_struct<mi::Float32, 3>&          translation,
        const mi::math::Vector_struct<mi::Uint32, 2>&           grid_size,
        const mi::math::Vector_struct<mi::Float32, 2>&          k_range,
        nv::index::IDistributed_data_import_callback*           importer_callback,
        mi::neuraylib::IDice_transaction*                       dice_transaction) const = 0;

    /// Creates a default regular heightfield scene element.  The default
    /// heightfield has the given elevation value at each grid location of the
    /// given 2D patch.  The scene element can be transformed by specifying
    /// scaling, a rotation around its local K-axis, and a translation. These
    /// transformations generate the matrix that will be returned by
    /// IRegular_heightfield::get_transform() and is applied to transform the
    /// scene element from the heightfield's local space to the global space.
    ///
    /// \param[in] default_height    The elevation value used at every grid position
    ///                                of the heightfield
    /// \param[in] scaling           Scaling; use (1, 1, 1) to disable
    /// \param[in] rotation_k        Rotation around the K-axis (in radians)
    /// \param[in] translation       Translation; use (0, 0, 0) to remain at the origin
    /// \param[in] grid_size         Number of grid points in each direction
    /// \param[in] k_range           Minimum and maximum K-value (equal to height)
    ///                              in the heightfield
    /// \param[in] dice_transaction  The DiCE transaction.
    ///
    /// \return                      The new \c IRegular_heightfield instance..
    ///
    virtual IRegular_heightfield* create_regular_heightfield(
        mi::Float32                                             default_height,
        const mi::math::Vector_struct<mi::Float32, 3>&          scaling,
        mi::Float32                                             rotation_k,
        const mi::math::Vector_struct<mi::Float32, 3>&          translation,
        const mi::math::Vector_struct<mi::Uint32, 2>&           grid_size,
        const mi::math::Vector_struct<mi::Float32, 2>&          k_range,
        mi::neuraylib::IDice_transaction*                       dice_transaction) const = 0;
    
    /// Creates a default height field scene element.  The default
    /// heightfield has the given elevation value at each grid location of the
    /// given 2D patch.  The scene element can be transformed by specifying
    /// scaling, a rotation around its local K-axis, and a translation. These
    /// transformations generate the matrix that will be returned by
    /// IHeight_field_scene_element::get_transform() and is applied to transform the
    /// scene element from the heightfield's local space to the global space.
    ///
    /// \param[in] scaling           Scaling; use (1, 1, 1) to disable
    /// \param[in] rotation_k        Rotation around the K-axis (in radians)
    /// \param[in] translation       Translation; use (0, 0, 0) to remain at the origin
    /// \param[in] grid_size         Number of grid points in each direction
    /// \param[in] k_range           Minimum and maximum K-value (equal to height)
    ///                              in the heightfield
    /// \param[in] importer_callback Distributed data import callback function.
    /// \param[in] dice_transaction  The DiCE transaction.
    ///
    /// \return                      The new \c IRegular_heightfield instance..
    virtual IHeight_field_scene_element* create_height_field(
        const mi::math::Vector_struct<mi::Float32, 3>&          scaling,
        mi::Float32                                             rotation_k,
        const mi::math::Vector_struct<mi::Float32, 3>&          translation,
        const mi::math::Vector_struct<mi::Uint32, 2>&           grid_size,
        const mi::math::Vector_struct<mi::Float32, 2>&          k_range,
        nv::index::IDistributed_data_import_callback*           importer_callback,
        mi::neuraylib::IDice_transaction*                       dice_transaction) const = 0;

    /// Creates a new triangle mesh scene element from the given import configuration, but does not yet
    /// add it to the scene description.
    ///
    /// \param[in] ijk_bbox                     The local space bounding box.
    /// \param[in] importer_callback            Distributed data import callback function.
    /// \param[in] dice_transaction             The DiCE transaction.
    /// \return                                 The new \c ITriangle_mesh_scene_element instance.
    ///
    virtual ITriangle_mesh_scene_element* create_triangle_mesh(
        const mi::math::Bbox_struct<mi::Float32, 3> &           ijk_bbox,
        nv::index::IDistributed_data_import_callback*           importer_callback,
        mi::neuraylib::IDice_transaction*                       dice_transaction) const = 0;

    ///////////////////////////////////////////////////////////////////////////////////
    /// \name Bounding boxes and region of interest
    ///@{

    /// Clipping the space in which the entire scene contents is positioned.
    /// The scene contents is positioned in a joint 'global' space. The joint space 
    /// is used for spatial subdivision and, thus, is also know as 'subdivision space'.
    ///
    /// \param[in] bbox     The bounding box clips the space that contains all the scene contents.
    ///                     Only those parts of the scene that are inside the clipped space are
    ///                     displayed. The bounding box is defined in the joint subdivision space.
    ///
    virtual void set_clipped_bounding_box(const mi::math::Bbox_struct<mi::Float32, 3>& bbox) = 0;

    /// Returns the bounding box that clips the space that contains the scene contents.
    ///
    /// \return Returns the bounding box that clips the space containing the scene contents.
    ///         The bounding box is defined in the joint subdivision space.
    ///
    virtual mi::math::Bbox_struct<mi::Float32, 3> get_clipped_bounding_box() const = 0;

    ///@}
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_ISCENE_H
