/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

// # RTC Kernel:
// ** Structured Volume Basic **

// # Summary:
// A simple volume kernel for unstructure volumes.
// It samples the volume scalar value for the current position and
// maps it to color through the colormap. Finally allows the users to change
// the contrast of the volume on the fly through the "pfloat 1" GUI parameter.

NV_IDX_XAC_VERSION_1_0

// DON'T CHANGE THIS STRUCT !!!
// It maps the GUI parameters from the Custom Visual Element GUI to this kernel.
// floats maps {pfloat1, pfloat2, pfloat3, pfloat4} GUI Parameters.
// ints maps {pint1, pint2, pint3, pint4} GUI Parameters.
struct Custom_params
{
    float4 floats;  // floats array
    int4 ints;      // ints array
};

using namespace nv::index::xac;
using namespace nv::index::xaclib;

class Volume_sample_program
{
    NV_IDX_VOLUME_SAMPLE_PROGRAM

    const Custom_params*  m_custom_params;

public:
    NV_IDX_DEVICE_INLINE_MEMBER
    void initialize()
    {
        // maps Custom Visual Element GUI parameters to this kernel.
        m_custom_params = state.bind_parameter_buffer<Custom_params>(0);
    }

    NV_IDX_DEVICE_INLINE_MEMBER
    int execute(
        const Sample_info_self& sample_info,
              Sample_output&    sample_output) const
    {
        // maps "pfloat 1" GUI parameter as gamma correction
        float screen_gamma = (m_custom_params->floats.x/100.f + 1.f)/2.f;
        if(screen_gamma < 0.001f)
          screen_gamma = 0.001f;

        // sample volume
        const auto& cell_info = sample_info.sample_cell_info;
        float sample_value = sample_value = state.self.fetch_attribute<float>(0u, cell_info);

        // get colormap
        const auto colormap = state.self.get_colormap();

        // lookup color value
        sample_output.color = colormap.lookup(sample_value);

        // apply gamma correction
        sample_output.color = gamma_correct(sample_output.color, screen_gamma);

        return NV_IDX_PROG_OK;
    }
}; // class Volume_sample_program
