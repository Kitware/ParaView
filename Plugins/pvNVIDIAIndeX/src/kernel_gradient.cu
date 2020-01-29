/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

// # RTC Kernel:
// ** Volume Gradient Colormapping **

// # Summary:
// Compute the volume gradient and map to the given colormap.

NV_IDX_XAC_VERSION_1_0

struct Gradient_params
{
    float gradient; // [0.01, 1] gradient value
    float grad_max; // 20.0f maximum gradient scale
    float dummy1;
    float dummy2;
};

using namespace nv::index::xac;
using namespace nv::index::xaclib;

class Volume_sample_program
{
    NV_IDX_VOLUME_SAMPLE_PROGRAM

    const float min_alpha       = 1.0e-6f;      // min alpha to trigger gradient computation
    const float dh              = 1.0f;         // finite-differencing stepsize
    const float alpha_exp       = 1.0f;         // fall-off parameter if gradient is used for transparency

    // Gradient Color method:
    // 0: use z gradient only
    // 1: use x,y gradient only
    // 2: use gradient magnitude
    // 3: darken sample color by magnitude
    const int color_method      = 3;

    // Use gradient to set transparency
    const bool use_grad_alpha   = 0;
    const float screen_gamma    = 0.6f;
    const uint field_index      = 0u;

    const Colormap colormap = state.self.get_colormap();

    const Gradient_params*  m_gradient_params;

public:
    NV_IDX_DEVICE_INLINE_MEMBER
    void initialize()
    {
        m_gradient_params = state.bind_parameter_buffer<Gradient_params>(0);
    }

    NV_IDX_DEVICE_INLINE_MEMBER
    int execute(
        const Sample_info_self&  sample_info,
              Sample_output&     sample_output)
    {
        // get sampling info references
        const float3& sample_position = sample_info.sample_position_object_space;

        // get sparse volume reference
        const auto& sparse_volume = state.self;
        const auto& sample_context = sample_info.sample_context;
        const auto sampler = sparse_volume.generate_sampler<float>(
                                                         field_index,
                                                         sample_context);

        // sample volume, local gradient and color
        const float  volume_sample = sampler.fetch_sample(sample_position);
        const float4 sample_color  = colormap.lookup(volume_sample);

        // check if sample can be skipped
        if (sample_color.w < min_alpha) return NV_IDX_PROG_DISCARD_SAMPLE;

        // compute volume gradient (use trilinear filter for gradients)
        const float3 vol_grad = volume_gradient<Volume_filter_mode::TRILINEAR>(
                    sparse_volume,
                    sample_context,
                    sample_position,
                    dh);

        // scale gradient by user input
        const float grad_scale = m_gradient_params->gradient * m_gradient_params->grad_max;

        if (color_method == 0)
        {
            // color by height gradient
            sample_output.color  = colormap.lookup(vol_grad.z * grad_scale);
        }
        else if (color_method == 1)
        {
            // color by x,y gradient
            const float vs_xy_mag = sqrt(pow(vol_grad.x,2.0f) * pow(vol_grad.y,2.0f));
            sample_output.color  = colormap.lookup(vs_xy_mag * grad_scale);
        }
        else if (color_method == 2)
        {
            // color by gradient magnitude
            const float grad_mag = length(vol_grad);
            sample_output.color  = colormap.lookup(grad_mag * grad_scale);
        }
        else
        {
            // shade color by gradient magnitude
            const float grad_mag = length(vol_grad);
            sample_output.color  = clamp(sample_color * (grad_mag * grad_scale),0.0f,1.0f);
            sample_output.color.w = powf(sample_output.color.w, alpha_exp);
        }

        if (!use_grad_alpha) sample_output.color.w = sample_color.w;

        // apply gamma correction
        sample_output.color = gamma_correct(sample_output.color, screen_gamma);

        return NV_IDX_PROG_OK;
    }
}; // class Volume_sample_program
