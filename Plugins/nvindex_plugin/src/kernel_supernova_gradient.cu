/* Copyright 2018 NVIDIA Corporation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*  * Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*  * Neither the name of NVIDIA CORPORATION nor the names of its
*    contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// # RTC Kernel:
// ** Volume Gradient Colormapping **
// # Summary: Compute the volume gradient and map to the given colormap.

struct Supernova_gradient_params
{
    float gradient_scale;       // gradient value (0.5) [0,1]
    float screen_gamma;         // (0.55)
    int   color_method;         // Gradient Color method (3):
                                // 0: use z gradient only
                                // 1: use x,y gradient only
                                // 2: use gradient magnitude
                                // 3: darken sample color by magnitude
};

using namespace nv::index::xac;
using namespace nv::index::xaclib;

class Volume_sample_program
{
    NV_IDX_VOLUME_SAMPLE_PROGRAM

    const float min_alpha       = 0.001f;    // min alpha to trigger gradient computation
    const float dh              = 2.0f;     // finite-differencing stepsize

   // highlight core region (fixed to dataset)
    const float4 nova_color     = make_float4(1.0f, 0.4f , 0.0f, 1.0f);
    const float3 nova_center    = make_float3(316.5f);
    const float nova_max_dist   = 8.0f;
    const float nova_falloff_exp = 1.75f;

    // Use gradient to set transparency
    const bool use_grad_alpha   = 1;

    const Colormap colormap = state.self.get_colormap();
    const Supernova_gradient_params*  m_params;

public:
    NV_IDX_DEVICE_INLINE_MEMBER
    void initialize()
    {
        m_params = state.bind_parameter_buffer<Supernova_gradient_params>(0);
    }

    NV_IDX_DEVICE_INLINE_MEMBER
    int execute(
        const Sample_info_self&  sample_info,
              Sample_output&     sample_output)
    {
        const float3& sample_position = sample_info.sample_position;
        const float3& scene_position = sample_info.scene_position;

        // highlight core region
        const float dist = length(nova_center - scene_position);
        if (dist < nova_max_dist)
        {
            float nd = 1.0f - (dist / nova_max_dist);
            nd = powf(nd, nova_falloff_exp);

            sample_output.color = (1.0f - nd) * make_float4(1.0f) + nd * nova_color;
            sample_output.color.w = nd;

            return NV_IDX_PROG_OK;
        }

        // sample volume, local gradient and color
        const float  volume_sample = state.self.sample<float>(sample_position);
        const float4 sample_color  = colormap.lookup(volume_sample);

        // check if sample can be skipped
        if (sample_color.w < min_alpha) return NV_IDX_PROG_DISCARD_SAMPLE;

        // compute volume gradient
        const float3 vol_grad = state.self.get_gradient(sample_position, dh);

        // scale gradient by user input
        float grad_scale = m_params[0].gradient_scale;

        if (m_params[0].color_method == 0)
        {
            // color by height gradient
            sample_output.color  = colormap.lookup(vol_grad.z * grad_scale);
        }
        else if (m_params[0].color_method == 1)
        {
            // color by x,y gradient
            const float vs_xy_mag = sqrt(pow(vol_grad.x,2.0f) * pow(vol_grad.y,2.0f));
            sample_output.color  = colormap.lookup(vs_xy_mag * grad_scale);
        }
        else if (m_params[0].color_method == 2)
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
        }

        if (!use_grad_alpha) sample_output.color.w = sample_color.w;

        // apply gamma correction
        sample_output.color = gamma_correct(sample_output.color, m_params[0].screen_gamma);

        return NV_IDX_PROG_OK;
    }
}; // class Volume_sample_program
