/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Interface for user-defined distributed computing algorithms

#ifndef NVIDIA_INDEX_IDISTRIBUTED_COMPUTE_ALGORITHM_H
#define NVIDIA_INDEX_IDISTRIBUTED_COMPUTE_ALGORITHM_H

#include <mi/base/types.h>
#include <mi/math/vector.h>
#include <mi/math/color.h>
#include <mi/dice.h>
#include <mi/base/interface_declare.h>

namespace nv
{
namespace index
{

/// @ingroup nv_index_data_computing
/// Interface class that enables implementing distributed computing algorithms or computing tasks
/// that can operate on the distributed data uploaded to the cluster environment.
///
/// This interface class wraps the DiCE fragmented job interface class and extends it by two
/// additional interface methods: The number of compute units that the distributed computing tasks
/// should be split into (a.k.a. the number of fragments of the fragmented job) needs to be returned
/// by the method \c get_nb_of_fragments(). After running the distributed computing tasks the method
/// \c get_updated_bounding_box needs to return the newly computed bounding box.
///
/// The interface class \c IHeightfield_interaction provides means for invoking a distributed computing
/// task (i.e., the fragmented job) for heightfields internally. Alternatively, the \c
/// IDistributed_compute_algorithm interface class can also be used directly to implement
/// distributed compute tasks working with volume or heightfield data.
///
class IDistributed_compute_algorithm :
        public mi::base::Interface_declare<0x5e67c454,0x2566,0x4485,0xb3,0xea,0x76,0x86,0x5f,0x67,0x45,0xe6,
                                           mi::neuraylib::IFragmented_job>
{
public:
    /// The number of fragments defines the number of compute units the problem is split into.
    ///
    /// \return     The number of fragments used as parameter to invoke the
    ///             distributed compute tasks, i.e., the fragmented job,
    ///             through the DiCE method \c execute_fragmented().
    ///
    virtual mi::Size get_nb_of_fragments() const = 0;

    /// Returns the bounding box of the scene element associated with the
    /// compute algorithm, which might have been changed by the computation.
    ///
    /// \param[out] bbox    The bounding box that might be changed if the extent
    ///                     of the scene element changes due to the implemented
    ///                     compute algorithm. The bounding box is defined in the
    ///                     scene element's IJK space. Should be initialized with the
    ///                     original bounding box value.
    ///
    virtual void get_updated_bounding_box(
        mi::math::Bbox_struct<mi::Float32, 3>& bbox) const = 0;
};

/// @ingroup nv_index_data_computing
/// Mixin class for implementing the IDistributed_compute_algorithm interface.
///
/// For convenience, the mixin class leverages the default implementation of the
/// fragmented job mixin class.
template <mi::Uint32 i_id1, mi::Uint16 i_id2, mi::Uint16 i_id3,
          mi::Uint8 i_id4, mi::Uint8 i_id5, mi::Uint8 i_id6, mi::Uint8 i_id7,
          mi::Uint8 i_id8, mi::Uint8 i_id9, mi::Uint8 i_id10, mi::Uint8 i_id11,
          class I = IDistributed_compute_algorithm>
class Distributed_compute_algorithm :
    public mi::neuraylib::Fragmented_job<i_id1,i_id2,i_id3,i_id4,i_id5,i_id6,i_id7,i_id8,i_id9,i_id10,i_id11,I>
{
};


}} // namespace index / nv

#endif // NVIDIA_INDEX_IDISTRIBUTED_COMPUTE_ALGORITHM_H
