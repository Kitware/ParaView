/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/**
   \file
   \brief        API for compute programs in the NVIDIA IndeX library.
*/

#ifndef NVIDIA_INDEX_COMPUTE_H
#define NVIDIA_INDEX_COMPUTE_H

#include <mi/base/interface_declare.h>
#include <mi/math/bbox.h>
#include <mi/neuraylib/iserializer.h> // Tag_struct
#include <nv/index/ierror.h>

namespace nv
{
namespace index
{

// forward declarations
class ICompute_results;
class ICompute_launch_request;
class IDistributed_data_access;
class IIndex_rendering;

/// Interface to compute infrastructure.
/// Access this by using IIndex::get_api_component().
///
class IIndex_compute : public mi::base::Interface_declare<0x5aee3342, 0x97c2, 0x41fc, 0xa3, 0x40,
                         0x3e, 0x1c, 0x20, 0x87, 0x10, 0xff>
{
public:
  /// Create object to specify compute launch request.
  virtual ICompute_launch_request* create_launch_request() = 0;

  /// Perform a compute launch.
  /// \param[in]  request          The launch request specifying the compute program and elements.
  /// \param[in]  index_rendering  The index_rendering interface.
  /// \param[in]  session_tag      The \c ISession tag.
  /// \param[in]  transaction      DiCE transaction.
  /// \return The compute result,\see ICompute_results.
  ///
  virtual const ICompute_results* perform_compute(const ICompute_launch_request* request,
    IIndex_rendering* index_rendering, mi::neuraylib::Tag_struct session_tag,
    mi::neuraylib::IDice_transaction* transaction) = 0;
};

/// Interface to define a compute launch request.
///
class ICompute_launch_request : public mi::base::Interface_declare<0x947a0c14, 0x79a2, 0x454a, 0xa0,
                                  0x76, 0x1d, 0xcf, 0x65, 0x4d, 0x56, 0xc3>
{
public:
  /// Specify compute program and data write target.
  virtual void set_launch(mi::neuraylib::Tag_struct xac_compute_program_tag,
    mi::neuraylib::Tag_struct data_write_target_tag) = 0;

  /// Set region of interest in the datasets local coordinate system.
  /// Computation will be restricted to to points inside the region.
  /// Specifying an empty bounding box is equivalent to the complete datasets region.
  virtual void set_region_of_interest(const mi::math::Bbox_struct<mi::Float32, 3>& box) = 0;

  /// Returns true if request is valid.
  virtual bool is_valid() const = 0;

  /// Get launch program.
  virtual mi::neuraylib::Tag_struct get_compute_program_tag() const = 0;

  /// Get data write target.
  virtual mi::neuraylib::Tag_struct get_data_write_target_tag() const = 0;

  /// Get region of interest, returns true if region of interest is valid.
  virtual bool get_region_of_interest(mi::math::Bbox_struct<mi::Float32, 3>& box) const = 0;
};

/// Interface to access results of compute launch.
///
class ICompute_results : public mi::base::Interface_declare<0xcb08140d, 0x10bc, 0x4c8f, 0xa0, 0xf7,
                           0xe, 0x85, 0x31, 0x21, 0xd8, 0x6e>
{
public:
  /// Return true if result is valid.
  virtual bool is_valid() const = 0;

  /// Returns an instance of the error set interface \c IError_set containing information about
  /// the success or failure of the compute process.
  virtual const IError_set* get_error_set() const = 0;

  /// Get interface to fetch computed data.
  virtual IDistributed_data_access* access_compute_results() const = 0;
};
}
} // nv::index

#endif
