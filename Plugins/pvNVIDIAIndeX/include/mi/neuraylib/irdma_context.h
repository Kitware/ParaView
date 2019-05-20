/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Support for remote DMA transfers.

#ifndef MI_NEURAYLIB_IRDMA_CONTEXT_H
#define MI_NEURAYLIB_IRDMA_CONTEXT_H

#include <mi/base/interface_declare.h>

namespace mi
{

namespace neuraylib
{

/** \addtogroup mi_neuray_dice
@{
*/

/// An RDMA buffer represents a piece of pinned memory which can be used to transfer data over RDMA.
/// On Linux, an RDMA buffer can be allocated on CPU (i.e., host memory) or on GPU (i.e., device
/// memory). When an RDMA buffer is allocated on GPU, the device ID must be specified (because
/// a GPU can have multiple devices). When GPUDirect RDMA is installed, a GPU-based RDMA buffer
/// can be used to transmit or receive data directly over RDMA, i.e., the CPU is not involved
/// and the transferred data does not touch the CPU memory. This feature (GPUDirect RDMA) is
/// currently available only on Linux. Further, the following requirements must be satisfied.
///
///  Hardware requirements for GPUDirect RDMA:
///  - InfiniBand Mellanox ConnectX-3, ConnectX-3 Pro, or Connect-IB
///  - NVIDIA Tesla and Quadro GPUs
///  - GPU and InfiniBand devices must share the same PCI Express root. Otherwise, the performance
///  might be reduced.
///
/// Software requirements for GPUDirect RDMA:
/// - MLNX_OFED v2.1-x.x.x or later (from www.mellanox.com -> Products -> Software
///       -> InfiniBand/VPI Drivers -> Linux SW/Drivers)
/// - Plugin module to enable GPUDirect RDMA (from www.mellanox.com -> Products -> Software
///       -> InfiniBand/VPI Drivers -> GPUDirect RDMA)
/// - NVIDIA Driver 331.20 or later
/// - NVIDIA CUDA Runtime and Toolkit 6.0 or later
///
/// To check whether GPUDirect RDMA is running on your Linux system, execute
/// "service nv_peer_mem status" or "lsmod | grep nv_peer_mem" (this depends on the flavor
/// of your Linux system).
///
/// In case, GPUDirect RDMA is not available, DiCE offers a fallback solution by allocating
/// an RDMA buffer on CPU and copies the data between CPU-based and GPU-based RDMA buffers
/// in a transparent manner.
///
class IRDMA_buffer : public mi::base::Interface_declare<0xa2c75819, 0x752a, 0x4348, 0x9f, 0x62,
                       0x95, 0x5a, 0x2a, 0xcf, 0xed, 0x12>
{
public:
  /// Returns the ID of this buffer.
  ///
  /// To be used to send to the other host.
  virtual Uint64 get_id() const = 0;

  /// Indicates whether the RDMA buffer is in main memory or on a GPU.
  ///
  /// \return    -1, if the RDMA buffer in main memory, or the GPU ID otherwise
  virtual Sint32 get_gpu_id() const = 0;

  /// Returns a const pointer to the data in this buffer.
  virtual const Uint8* get_data() const = 0;

  /// Returns a mutable pointer to the data in this buffer.
  virtual Uint8* get_data() = 0;

  /// Returns the size of the data in this buffer buffer.
  virtual Size get_size() const = 0;

  /// Duplicates an RDMA buffer (const).
  ///
  /// The new RDMA buffer will still use the memory provided by the original RDMA buffer but will
  /// point to a sub part of the original buffer. This is useful if a continuous piece of memory
  /// is needed for generating the data, but the result should be sent in different fragmented job
  /// results, e.g., to different hosts.
  ///
  /// The source RDMA buffer will be retained by this buffer, thus keeping the memory pinned and
  /// unusable for other operations as long as the returned buffer exists.
  ///
  /// \param offset   The offset in the original RDMA buffer at which the data of the returned
  ///                 RDMA buffer starts.
  /// \param size     The size of the data in the returned RDMA buffer. The data in the returned
  ///                 RDMA buffer ends at \p offset plus \p size (excluding) in the original RDMA
  ///                 buffer.
  /// \return         The RDMA buffer or \c NULL if \p offset and \p size result in a memory
  ///                 region not completely in the original buffer.
  virtual const IRDMA_buffer* duplicate(Size offset, Size size) const = 0;

  /// Duplicates an RDMA buffer (mutable).
  ///
  /// The new RDMA buffer will still use the memory provided by the original RDMA buffer but will
  /// point to a sub part of the original buffer. This is useful if a continuous piece of memory
  /// is needed for generating the data, but the result should be sent in different fragmented job
  /// results, e.g., to different hosts.
  ///
  /// The source RDMA buffer will be retained by this buffer, thus keeping the memory pinned and
  /// unusable for other operations as long as the returned buffer exists.
  ///
  /// \param offset   The offset in the original RDMA buffer at which the data of the returned
  ///                 RDMA buffer starts.
  /// \param size     The size of the data in the returned RDMA buffer. The data in the returned
  ///                 RDMA buffer ends at \p offset plus \p size (excluding) in the original RDMA
  ///                 buffer.
  /// \return         The RDMA buffer or \c NULL if \p offset and \p size result in a memory
  ///                 region not completely in the original buffer.
  virtual IRDMA_buffer* duplicate(Size offset, Size size) = 0;
};

/// The RDMA context works as a cache for RDMA buffers.
///
/// The allocation of those buffers is relatively costly, especially for larger buffers. Therefore,
/// it is better to not allocate them for every message.
class IRDMA_context : public mi::base::Interface_declare<0x5f3980e9, 0xfadc, 0x478d, 0xbb, 0x18,
                        0x44, 0x10, 0xa4, 0x6f, 0xf7, 0xb5>
{
public:
  /// Allocates an RDMA buffer which can be used to write locally to and to read from on a remote
  /// host.
  ///
  /// \param size               The size of the RDMA buffer.
  /// \param gpu_id            -1 for main memory, or the ID of the GPU on which the RDMA buffer
  ///                           should be allocated.
  /// \return                   The RDMA buffer or \c NULL in case of an allocation failure. The
  ///                           method also returns \c NULL if called from
  ///                           #mi::neuraylib::IFragmented_job::execute_fragment_remote_rdma()
  ///                           and \p size exceeds the size of the RDMA buffer on the receiver
  ///                           side.
  virtual IRDMA_buffer* get_write_memory(Size size, Sint32 gpu_id = -1) = 0;

  /// Allocates an RDMA buffer which can be used to read locally from a buffer that was written on
  /// a remote host.
  ///
  /// \param size               The size of the RDMA buffer.
  /// \param gpu_id            -1 for main memory, or the ID of the GPU on which the RDMA buffer
  ///                           should be allocated.
  /// \return                   The RDMA buffer or \c NULL in case of an allocation failure.
  virtual IRDMA_buffer* get_read_memory(Size size, Sint32 gpu_id = -1) = 0;

  /// Creates an RDMA buffer that is a wrapper for a buffer provided by users.
  /// Currently, this function should only be called inside
  /// #mi::neuraylib::IFragmented_job::get_rdma_result_buffer() to create an
  /// RDMA buffer for receiving results from a fragmented job.
  ///
  /// \param data               The data buffer provided by the user.
  /// \param size               The buffer's size.
  ///
  /// \param gpu_id            -1 for main memory, or the ID of the GPU on
  ///                           which the user's buffer is allocated.
  /// \return                   The RDMA buffer or \c NULL in case of failure.
  virtual IRDMA_buffer* wrap_user_buffer(Uint8* data, Size size, Sint32 gpu_id = -1) = 0;

  /// Send an RDMA buffer to the remote host associated with this RDMA context.
  ///
  /// \param buffer         The RDMA buffer to be sent.
  /// \return
  ///                       -  0: Success
  ///                       - -1: Invalid parameters (e.g., buffer is \c NULL).
  ///                       - -2: Operation not supported outside of
  ///                             #mi::neuraylib::IFragmented_job::execute_fragment_remote_rdma().
  virtual Sint32 flush(IRDMA_buffer* buffer) = 0;
};

/*@}*/ // end group mi_neuray_dice

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IRDMA_CONTEXT_H
