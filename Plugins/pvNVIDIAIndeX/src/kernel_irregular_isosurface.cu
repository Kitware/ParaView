/* Copyright 2019 NVIDIA Corporation. All rights reserved.
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
// ** Volume Isosurface Raycaster **

// # Summary:
// Compute the intersection of an isosurface with a user-controlled iso value along the current
// ray segment and shade using a fixed Blinn-Phong model.

// Define the user-defined data structure
struct Isosurface_params
{
  float rh;        // raycast step size
  // Common lighting parameter.
  int light_mode;  // 0=headlight, 1=orbital
  float angle;     // 0.0 angle
  float elevation; // 0.0 elevation

  float iso_min;   // 0.5, iso value in %
  float iso_max;   // 0.5, iso value in %
  int fill_up;     // 1
  int use_shading; // 1, use local phong-blinn model

  float spec_fac;  // 1.0f, specular level (phong)
  float shininess; // 50.0f, shininess parameter (phong)
  float amb_fac;   // 0.2f, ambient factor

  float3 spec_color; // make_float3(1.0f), specular color

  float dummy[2]; // for memory 16 bytes alignment
};

using namespace nv::index;
using namespace nv::index::xac;

class Volume_sample_program
{
  NV_IDX_VOLUME_SAMPLE_PROGRAM

  const uint field_index  = 0u;
  const float min_alpha   = 0.05f; // finite difference
  const float diff_exp    = 2.0f; // diffuse falloff (like edge enhance)

public:
  const Isosurface_params* m_isosurface_params; // define variables to bind user-defined buffer to

public:
  NV_IDX_DEVICE_INLINE_MEMBER
  void initialize()
  {
    // Bind the contents of the buffer slot 0 to the variable
    m_isosurface_params = state.bind_parameter_buffer<Isosurface_params>(0);
  }

  NV_IDX_DEVICE_INLINE_MEMBER
  int execute(const Sample_info_self&   sample_info,
                    Sample_output&      sample_output)
  {
    const auto& cell_info = sample_info.sample_cell_info;
    const auto& volume = state.self;
    const float3 ray_dir = sample_info.ray_direction;
    const Colormap& colormap = volume.get_colormap();

    // retrieve parameter buffer contents (fixed values in code definition)
    float iso_min = (1.0f - m_isosurface_params->iso_min) * colormap.get_domain_min() +
      m_isosurface_params->iso_min * colormap.get_domain_max();
    float iso_max = (1.0f - m_isosurface_params->iso_max) * colormap.get_domain_min() +
      m_isosurface_params->iso_max * colormap.get_domain_max();

    if (iso_max < iso_min)
      return NV_IDX_PROG_DISCARD_SAMPLE;

    // sample volume and colormap
    const float volume_sample = volume.fetch_attribute<float>(field_index, cell_info);

    // get spatial sample points for each dimensions
    const float rh = m_isosurface_params->rh;
    const float dh = 0.1f*rh;
    const float vs_dr_p = volume.fetch_attribute_offset<float>(field_index, cell_info, ray_dir * rh);
    const float vs_dr_n = volume.fetch_attribute_offset<float>(field_index, cell_info, - ray_dir * rh);

    // sum up threshold exceeds in both directions for iso_min
    float sum_over_min = float((vs_dr_p > iso_min) + (vs_dr_n > iso_min) + (volume_sample > iso_min));
    float sum_under_min = float((vs_dr_p < iso_min) + (vs_dr_n < iso_min) + (volume_sample < iso_min));

    // check for iso_min intersections
    if (sum_over_min > 0 && sum_under_min > 0)
    {
      // sample color
      const float4 sample_color = colormap.lookup(iso_min); // use isovalue color

      // check if to skip sample
      if (sample_color.w < min_alpha)
        return NV_IDX_PROG_DISCARD_SAMPLE;

      // valid intersection found
      if (m_isosurface_params->use_shading)
      {
        const float3 iso_normal =
          -normalize(xaclib::volume_gradient(volume, cell_info, dh)); // get isosurface normal
        sample_output.color = blinn_shader(iso_normal, sample_color, ray_dir);
      }
      else
      {
        // use sample color
        sample_output.color = sample_color;
        sample_output.color.w = 1.0f;
      }

      return NV_IDX_PROG_OK;
    }
    else if(sum_over_min >= 3)
    {
      // sum up threshold exceeds in both directions for iso_max
      float sum_over_max = float((vs_dr_p > iso_max) + (vs_dr_n > iso_max) + (volume_sample > iso_max));
      float sum_under_max = float((vs_dr_p < iso_max) + (vs_dr_n < iso_max) + (volume_sample < iso_max));

      // check for iso_max intersections
      if (sum_over_max > 0 && sum_under_max > 0)
      {
        // sample color
        const float4 sample_color = colormap.lookup(iso_max); // use isovalue color

        // check if to skip sample
        if (sample_color.w < min_alpha)
          return NV_IDX_PROG_DISCARD_SAMPLE;

        // valid intersection found
        if (m_isosurface_params->use_shading)
        {
          const float3 iso_normal =
            -normalize(xaclib::volume_gradient(volume, cell_info, dh)); // get isosurface normal
          sample_output.color = blinn_shader(iso_normal, sample_color, ray_dir);
        }
        else
        {
          // use sample color
          sample_output.color = sample_color;
          sample_output.color.w = 1.0f;
        }

        return NV_IDX_PROG_OK;
      }
      else if(sum_under_max >=3 && m_isosurface_params->fill_up > 0)
      {
        if(m_isosurface_params->fill_up == 1)
        {
          // use iso_min color
          sample_output.color = colormap.lookup(iso_min);
          sample_output.color.w = 1.0f;
          return NV_IDX_PROG_OK;
        }
        else
        {
          // use sample color
          sample_output.color = colormap.lookup(volume_sample);
          return NV_IDX_PROG_OK;
        }
      }
      else
      {
        // no isosurface intersection
        return NV_IDX_PROG_DISCARD_SAMPLE;
      }
    }
    else
    {
      // no isosurface intersection
      return NV_IDX_PROG_DISCARD_SAMPLE;
    }
  }

  NV_IDX_DEVICE_INLINE_MEMBER
  float4 blinn_shader(const float3& normal, const float4& sample_color, const float3& ray_dir)
  {
    const float3 diffuse_color = make_float3(sample_color);

    float NL, NH;
    if (m_isosurface_params->light_mode == 0)
    {
      NL = NH = fabsf(dot(ray_dir, normal));
    }
    else
    {
      const float theta = m_isosurface_params->angle;
      const float phi = m_isosurface_params->elevation;
      float3 light_dir = make_float3(sinf(phi) * cosf(theta), sinf(phi) * sinf(theta), cosf(phi));

      NL = fabsf(dot(light_dir, normal));
      float3 H = normalize(light_dir + ray_dir);
      NH = fabsf(dot(H, normal));
    }

    const float diff_amnt = powf(NL, diff_exp);
    const float spec_amnt = powf(NH, m_isosurface_params->shininess);

    // compute final color (RGB)
    const float3 shade_color = diffuse_color * (m_isosurface_params->amb_fac + diff_amnt) +
      m_isosurface_params->spec_color * (m_isosurface_params->spec_fac * spec_amnt);

    return clamp(
      make_float4(shade_color.x, shade_color.y, shade_color.z, sample_color.w), 0.0f, 1.0f);
  }
}; // class Volume_sample_program
