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
// ** Volume Alpha-Gradient Shading Raycaster **

// # Summary:
// Computes the alpha gradient of the volume & transfer function along a ray and applies Phong
// shading above a user defined threshold.

// Define the user-defined data structure
struct Isoraycast_params
{
  // common lighting params
  int light_mode;  // 0=headlight, 1=orbital
  float angle;     // 0.0 angle
  float elevation; // 0.0 elevation

  float diff_h; // 0.0f [0, 2] [GUI] transfer function gradient threshold to trigger phong shading
  uint use_shading; // 1 [GUI] use local phong-blinn model

  // shading [GUI / scene]
  float3 spec_color; // make_float3(1.0f) specular color
  float spec_fac;    // specular factor
  float shininess;   // 50.0f shininess parameter (phong)
  float amb_fac;     // 0.2f ambient factor
  float diff_exp;    // 2.0f diffuse falloff (like edge enhance)
};

using namespace nv::index::xac;
using namespace nv::index::xaclib;

class Volume_sample_program
{
  NV_IDX_VOLUME_SAMPLE_PROGRAM

public:
  const Isoraycast_params* m_isoraycast_params; // define variables to bind user-defined buffer to

public:
  NV_IDX_DEVICE_INLINE_MEMBER
  void initialize()
  {
    // Bind the contents of the buffer slot 0 to the variable
    m_isoraycast_params = state.bind_parameter_buffer<Isoraycast_params>(0);
  }

  NV_IDX_DEVICE_INLINE_MEMBER
  int execute(const Sample_info_self&   sample_info,
                    Sample_output&      sample_output)
  {
    const auto& volume = state.self;
    const float3& sample_position = sample_info.sample_position;
    const float3 ray_dir = sample_info.ray_direction;
    const Colormap& colormap = volume.get_colormap();

    const float volume_sample = volume.sample<float>(sample_position);

    // get spatial sample points for each dimensions
    const float rh = state.self.get_stepsize_min(); // ray sampling difference
    const float vs_dr_p = volume.sample<float>(sample_position + ray_dir * rh);
    const float vs_dr_n = volume.sample<float>(sample_position - ray_dir * rh);

    const float4 c0 = colormap.lookup(vs_dr_p);
    const float4 c1 = colormap.lookup(volume_sample);
    const float4 c2 = colormap.lookup(vs_dr_n);

    // sum up threshold exceeds in both directions
    const float smin = min(c0.w, min(c1.w, c2.w));
    const float smax = max(c0.w, max(c1.w, c2.w));

    // check for iso intersections
    if ((smax - smin) >= m_isoraycast_params->diff_h)
    {
      // valid intersection found
      if (m_isoraycast_params->use_shading)
      {
        sample_output.color = blinn_shader(sample_position, c1, ray_dir);
      }
      else
      {
        // use sample color
        sample_output.color = c1;
        sample_output.color.w = 1.0f;
      }

      return NV_IDX_PROG_OK;
    }
    else
    {
      // use sample color
      sample_output.color = c1;
    }

    return NV_IDX_PROG_OK;
  }

  NV_IDX_DEVICE_INLINE_MEMBER
  void gamma(float4& out_color, const float gamma)
  {
    // apply gamma correction
    out_color.x = powf(out_color.x, float(1.0f / gamma));
    out_color.y = powf(out_color.y, float(1.0f / gamma));
    out_color.z = powf(out_color.z, float(1.0f / gamma));
  }

  NV_IDX_DEVICE_INLINE_MEMBER
  float4 blinn_shader(const float3& sample_position, const float4& sample_color, const float3& ray_dir)
  {
    // get isosurface normal
    const float3 iso_normal = -normalize(state.self.get_gradient(sample_position));

    float3 light_dir;
    if (m_isoraycast_params->light_mode == 0)
    {
      light_dir = ray_dir;
    }
    else
    {
      const float theta = m_isoraycast_params->angle;
      const float phi = m_isoraycast_params->elevation;
      light_dir = make_float3(sinf(phi) * cosf(theta), sinf(phi) * sinf(theta), cosf(phi));
    }

    // diffuse term correction (edge enhance)
    const float diff_amnt = fabsf(dot(light_dir, iso_normal));
    const float diff_fac = powf(diff_amnt, m_isoraycast_params->diff_exp);

    // specular term
    float spec_amnt = 0.0f;

    if (diff_amnt > 0.0f)
    {
      const float3 H = normalize(light_dir + ray_dir);
      const float NH = fabsf(dot(H, iso_normal));
      spec_amnt = powf(NH, m_isoraycast_params->shininess);
    }

    // compute final color (RGB)
    const float3 shade_color =
      make_float3(sample_color) * (m_isoraycast_params->amb_fac + diff_amnt * diff_fac) +
      m_isoraycast_params->spec_color * (spec_amnt * m_isoraycast_params->spec_fac);

    return clamp(
      make_float4(shade_color.x, shade_color.y, shade_color.z, sample_color.w), 0.0f, 1.0f);
  }
}; // class Volume_sample_program
