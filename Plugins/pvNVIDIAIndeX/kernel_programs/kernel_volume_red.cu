/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

// # RTC Kernel:
// ** Volume red **

// # Summary:
// A very simple volume kernel that creates an red volume full opaque.

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
        const Sample_info_self&  sample_info,
              Sample_output&     sample_output)
    {
        // Set the current volume sample to be red full opaque.
        sample_output.color = make_float4(1.f, 0.f, 0.f, 1.f);

        return NV_IDX_PROG_OK;
    }
}; // class Volume_sample_program
