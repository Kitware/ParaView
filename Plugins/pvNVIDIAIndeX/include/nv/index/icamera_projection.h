/******************************************************************************
 * Copyright 2025 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Configuration of advanced camera projection, like VR/XR lens emulation.

#ifndef NVIDIA_INDEX_ICAMERA_PROJECTION_H
#define NVIDIA_INDEX_ICAMERA_PROJECTION_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>

namespace nv {
namespace index {

/// Advanced camera projection, like VR/XR lens emulation.
///
/// \ingroup nv_index_scene_description
///
class ICamera_projection :
    public mi::base::Interface_declare<0x806c3f22,0xd4e0,0x420a,0x85,0xe4,0xf7,0x88,0x54,0x7b,0x2e,0x23,
                                       mi::neuraylib::IElement>
{
public:
    /// Data format of the projection texture.
    enum Texel_data_format : mi::Uint32
    {
        FLOAT32_3                         ///< Three float32 values per texel.
    };

    /// Supported projection mode values, bit flags.
    enum Projection_mode : mi::Uint32
    {
        NONE                     = 0x00u, ///< No projection.
        WARP_PIECEWISE_QUADRATIC = 0x01u, ///< Use pixel warping, see \c set_warp_parameters().
        PROJECTION_TEXTURE       = 0x02u  ///< Use projection texture, see \c set_projection_texture_data().
    };

    /// Set mode of projection, valid modes are defined in the \c Projection_mode enum.
    /// \param[in] mode  Bit flag of active modes.
    /// \return true, when \c mode was valid.
    virtual bool set_projection_mode(mi::Uint32 mode) = 0;

    /// Returns the current projection mode.
    /// \return bit flag with values from \c Projection_mode.
    virtual mi::Uint32 get_projection_mode() const = 0;

    /// Set projection texture data for mode \c PROJECTION_TEXTURE.
    /// \param[in] texel_data    Point to texture data in the give format.
    /// \param[in] width         Width of the texture.
    /// \param[in] height        Height of the texture.
    /// \param[in] texel_format  Format of the texels in the texture.
    /// \return true, if the texture data was set successfully.
    virtual bool set_projection_texture_data(
        const void*       texel_data,
        mi::Uint32        width,
        mi::Uint32        height,
        Texel_data_format texel_format) = 0;

    /// Set parameters for pixel warping.
    /// \param[in] parameters      generic array of parameters
    /// \param[in] nb_parameters   number of parameters (24 for \c WARP_PIECEWISE_QUADRATIC)
    virtual void set_warp_parameters(
        const mi::Float32* parameters,
        mi::Uint32         nb_parameters) = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_ICAMERA_PROJECTION_H
