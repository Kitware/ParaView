/* Copyright 2019 NVIDIA Corporation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* * Redistributions of source code must retain the above copyright
*  notice, this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
*  notice, this list of conditions and the following disclaimer in the
*  documentation and/or other materials provided with the distribution.
* * Neither the name of NVIDIA CORPORATION nor the names of its
*  contributors may be used to endorse or promote products derived
*  from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
// # RTC Kernel:
// ** Volume Depth Enhancement **
// # Summary:
// Apply a local front-to-back averaging along a predefined ray segment to darken samples in low
// alpha regions.
// Define the user-defined data structure

NV_IDX_XAC_VERSION_1_0

struct Depth_enhancement_params
{
    float rh;
    // common lighting params
    int light_mode;   // 0=headlight, 1=orbital
    float angle;    // 0 angle
    float elevation;  // pi/2 angle
    int max_dsteps;   // = 8 [GUI] number of additional samples
    float screen_gamma; // 0.9f [GUI] gamma correction parameter

    // shading parameters [GUI / scene]
    float3 spec_color; // make_float3(1.0f) specular color
    float spec_fac;  // 0.2f specular factor (phong)
    float shininess;  // 50.0f shininess parameter (phong)
    float amb_fac;   // 0.4f ambient factor
};

using namespace nv::index;
using namespace nv::index::xac;

class Volume_sample_program
{
    NV_IDX_VOLUME_SAMPLE_PROGRAM

    const uint field_index      = 0u;
    const float min_grad_length = 0.001f;
    const float shade_h         = 0.5f; // min alpha value for shading
    const float min_alpha       = 0.001f; // min alpha for sampling

public:
    const Depth_enhancement_params*
    m_depth_enhancement_params; // define variables to bind user-defined buffer to

public:
    NV_IDX_DEVICE_INLINE_MEMBER
    void initialize()
    {
        // Bind the contents of the buffer slot 0 to the variable
        m_depth_enhancement_params = state.bind_parameter_buffer<Depth_enhancement_params>(0);
    }
    NV_IDX_DEVICE_INLINE_MEMBER
    int execute(
        const   Sample_info_self&  sample_info,
                Sample_output&   sample_output)
    {

        // retrieve ray intersection properties
        const float3& scene_position = sample_info.scene_position;
        const float3& ray_dir = sample_info.ray_direction;

        // get sparse volume reference
        const auto& cell_info = sample_info.sample_cell_info;
        const auto& irregular_volume = state.self;

        // get the associated colormap
        const Colormap colormap = irregular_volume.get_colormap();

        // retrieve user parameter buffer contents
        float3 light_dir = ray_dir;

        if (m_depth_enhancement_params->light_mode != 0)
        {
            const float theta = m_depth_enhancement_params->angle;
            const float phi = m_depth_enhancement_params->elevation;
            light_dir = make_float3(sinf(phi) * cosf(theta), sinf(phi) * sinf(theta), cosf(phi));
        }

        // sample volume along the ray
        const float rh = m_depth_enhancement_params->rh;
        const float3 p0 = scene_position;
        const float3 p1 = p0 + (ray_dir * rh);
        const float3 v01 = (ray_dir * rh);

        const float vs_p0 = irregular_volume.fetch_attribute<float>(field_index, cell_info);

        // init sample color sample
        float4 sample_color  = colormap.lookup(vs_p0);
        sample_output.set_color(sample_color);

        // stop computation if opacity is below threshold (improves performance)
        if (sample_color.w < min_alpha)
        {
            return NV_IDX_PROG_OK;
        }

        // check the number of steps to take along the ray
        if (m_depth_enhancement_params->max_dsteps >= 2)
        {
            // init result color
            float4 acc_color = make_float4(
              sample_color.x * sample_color.w,
              sample_color.y * sample_color.w,
              sample_color.z * sample_color.w,
              sample_color.w);

            // iterate steps
            for (int ahc=1; ahc < m_depth_enhancement_params->max_dsteps; ahc++)
            {
                // get step size
                const float rt = float(ahc) / float(m_depth_enhancement_params->max_dsteps);
                const float3 pi = p0 + (p1 - p0)*rt;
                const float3 v0i = pi - p0;

                // sample current position
                const float vs_pi = irregular_volume.fetch_attribute_offset<float>(field_index, cell_info, v0i);
                const float4 cur_col  = colormap.lookup(vs_pi);

                // front-to-back blending
                const float omda = (1.0f - acc_color.w);

                acc_color.x += omda * cur_col.x * cur_col.w;
                acc_color.y += omda * cur_col.y * cur_col.w;
                acc_color.z += omda * cur_col.z * cur_col.w;
                acc_color.w += omda * cur_col.w;
            }

            // assign result color
            sample_color = acc_color;
            sample_output.set_color(sample_color);
        }

        // check if local shading has to be applied
        if (sample_color.w > shade_h)
        {
            // get the volume gradient
            const float3 vs_grad = get_gradient_3n(sample_info, vs_p0, rh);

            // check gradient length
            if (length(vs_grad) < min_grad_length)
              return NV_IDX_PROG_OK;

            // get isosurface normal
            const float3 iso_normal = -normalize(vs_grad);

            // set ambient & diffuse color
            const float3 diffuse_color = make_float3(sample_color);

            // init shading parameters
            const float lambertian = fabsf(dot(light_dir,iso_normal));
            float spec_fac = 0.0f;

            // check normal direction (two-sided shading)
            if(lambertian > 0.0f)
            {
                // this is blinn phong
                const float3 half_dir = normalize(light_dir + ray_dir);
                const float spec_angle = fabsf(dot(half_dir, iso_normal));

                spec_fac = powf(spec_angle, m_depth_enhancement_params->shininess);
            }

            // compute final shaded color (RGB)
            float4 color_linear = make_float4(diffuse_color * (m_depth_enhancement_params->amb_fac + lambertian) + m_depth_enhancement_params->spec_color * (spec_fac * m_depth_enhancement_params->spec_fac));

            // apply gamma correction
            color_linear = xaclib::gamma_correct(color_linear, m_depth_enhancement_params->screen_gamma);
            color_linear.w = sample_color.w;
            color_linear = clamp(color_linear, 0.0f, 1.0f);

            // clamp result color
            sample_output.set_color(color_linear);
        }

        return NV_IDX_PROG_OK;
    }

    // compute gradient for irregular volume
    NV_IDX_DEVICE_INLINE_MEMBER
    float3 get_gradient_3n(const Sample_info_self&  sample_info, float vs_c, float rh)
    {
        const float dh = 0.5f*rh;
        const float dh_inv = 1.0f/rh;
        const auto& cell_info = sample_info.sample_cell_info;
        const auto& ivol = state.self;

        // get spatial sample points for each dimensions
        // const float vs_c = ivol.fetch_attribute<float>(0u, cell_info);

        const float vs_dx_p = ivol.fetch_attribute_offset<float>(0u, cell_info, make_float3( dh, 0, 0));
        const float vs_dy_p = ivol.fetch_attribute_offset<float>(0u, cell_info, make_float3(  0, dh, 0));
        const float vs_dz_p = ivol.fetch_attribute_offset<float>(0u, cell_info, make_float3(  0, 0, dh));

        // get R3 gradient vector
        return make_float3(
            (vs_dx_p - vs_c) * dh_inv,
            (vs_dy_p - vs_c) * dh_inv,
            (vs_dz_p - vs_c) * dh_inv);

    }
}; // class Volume_sample_program
