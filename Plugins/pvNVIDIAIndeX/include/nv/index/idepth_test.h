/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Depth test shape rendering attribute

#ifndef NVIDIA_INDEX_IDEPTH_TEST_H
#define NVIDIA_INDEX_IDEPTH_TEST_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/iattribute.h>

namespace nv
{
namespace index
{
/// @ingroup nv_index_scene_description_attribute
///
/// Defines a depth test for the shapes defined in the scene description.
/// Similar to the depth tests in OpenGL, NVIDIA IndeX allows the application
/// to define the depth ordering of shapes on a 'per-fragment' level.
/// The depth test operation represents an attribute in the scene description
/// and has an effect on all the shapes that are defined subsequently
/// in the hierarchical description of the scene.
///
class IDepth_test : public mi::base::Interface_declare<0xa8a4d29c, 0x4a2d, 0x456a, 0xa2, 0x73, 0x4d,
                      0x5e, 0x3b, 0x27, 0x31, 0xd5, nv::index::IAttribute>
{
public:
  /// Depth test modes that are supported by NVIDIA IndeX.
  enum Depth_test_mode
  {
    /// z-test less operator (<)
    TEST_LESS = 0,
    /// z-test less than or equal operator (<=)
    TEST_LESS_EQUAL = 1
  };

public:
  /// Set depth test operator of this attribute.
  /// The default depth test operation is TEST_LESS_EQUAL.
  ///
  /// \param[in] test     The parameter defines the depth test operator.
  ///
  virtual void set_depth_test(Depth_test_mode test) = 0;

  /// Get current depth test operator of this attribute.
  ///
  /// \return   Returns the current depth test operator.
  ///
  virtual Depth_test_mode get_depth_test() const = 0;
};
}
} // namespace index / nv

#endif // NVIDIA_INDEX_IDEPTH_TEST_H
