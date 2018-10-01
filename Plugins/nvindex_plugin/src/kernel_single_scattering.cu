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
// ** Volume Single-Scattering **

// # Summary:
// Trace an additional shadow ray in a user-controlled direction to darken the current sample
// color.
// NOTE: creates artifacts if shadow ray crosses subregion boundaries

// Define the user-defined data structure
struct Single_scattering_params
{
  // common lighting params
  int light_mode;       // 0=headlight, 1=orbital
  float angle;          // 0.0f [GUI] shadow ray angle around axis
  float elevation;      // pi/2 [GUI] shadow ray elevation around axis
  int steps;            // 50 [GUI] shadow ray samples (maximum)
  float light_distance; // 100.0f [GUI] light traveling distance (maximum)
  float min_alpha;      // 0.1f [GUI] alpha threshold for scattering
  float max_shadow;     // 0.2f [GUI] darkest shadow factor
  float shadow_exp;     // 1.5f [GUI] shadow dampening
  float shadow_offset;  // 0.05f offset shadow ray (avoids voxel self darkening)

  // shading
  uint two_sided;   // true
  uint use_shading; // true [GUI] use local shading
  float amb_fac;    // 0.8f ambient factor
  float spec_fac;   // 0.5f specular factor
  float shininess;  // 50   speculat shininess

  // volume
  float dh;             // 1.0f finite difference (gradient approximation)
  uint volume_id;       // 0u volume to sample
  float min_samp_alpha; // 0.01f minimum processing alpha

  float3 dummy;
};

using namespace nv::index::xac;
using namespace nv::index::xaclib;

class Volume_sample_program
{
  NV_IDX_VOLUME_SAMPLE_PROGRAM

public:
  const Single_scattering_params*
    m_single_scattering_params; // define variables to bind user-defined buffer to

public:
  NV_IDX_DEVICE_INLINE_MEMBER
  void initialize()
  {
    // Bind the contents of the buffer slot 0 to the variable
    m_single_scattering_params = state.bind_parameter_buffer<Single_scattering_params>(0);
  }

  NV_IDX_DEVICE_INLINE_MEMBER
  int execute(const Sample_info_self&   sample_info,
                    Sample_output&      sample_output)
  {
    // retrieve parameter buffer contents (fixed values in code definition)
    const auto& volume = state.scene.access_by_id<Regular_volume>(m_single_scattering_params->volume_id);
    const float3& sample_position = sample_info.sample_position;
    const Colormap& colormap = state.self.get_colormap();

    // sample volume and colormap
    const float volume_sample = volume.sample<float>(sample_position);
    const float4 sample_color = colormap.lookup(volume_sample);

    if (sample_color.w < m_single_scattering_params->min_samp_alpha)
    {
      sample_output.color = sample_color;
      return NV_IDX_PROG_DISCARD_SAMPLE;
    }

    // setup light direction
    float3 light_dir;
    if (m_single_scattering_params->light_mode == 0)
    {
      light_dir = state.scene.camera.get_to() * m_single_scattering_params->light_distance;
    }
    else
    {
      const float theta = m_single_scattering_params->angle;
      const float phi = m_single_scattering_params->elevation;
      light_dir = make_float3(sinf(phi) * cosf(theta), sinf(phi) * sinf(theta), cosf(phi)) *
        m_single_scattering_params->light_distance;
    }

    const float3 world_pos = sample_info.scene_position;
    const float3 light_pos = world_pos + light_dir;
    const float3 light_off = light_dir *
      m_single_scattering_params->shadow_offset; // offset to avoid self darkening of voxels

    // init output color
    if (m_single_scattering_params->use_shading)
    {
      sample_output.color = blinn_shading(
        sample_position, sample_color, -normalize(light_dir), sample_info.ray_direction, m_single_scattering_params->dh);
      sample_output.color.w = sample_color.w;
    }
    else
    {
      sample_output.color = sample_color;
    }

    // check for iso intersections
    if (sample_color.w > m_single_scattering_params->min_alpha)
    {
      // init shadow value
      float acc_shadow = 1.0f;
      int scc = 0;

      // iterate shadow samples
      for (int sc = 0; sc < m_single_scattering_params->steps; sc++)
      {
        scc++;
        float sample_value = 0.0f;

        const float st = (float)(sc) / (float)(m_single_scattering_params->steps - 1.0f);

        const float3 sray_pos = world_pos + st * light_dir + light_off;
        const float3 sub_pos = transform_point(state.self.get_scene_to_sample_transform(), sray_pos);

        sample_value = volume.sample<float>(sub_pos);

        // if (volume.is_inside(world_pos))
        if (volume.is_inside(sub_pos))
        {
          const float4 rs_color = colormap.lookup(sample_value);
          const float voxel_alpha = rs_color.w;

          // accumulate darkening along ray
          acc_shadow *= (1.0f - powf(voxel_alpha, m_single_scattering_params->shadow_exp));

          if (acc_shadow < m_single_scattering_params->max_shadow)
          {
            acc_shadow = m_single_scattering_params->max_shadow;
            break;
          }
        }
        else
          break;
      }

      // apply shadow correction
      sample_output.color *= fmaxf(fminf(acc_shadow, 1.0f), 0.0f);
      sample_output.color.w = sample_color.w;
    }

    return NV_IDX_PROG_OK;
  }

  NV_IDX_DEVICE_INLINE_MEMBER
  float4 blinn_shading(const float3& sample_position,
    const float4& sample_color,
    const float3& light_dir,
    const float3& view_dir,
    const float dh)
  {
    // define parameters
    const float3 diff_color = make_float3(sample_color);
    const float3 spec_color = make_float3(1.0f); // specular color
    const float screen_gamma = 0.7f;             // gamma correction parameter

    // get R3 gradient vector
    const float3 vs_grad = state.self.get_gradient(sample_position, dh); // compute R3 gradient
    const float3 iso_normal = -normalize(vs_grad);                       // get isosurface normal

    const float diff_amnt = fabsf(dot(light_dir, iso_normal)); // two sided shading
    float spec_amnt = 0.0f;

    if (diff_amnt > 0.0f)
    {
      // this is blinn phong
      float3 H = normalize(light_dir + view_dir);
      float NH = fabsf(dot(H, iso_normal)); // two sided shading
      spec_amnt = powf(NH, m_single_scattering_params->shininess);
    }

    // compute final color (RGB)
    const float3 color_linear = diff_color * (m_single_scattering_params->amb_fac + diff_amnt) +
      spec_color * (spec_amnt * m_single_scattering_params->spec_fac);

    // apply gamma correction
    float4 color_gcorrect = make_float4(color_linear.x, color_linear.y, color_linear.z, 1.0f);
    color_gcorrect.x = powf(color_gcorrect.x, float(1.0f / screen_gamma));
    color_gcorrect.y = powf(color_gcorrect.y, float(1.0f / screen_gamma));
    color_gcorrect.z = powf(color_gcorrect.z, float(1.0f / screen_gamma));
    color_gcorrect.w = sample_color.w;

    return clamp(color_gcorrect);
  }

  NV_IDX_DEVICE_INLINE_MEMBER
  float4 clamp(const float4& color)
  {
    return make_float4(max(min(color.x, 1.0f), 0.0f),
      max(min(color.y, 1.0f), 0.0f),
      max(min(color.z, 1.0f), 0.0f),
      max(min(color.w, 1.0f), 0.0f));
  }
}; // class Volume_sample_program
