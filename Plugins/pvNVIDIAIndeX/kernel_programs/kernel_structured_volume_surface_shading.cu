/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

// # XAC Kernel:
// ** Structured Volume Surface Shading **

// # Summary:
// Render the structured volume and optionally apply local surface shading at an user selected scalar threshold

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

using namespace nv::index;
using namespace nv::index::xac;

class Volume_sample_program
{
    NV_IDX_VOLUME_SAMPLE_PROGRAM

    // sampling parameters
    const uint field_index      = 0u;       // volume field to use (default: 0)
    const float dh              = 1.0f;     // finite-difference stepsize (gradient approximation)

    // shading parameters
    const float diffuse_falloff = 1.0f;     // angular falloff for diffues lighting term
    const float shininess       = 100.0f;   // 'shininess' falloff parameter (lower is brighter)
    const float ambient_factor  = 0.1f;     // scaling of ambient term

    const float4 specular_color = make_float4(1.0f);

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
              Sample_output&    sample_output)
    {
        // map "pint 1" GUI parameter as use shading option (true/false)
        // enable/disable shading effect
        const bool use_shading = (m_custom_params->ints.x >= 1);

        // map "pfloat 1" GUI parameter as min shading alpha
        // min alpha threshold for shading.
        float min_shade_alpha = m_custom_params->floats.x/100.f;
        if(min_shade_alpha < 0.f)
          min_shade_alpha = 0.f;

        // get current sample position
        const float3& sample_position = sample_info.sample_position_object_space;
        const auto& sample_context = sample_info.sample_context;

        // get reference to the current volume
        const auto& sparse_volume = state.self;

        // generate a volume sampler
        const auto sampler = sparse_volume.generate_sampler<float>(
                                                         field_index,
                                                         sample_context);

        // sample the volume at the current position
        const float sample_value = sampler.fetch_sample(sample_position);

        // sample the color value using the transfer function (colormap)
        const Colormap colormap = state.self.get_colormap();
        float4 sample_color = colormap.lookup(sample_value);

        // check if shading should be used
        if (use_shading && (sample_color.w > min_shade_alpha))
        {
            // make this sample full opaque
            sample_color.w = 1.f;

            // compute volume gradient using a trilinear filter
            const float3 gradient = xaclib::volume_gradient<Volume_filter_mode::TRILINEAR>(
                        sparse_volume,
                        sample_position,
                        field_index,
                        dh);

            // get shading parameters
            const float3& view_direction = sample_info.ray_direction;
            const float3 iso_normal = -normalize(gradient);

            // apply built-in headlight shading
            sample_color = xaclib::headlight_shading(
                        state.scene,
                        iso_normal,
                        view_direction,
                        sample_color,
                        specular_color,
                        diffuse_falloff,
                        shininess,
                        ambient_factor);
        }

        // store the output color
        sample_output.set_color(sample_color);

        return NV_IDX_PROG_OK;
    }
}; // class Volume_sample_program
