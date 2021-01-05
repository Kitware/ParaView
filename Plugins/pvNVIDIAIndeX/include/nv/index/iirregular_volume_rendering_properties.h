/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/**
   \file
   \brief        Scene attribute controlling irregular volume rendering.
*/


#ifndef NVIDIA_INDEX_IIRREGULAR_VOLUME_RENDERING_PROPERTIES_H
#define NVIDIA_INDEX_IIRREGULAR_VOLUME_RENDERING_PROPERTIES_H

#include <mi/dice.h>
#include <nv/index/iattribute.h>


namespace nv {
namespace index {

/// Diagnostic rendering properties for irregular volumes.
///
/// \ingroup nv_index_scene_description_attribute
///
class IIrregular_volume_rendering_properties :
    public mi::base::Interface_declare<0x72327639,0xd6ed,0x4fc9,0xba,0x2f,0x92,0xf3,0x94,0x2a,0xee,0x7c,
                                       nv::index::IAttribute>
{
public:
    /// Get default subregion halo size, in object space.
    /// Subregions are expanded by the halo size for data loading.
    /// Similar to subcube_border_size, it enables filtering and it defines the step size limit 
    /// for discrete volume sampling.
    /// \return     Returns the halo size.
    virtual mi::Float32 get_halo_size() const = 0;
    /// Set default subregion halo size, in object space.
    /// \param[in] f    The size of the halo.
    virtual void        set_halo_size(mi::Float32 f) = 0;

    /// \name Render sampling settings
    ///@{

    /// Get render sampling mode (0 = preintegrated colormap; 1 = discrete sampling)
    /// \return         Returns the render sampling mode.
    virtual mi::Uint32  get_sampling_mode() const = 0;
    /// Set render sampling mode (0 = preintegrated colormap; 1 = discrete sampling)
    /// \param[in] m    Render sampling mode.
    virtual void        set_sampling_mode(mi::Uint32 m) = 0;

    /// Get length of discrete sampling segment length on ray.
    /// Should be less than subregion halo size to avoid artifacts.
    /// \return         Returns the segment length.
    virtual mi::Float32 get_sampling_segment_length() const = 0;
    /// Set length of discrete sampling segment length on ray.
    /// \param[in] l    The segment length.
    virtual void        set_sampling_segment_length(mi::Float32 l) = 0;
        
    /// Get reference length of discrete sampling segment length on ray.
    /// \return         Returns the reference length.
    virtual mi::Float32 get_sampling_reference_segment_length() const = 0;
    /// Set reference length of discrete sampling segment length on ray.
    /// \param[in] l    The reference length.
    virtual void        set_sampling_reference_segment_length(mi::Float32 l) = 0;
    
    ///@}


    /// \name Diagnostic rendering settings
    ///@{

    /// Get diagnostic rendering mode.
    /// \return Returns the diagnostic rendering is performed instead of normal rendering (1:wireframe, 2:run path)
    ///         or 0 otherwise.
    virtual mi::Uint32  get_diagnostics_mode() const = 0;
    /// Get diagnostic rendering mode.
    /// \param[in] m    Set the diagnostic rendering mode (0: no diagnostics, 1: wireframe, 2: run path)
    virtual void        set_diagnostics_mode(mi::Uint32 m) = 0;

    /// Get bit flags to enable various diagnostics (internal).
    /// \return     The bit flags currently set.
    virtual mi::Uint32  get_diagnostics_flags() const = 0;
    /// Set bit flags to enable various diagnostics (internal).
    /// \param[in] f    Flags for diagnostic rendering.
    virtual void        set_diagnostics_flags(mi::Uint32 f) = 0;

    /// Get world space size of wireframe mode lines.
    /// \return         Returns the width of the wire frame lines.
    virtual mi::Float32 get_wireframe_size() const = 0;
    /// Set world space size of wireframe mode lines.
    /// \param[in] s    Sets the width of the wire frame lines.
    virtual void        set_wireframe_size(mi::Float32 s) = 0;

    /// Get distance from camera where color modulation starts.
    /// \return         Returns the distance value.
    virtual mi::Float32 get_wireframe_color_mod_begin() const = 0;
    /// Set distance from camera where color modulation starts.
    /// \param[in] f    The distance value.
    virtual void        set_wireframe_color_mod_begin(mi::Float32 f) = 0;

    /// Get distance factor. Distance is multiplied by factor for color modulation. 0 disables color modulation.
    /// \return         Returns the color modulation factor.
    virtual mi::Float32 get_wireframe_color_mod_factor() const = 0;
    /// Set distance factor. Distance is multiplied by factor for color modulation. 0 disables color modulation.
    /// \param[in] f    The color modulation factor.
    virtual void        set_wireframe_color_mod_factor(mi::Float32 f) = 0;
    
    ///@}
};

} // namespace index
} // namespace nv

#endif
