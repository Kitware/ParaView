/******************************************************************************
 * Copyright 2018 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Pipe database element for voxel

#ifndef NVIDIA_INDEX_IPIPE_H
#define NVIDIA_INDEX_IPIPE_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/idistributed_data.h>
#include <nv/index/iscene_element.h>

namespace nv
{
namespace index
{

///
/// A pipe is defined by several connected lines which all have the same radius.
/// @ingroup nv_index_scene_description_shape
///
class IPipe
{
public:
  /// Get the number of points
  ///
  /// \return the number of points of the pipe
  virtual mi::Size get_nb_points() const = 0;

  /// Get the point array of the pipe
  ///
  /// \return the point array
  virtual const mi::math::Vector_struct<mi::Float32, 3>* get_points() const = 0;

  /// Get the number of radii
  ///
  /// radius per segment, overrides global radius. if nb_radii is 0, global
  /// radius is taken. if nb_radii is 1, the radius is constant for this pipe.
  /// otherwise there is one radius for each segment (i.e. npoints-1)
  ///
  /// \return the number of radii
  ///
  virtual mi::Size get_nb_radii() const = 0;

  /// Get the radii array
  ///
  /// \return the radii array
  ///
  virtual const mi::Float32* get_radii() const = 0;

  /// Get the number of materials
  ///
  /// \return the number of materials
  ///
  virtual mi::Size get_nb_materials() const = 0;

  /// Get the material ID array
  ///
  /// \return the material ID array
  ///
  virtual const mi::Uint16* get_materials() const = 0;
};

/// The pipeset is a set of pipes. Here is the definition of the database element.
/// @ingroup nv_index_scene_description_shape
///
class IPipe_set : public mi::base::Interface_declare<0xf8f26e5d, 0x8f04, 0x43bb, 0xb4, 0x2d, 0xcd,
                    0xed, 0x86, 0xbe, 0x59, 0x51, index::IDistributed_data>
{
public:
  virtual const IPipe* get_pipe(mi::Uint32 index) const = 0;

  /// Get the number of pipes
  ///
  /// \return the number of pipes
  ///
  virtual mi::Size get_nb_pipes() const = 0;

  /// default radius if not given for pipe
  /// \return radius size
  virtual mi::Float32 get_radius() const = 0;
};

/// @ingroup nv_index_scene_description_shape
class IPipe_set_scene_element
  : public mi::base::Interface_declare<0x95073d19, 0x97ff, 0x4ab2, 0xaf, 0xbc, 0x8a, 0x2c, 0x4f,
      0x49, 0x29, 0xb3, nv::index::IDistributed_data>
{
public:
  /// Returns the tag of the import strategy that is used for
  /// loading the dataset.
  ///
  /// \return tag of the IDistributed_data_import_strategy
  virtual mi::neuraylib::Tag_struct get_import_strategy() const = 0;
};
}
} // namespace index / nv

#endif // NVIDIA_INDEX_IPIPE_H
