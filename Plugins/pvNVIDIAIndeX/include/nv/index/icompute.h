/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief API for compute programs in the NVIDIA IndeX library.

#ifndef NVIDIA_INDEX_ICOMPUTE_H
#define NVIDIA_INDEX_ICOMPUTE_H

#include <mi/base/interface_declare.h>
#include <mi/math/bbox.h>
#include <mi/neuraylib/iserializer.h>    // Tag_struct

#include <nv/index/iattribute.h>
#include <nv/index/idistributed_data_subset.h>
#include <nv/index/ierror.h>

namespace nv {
namespace index {

// forward declarations
class ICompute_results;
class ICompute_launch_request;
class IDistributed_data_access;
class IIndex_rendering;

/// @ingroup xac_compute
///
/// Interface to XAC compute infrastructure.
/// Access this by using \c IIndex::get_api_component().
///
class IIndex_compute :
    public mi::base::Interface_declare<0x5aee3342,0x97c2,0x41fc,0xa3,0x40,0x3e,0x1c,0x20,0x87,0x10,0xff>
{
public:
    /// Create object to specify compute launch request.
    virtual ICompute_launch_request* create_launch_request() = 0;

    /// Perform a compute launch.
    /// \param[in]  request          The launch request specifying the compute program and elements.
    /// \param[in]  index_rendering  The index_rendering interface.
    /// \param[in]  session_tag      The \c ISession tag.
    /// \param[in]  dice_transaction DiCE transaction.
    /// \return The compute result, \see ICompute_results.
    /// 
    virtual const ICompute_results* perform_compute(
        const ICompute_launch_request*      request,
        IIndex_rendering*                   index_rendering,
        mi::neuraylib::Tag_struct           session_tag,
        mi::neuraylib::IDice_transaction*   dice_transaction) = 0;
};

/// @ingroup xac_compute
///
/// Interface to define a compute launch request.
///
class ICompute_launch_request :
    public mi::base::Interface_declare<0x7487d3dd,0x72e3,0x4a9e,0x9e,0x9d,0x1d,0x8b,0xfb,0x96,0xd5,0xa0>
{
public:
    /// Specify compute program and target for both data write and compute.
    virtual void set_launch(
        mi::neuraylib::Tag_struct xac_compute_program_tag,
        mi::neuraylib::Tag_struct data_write_target_tag) = 0;

    /// Specify compute program and separate targets for data write and compute.
    virtual void set_launch(
        mi::neuraylib::Tag_struct xac_compute_program_tag,
        mi::neuraylib::Tag_struct xac_compute_target_tag,
        mi::neuraylib::Tag_struct data_write_target_tag) = 0;

    /// Set region of interest in the datasets local coordinate system.
    /// Computation will be restricted to to points inside the region.
    /// Specifying an empty bounding box is equivalent to the complete datasets region.
    virtual void set_region_of_interest(const mi::math::Bbox_struct<mi::Float32, 3>& box) = 0;

    /// Returns true if request is valid.
    virtual bool is_valid() const = 0;

    /// Get launch program.
    virtual mi::neuraylib::Tag_struct get_compute_program_tag() const = 0;

    /// Get compute target.
    virtual mi::neuraylib::Tag_struct get_compute_target_tag() const = 0;

    /// Get data write target.
    virtual mi::neuraylib::Tag_struct get_data_write_target_tag() const = 0;

    /// Get region of interest, returns true if region of interest is valid.
    virtual bool get_region_of_interest(mi::math::Bbox_struct<mi::Float32, 3>& box) const = 0;
};

/// @ingroup xac_compute
///
/// Interface to access results of compute launch.
/// 
class ICompute_results :
    public mi::base::Interface_declare<0xcb08140d,0x10bc,0x4c8f,0xa0,0xf7,0xe,0x85,0x31,0x21,0xd8,0x6e>
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

/// @ingroup xac_compute
///
/// Interface to parameters of a compute plane. Applying this attribute to a \c IPlane scene element
/// makes the \c IPlane a valid target for a \c ICompute_launch_request. 
///
class ICompute_plane_parameters :
    public mi::base::Interface_declare<0xdaf77e98,0xd880,0x415c,0x86,0x39,0x2d,0x6b,0xee,0x9e,0x5c,0xbe,
                                       nv::index::IAttribute>
{
public:
    /// Define surface parameterization of the plane. This specifies the resolution of the 2D result buffer.
    virtual void set_resolution(const mi::math::Vector_struct<mi::Uint32, 2>& res) = 0;

    /// Define the format of compute values. (Similar to \c IRay_sampling_value_format).
    /// This defines the value slots to which a XAC compute program can write per invocation.
    ///
    /// \param[in] value_sizes  Sizes of values (in bytes) for \c nb_values (max. 64 byte per value).
    /// \param[in] nb_values    Number of values (max. 32),
    /// \return false if number of values is too large or if any of the value sizes is invalid.
    virtual bool set_value_sizes(const mi::Uint32* value_sizes, mi::Uint32 nb_values) = 0;

    /// Get the specified resolution.
    virtual mi::math::Vector_struct<mi::Uint32, 2> get_resolution() const = 0;

    /// Get the number of values specified. 
    virtual mi::Uint32 get_nb_values() const = 0;

    /// Get the byte size of value \c i
    virtual mi::Uint32 get_value_size(mi::Uint32 i) const = 0;
};

/// @ingroup xac_compute
///
/// Interface to compute results from a compute plane target.
///
class ICompute_result_buffer_2D :
    public mi::base::Interface_declare<0x8f335887,0x6c3b,0x48be,0xb0,0x55,0x46,0xb0,0xc4,0xdb,0xf4,0xb0,
                                       IDistributed_data_subset>
{
public:
    /// Get the resolution of this buffer. Note that this might be smaller than the resolution
    /// of \c ICompute_plane_parameters, depending on the bounding box given to \c IDistributed_data_access.
    virtual mi::math::Vector_struct<mi::Uint32, 2> get_resolution() const = 0;

    /// Get offset of this buffer inside the total resolution of \c ICompute_plane_parameters.
    virtual mi::math::Vector_struct<mi::Uint32, 2> get_offset() const = 0;


    /// Get the number of values per buffer element, \see ICompute_plane_parameters.
    virtual mi::Uint32 get_nb_values() const = 0;

    /// Get the byte size of value \c i, \see ICompute_plane_parameters.
    virtual mi::Uint32 get_value_size(mi::Uint32 i) const = 0;

    /// Get value mask. The bits of a value mask tell if a user value was written in the XAC compute program.
    /// If the bit is 0, then the value with value index equivalent to the bit number is not valid.
    ///
    /// \return Pointer to mask data with resolution dependent layout (x-major), or 0 if buffer is not valid.
    virtual const mi::Uint32* get_value_masks() const = 0;

    /// Get values of given value index.
    /// A value might be invalid, if the XAC compute program did not write to it.
    /// To check this, use get_value_masks().
    /// 
    /// \param[in]  value_index     Index of value (\see get_nb_values()).
    /// \return Pointer to values with resolution dependent layout (x-major), or 0 if buffer is not valid.
    virtual const void* get_values(mi::Uint32 value_index) const = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_ICOMPUTE_H
