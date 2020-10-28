/***************************************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Provides information about GPUs.

#ifndef MI_NEURAYLIB_IGPU_DESCRIPTION_H
#define MI_NEURAYLIB_IGPU_DESCRIPTION_H

#include <mi/base/interface_declare.h>

namespace mi {

namespace neuraylib {

/** \addtogroup mi_neuray_configuration
@{
*/

/// This interface describes a GPU.
///
/// \if IRAY_API \see #mi::neuraylib::IRendering_configuration::get_gpu_description()
/// \else        \see #mi::neuraylib::IDice_configuration::get_gpu_description()
/// \endif
class IGpu_description : public
    mi::base::Interface_declare<0x1e2e02ff,0xf083,0x4a12,0xb7,0xda,0x02,0x58,0xb6,0xd0,0x04,0x75>
{
public:
    /// Returns the GPU ID.
    virtual Uint32 get_id() const = 0;

    /// Returns the GPU name.
    ///
    /// The name is human-readable string, typically involving the GPU model.
    virtual const char* get_name() const = 0;

    /// Returns the GPU memory in bytes (or -1 in case of failures).
    virtual Sint64 get_memory_size() const = 0;

    /// Indicates whether the GPU is in TCC mode.
    ///
    /// Returns \c true if the GPU is in TCC mode (Windows only), and \c false otherwise.
    virtual bool get_tcc_flag() const = 0;

    /// Returns the CUDA device ID, or -1 if the GPU is not a CUDA device.
    virtual Sint32 get_cuda_device_id() const = 0;

    /// Returns the CUDA compute capability (major), or -1 if the GPU is not a CUDA device.
    virtual Sint32 get_cuda_compute_capability_major() const = 0;

    /// Returns the CUDA compute capability (minor), or -1 if the GPU is not a CUDA device.
    virtual Sint32 get_cuda_compute_capability_minor() const = 0;
    
    /// Returns the clock rate in kilohertz, or -1 if the GPU is not a CUDA device.
    virtual Sint32 get_clock_rate() const = 0;

    /// Returns the number of multiprocessors, or -1 if the GPU is not a CUDA device.
    virtual Sint32 get_multi_processor_count() const = 0;

    /// Returns the PCI bus ID (or -1 in case of failure).
    virtual Sint32 get_pci_bus_id() const = 0;

    /// Returns the PCI device ID (or -1 in case of failure).
    virtual Sint32 get_pci_device_id() const = 0;

    /// Indicates whether the GPU is attached to a display.
    ///
    /// Returns \c true if the GPU is attached to a display (Windows only), and \c false otherwise.
    virtual bool is_attached_to_display() const = 0;

    /// Returns \c true if the GPU is part of an Optimus/mixed internal+discrete GPU setup (Windows only), and \c false otherwise.
    virtual bool get_optimus_flag() const = 0;
};

/*@}*/ // end group mi_neuray_configuration

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IGPU_DESCRIPTION_H
