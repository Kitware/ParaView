/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

#ifndef NVIDIA_INDEX_IINFERENCE_RESULT_H
#define NVIDIA_INDEX_IINFERENCE_RESULT_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>

#include <nv/index/imemory_manager.h>

namespace nv {
namespace index {

/// Interface class representing AI/DL inference results to be consumed by NVIDIA IndeX.
///
/// \ingroup nv_index_data_computing
///
class IInference_result :
    public mi::base::Interface_declare<0xd4374014,0x592b,0x4c6d,0x9c,0x3d,0x85,0xe7,0x9e,0xe6,0x1b,0x23>
{
public:
    /// Binds the results of the inference operation to an XAC program's slot.
    ///
    /// The inference operation shall write into a CUDA buffer which is wrapped by an instance
    /// of the class \c ICuda_memory_buffer. This buffer shall be passed back to NVIDIA IndeX 
    /// compute and rendering system and shall be bind to a specific XAC program.
    /// In the context of NVIDIA IndeX's Advanced Computing (XAC), uniform data, or user buffers
    /// are bound to an XAC program using slots, which represent unique identifier. These enable
    /// an XAC program to retrieve the uniform data or user buffer and make use of them inside the 
    /// XAC program. While the NVIDIA IndeX system transparently binds the CUDA buffers, the user 
    /// or application writer is responsible for appropriate use of the CUDA buffers inside the XAC
    /// program, e.g., memory buffer overflows are not managed by NVIDIA IndeX but the user has to 
    /// take care of those.
    ///
    /// \param[out]     xac_program         
    ///                 The XAC program that shall make use of the inference results.
    ///                 The inference will be bound to this program and the program
    ///                 is responsible to grab and apply the inference results when
    ///                 rendering or computing the dataset.
    ///                 \note
    ///                 The tag is ignored today but will be considered in the future.
    ///
    /// \param[out]     slot_id
    ///                 The unique identifier that binds the inference results to an
    ///                 XAC program. The slot id needs to be available in the XAC program
    ///                 so that the program can make use of the bound data.
    ///                 \note
    ///                 The slot id is ignored today but will be considered in the future.
    ///
    /// \param[out]     inference_results
    ///                 The CUDA memory buffer containing the inference results to be bound
    ///                 to the XAC program.
    ///
    virtual void bind_inference_result(
        mi::neuraylib::Tag_struct           xac_program,
        mi::Uint32                          slot_id,
        nv::index::ICuda_memory_buffer*     inference_results) = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IINFERENCE_RESULT_H
