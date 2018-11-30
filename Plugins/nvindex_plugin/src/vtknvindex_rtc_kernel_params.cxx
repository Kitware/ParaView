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

#include "vtknvindex_rtc_kernel_params.h"

// kernel sources
const char* KERNEL_ISOSURFACE_STRING = R
  "(/******************************************************************************
  *
  Copyright 2018 NVIDIA Corporation.All rights reserved.*
    **************************************************************************** /

  // # RTC Kernel:
  // ** Volume Isosurface Raycaster **

  // # Summary:
  // Compute the intersection of an isosurface with a user-controlled iso value along the current
  // ray segment and shade using a fixed Blinn-Phong model.

  // Define the user-defined data structure
  struct Isosurface_params
{
  // common lighting params
  int light_mode;  // 0=headlight, 1=orbital
  float angle;     // 0.0 angle
  float elevation; // 0.0 elevation

  float iso_min;   // 0.5, iso value in %
  float iso_max;   // 0.5, iso value in %
  int fill_up;     // 1
  int use_shading; // 1, use local phong-blinn model
  float min_alpha; // 0.05, finite difference

  float spec_fac;  // 1.0f, specular level (phong)
  float shininess; // 50.0f, shininess parameter (phong)
  float amb_fac;   // 0.2f, ambient factor
  float diff_exp;  // 2.0f, diffuse falloff (like edge enhance)

  int show_grid;  // 0, show normal grid
  int ng_num;     // 16
  float ng_width; // 0.01f

  float3 spec_color; // make_float3(1.0f), specular color

  float2 dummy; // for memory 16 bytes alignment
};

NV_IDX_VOLUME_SAMPLE_PROGRAM_PREFIX
class NV_IDX_volume_sample_program
{
  NV_IDX_VOLUME_SAMPLE_PROGRAM

private:
  const float3 ray_dir = state.m_ray_direction;

public:
  const Isosurface_params* m_isosurface_params; // define variables to bind user-defined buffer to

public:
  NV_IDX_DEVICE_INLINE_MEMBER
  void init_instance()
  {
    // Bind the contents of the buffer slot 0 to the variable
    m_isosurface_params = NV_IDX_bind_parameter_buffer<Isosurface_params>(0);
  }

  NV_IDX_DEVICE_INLINE_MEMBER
  int execute(const NV_IDX_sample_info_self& sample_info, float4& output_color)
  {
    const NV_IDX_volume& volume = state.self;
    const float3& sample_position = sample_info.sample_position;
    const NV_IDX_colormap& colormap = volume.get_colormap();

    // retrieve parameter buffer contents (fixed values in code definition)
    float iso_min = (1.0f - m_isosurface_params->iso_min) * colormap.get_domain_min() +
      m_isosurface_params->iso_min * colormap.get_domain_max();
    float iso_max = (1.0f - m_isosurface_params->iso_max) * colormap.get_domain_min() +
      m_isosurface_params->iso_max * colormap.get_domain_max();

    if (iso_max > iso_min)
    {
      const float iso_tmp = iso_min;
      iso_min = iso_max;
      iso_max = iso_tmp;
    }

    // sample volume and colormap
    const float volume_sample = volume.sample<float>(sample_position);

    // get spatial sample points for each dimensions
    const float rh = state.m_ray_stepsize_min; // ray sampling difference
    const float vs_dr_p = volume.sample<float>(sample_position + ray_dir * rh);
    const float vs_dr_n = volume.sample<float>(sample_position - ray_dir * rh);

    // sum up threshold exceeds in both directions for iso_min
    float sum_over = float((vs_dr_p > iso_min) + (vs_dr_n > iso_min) + (volume_sample > iso_min));
    float sum_under = float((vs_dr_p < iso_min) + (vs_dr_n < iso_min) + (volume_sample < iso_min));

    // check for iso_min intersections
    if (sum_over > 0 && sum_under > 0)
    {
      // sample color
      const float4 sample_color = colormap.lookup(iso_min); // use isovalue color
      const float3 iso_normal =
        -normalize(volume.get_gradient(sample_position)); // get isosurface normal

      // check if to skip sample
      if (sample_color.w < m_isosurface_params->min_alpha)
      {
        return NV_IDX_PROG_DISCARD_SAMPLE;
      }
      else
      {
        // show normal grid lines
        if (m_isosurface_params->show_grid)
        {
          // compute angles
          const float ang_ng = M_PI / (float)(m_isosurface_params->ng_num + 1);

          // compute modulo difference
          const float ang_mx = fmodf(acos(iso_normal.x), ang_ng);
          const float ang_my = fmodf(acos(iso_normal.y), ang_ng);
          const float ang_mz = fmodf(acos(iso_normal.z), ang_ng);

          if (fabsf(ang_mx) < m_isosurface_params->ng_width)
          {
            output_color = make_float4(0.5f, 0.f, 0.f, 1.f);
            return NV_IDX_PROG_OK;
          }
          else if (fabsf(ang_my) < m_isosurface_params->ng_width)
          {
            output_color = make_float4(0.f, 0.5f, 0.f, 1.f);
            return NV_IDX_PROG_OK;
          }
          else if (fabsf(ang_mz) < m_isosurface_params->ng_width)
          {
            output_color = make_float4(0.f, 0.f, 0.5f, 1.f);
            return NV_IDX_PROG_OK;
          }
        }

        // valid intersection found
        if (m_isosurface_params->use_shading)
        {
          output_color = blinn_shader(iso_normal, sample_color);
        }
        else
        {
          // use sample color
          output_color = sample_color;
          output_color.w = 1.0f;
        }

        return NV_IDX_PROG_OK;
      }
    }
    else if ((m_isosurface_params->fill_up == 1 && sum_over >= 3) ||
      (m_isosurface_params->fill_up == -1 && sum_under >= 3))
    {
      // use iso_min color
      output_color = colormap.lookup(iso_min);
      output_color.w = 1.0f;

      return NV_IDX_PROG_OK;
    }
    else if ((m_isosurface_params->fill_up == 2 && sum_over >= 3) ||
      (m_isosurface_params->fill_up == -2 && sum_under >= 3))
    {
      // use sample color
      output_color = colormap.lookup(volume_sample);

      return NV_IDX_PROG_OK;
    }
    else
    {
      // no isosurface intersection
      // return NV_IDX_PROG_DISCARD_SAMPLE;
    }

    // sum up threshold exceeds in both directions for iso_max
    sum_over = float((vs_dr_p > iso_max) + (vs_dr_n > iso_max) + (volume_sample > iso_max));
    sum_under = float((vs_dr_p < iso_max) + (vs_dr_n < iso_max) + (volume_sample < iso_max));

    // check for iso_max intersections
    if (sum_over > 0 && sum_under > 0)
    {
      // sample color
      const float4 sample_color = colormap.lookup(iso_max); // use isovalue color
      const float3 iso_normal =
        -normalize(volume.get_gradient(sample_position)); // get isosurface normal

      // check if to skip sample
      if (sample_color.w < m_isosurface_params->min_alpha)
        return NV_IDX_PROG_DISCARD_SAMPLE;

      // show normal grid lines
      if (m_isosurface_params->show_grid)
      {
        // compute angles
        const float ang_ng = M_PI / (float)(m_isosurface_params->ng_num + 1);

        // compute modulo difference
        const float ang_mx = fmodf(acos(iso_normal.x), ang_ng);
        const float ang_my = fmodf(acos(iso_normal.y), ang_ng);
        const float ang_mz = fmodf(acos(iso_normal.z), ang_ng);

        if (fabsf(ang_mx) < m_isosurface_params->ng_width)
        {
          output_color = make_float4(0.5f, 0.f, 0.f, 1.f);
          return NV_IDX_PROG_OK;
        }
        else if (fabsf(ang_my) < m_isosurface_params->ng_width)
        {
          output_color = make_float4(0.f, 0.5f, 0.f, 1.f);
          return NV_IDX_PROG_OK;
        }
        else if (fabsf(ang_mz) < m_isosurface_params->ng_width)
        {
          output_color = make_float4(0.f, 0.f, 0.5f, 1.f);
          return NV_IDX_PROG_OK;
        }
      }

      // valid intersection found
      if (m_isosurface_params->use_shading)
      {
        output_color = blinn_shader(iso_normal, sample_color);
      }
      else
      {
        // use sample color
        output_color = sample_color;
        output_color.w = 1.0f;
      }

      return NV_IDX_PROG_OK;
    }
    else if ((m_isosurface_params->fill_up == 1 && sum_over >= 3) ||
      (m_isosurface_params->fill_up == -1 && sum_under >= 3))
    {
      // use iso_max color
      output_color = colormap.lookup(iso_max);
      output_color.w = 1.0f;

      return NV_IDX_PROG_OK;
    }
    else if ((m_isosurface_params->fill_up == 2 && sum_over >= 3) ||
      (m_isosurface_params->fill_up == -2 && sum_under >= 3))
    {
      // use sample color
      output_color = colormap.lookup(volume_sample);

      return NV_IDX_PROG_OK;
    }
    else
    {
      // no isosurface intersection
      return NV_IDX_PROG_DISCARD_SAMPLE;
    }
  }

  NV_IDX_DEVICE_INLINE_MEMBER
  float4 blinn_shader(const float3& normal, const float4& sample_color)
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

    const float diff_amnt = powf(NL, m_isosurface_params->diff_exp);
    const float spec_amnt = powf(NH, m_isosurface_params->shininess);

    // compute final color (RGB)
    const float3 shade_color = diffuse_color * (m_isosurface_params->amb_fac + diff_amnt) +
      m_isosurface_params->spec_color * (m_isosurface_params->spec_fac * spec_amnt);

    return clamp(
      make_float4(shade_color.x, shade_color.y, shade_color.z, sample_color.w), 0.0f, 1.0f);
  }
}; // class NV_IDX_volume_sample_program)";

const char* KERNEL_DEPTH_ENHANCEMENT_STRING = R
  "(/******************************************************************************
  *
  Copyright 2018 NVIDIA Corporation.All rights reserved.*
    **************************************************************************** /

  // # RTC Kernel:
  // ** Volume Depth Enhancement **

  // # Summary:
  // Apply a local front-to-back averaging along a predefined ray segment to darken samples in low
  // alpha regions.

  // Define the user-defined data structure
  struct Depth_enhancement_params
{
  // common lighting params
  int light_mode;     // 0=headlight, 1=orbital
  float angle;        // 0 angle
  float elevation;    // pi/2 angle
  int max_dsteps;     // = 8 [GUI] number of additional samples
  float ash;          // 0.01f adaptive sampling threshold
  float screen_gamma; // 0.9f [GUI] gamma correction parameter

  // shading parameters [GUI / scene]
  float3 spec_color; // make_float3(1.0f) specular color
  float spec_fac;    // 0.2f specular factor (phong)
  float shininess;   // 50.0f shininess parameter (phong)
  float amb_fac;     // 0.4f ambient factor

  float shade_h;   // 0.5f [GUI] min alpha value for shading
  float min_alpha; // 0.001f min alpha for sampling (improves performance)

  float2 dummy;
};

NV_IDX_VOLUME_SAMPLE_PROGRAM_PREFIX
class NV_IDX_volume_sample_program
{
  NV_IDX_VOLUME_SAMPLE_PROGRAM

public:
  const Depth_enhancement_params*
    m_depth_enhancement_params; // define variables to bind user-defined buffer to

public:
  NV_IDX_DEVICE_INLINE_MEMBER
  void init_instance()
  {
    // Bind the contents of the buffer slot 0 to the variable
    m_depth_enhancement_params = NV_IDX_bind_parameter_buffer<Depth_enhancement_params>(0);
  }

  NV_IDX_DEVICE_INLINE_MEMBER
  int execute(const NV_IDX_sample_info_self& sample_info, float4& output_color)
  {
    const NV_IDX_volume& volume = state.self;
    const float3& sample_position = sample_info.sample_position;
    const NV_IDX_colormap& colormap = volume.get_colormap();

    // retrieve parameter buffer contents (fixed values in code definition)
    const float3 wp = sample_info.scene_position;
    const float3 ray_dir = state.m_ray_direction;

    float3 light_dir;
    if (m_depth_enhancement_params->light_mode == 0)
    {
      light_dir = ray_dir;
    }
    else
    {
      const float theta = m_depth_enhancement_params->angle;
      const float phi = m_depth_enhancement_params->elevation;
      light_dir = make_float3(sinf(phi) * cosf(theta), sinf(phi) * sinf(theta), cosf(phi));
    }

    // sample volume
    const float rh = state.m_ray_stepsize_min * 2.0f; // ray sampling difference
    const float3 p0 = sample_position;
    const float3 p1 = sample_position + (ray_dir * rh);

    const float vs_p0 = volume.sample<float>(p0);
    const float vs_p1 = volume.sample<float>(p1);

    const float vs_min = min(vs_p0, vs_p1);
    const float vs_max = max(vs_p0, vs_p1);

    // set adaptive sampling
    int d_steps = m_depth_enhancement_params->max_dsteps;

    if (m_depth_enhancement_params->ash > 0.0f)
    {
      int((vs_max - vs_min) /
        fabsf(m_depth_enhancement_params->ash)); // get number of additional samples
      d_steps = min(d_steps, m_depth_enhancement_params->max_dsteps);
    }

    // compositing front-to-back
    /*const float4&    result;    // C_dst
    const float4&   color;  // C_src
    const float omda = 1.0f - result.w;
    result.x += omda * color.x;
    result.y += omda * color.y;
    result.z += omda * color.z;
    result.w += omda * color.w;*/

    // sample once
    const float4 sample_color = colormap.lookup(vs_p0);
    output_color = make_float4(sample_color.x, sample_color.y, sample_color.z, sample_color.w);

    if (output_color.w < m_depth_enhancement_params->min_alpha)
      return NV_IDX_PROG_DISCARD_SAMPLE;

    if (d_steps > 1)
    {
      // init result color
      float4 result = make_float4(0.0f);

      // iterate steps
      for (int ahc = 0; ahc < d_steps; ahc++)
      {
        // get step size
        const float rt = float(ahc) / float(d_steps);
        const float3 pi = (1.0f - rt) * p0 + rt * p1;
        const float vs_pi = volume.sample<float>(pi);
        const float4 cur_col = colormap.lookup(vs_pi);

        // front-to-back blending
        const float omda = (1.0f - result.w);

        result.x += omda * cur_col.x * cur_col.w;
        result.y += omda * cur_col.y * cur_col.w;
        result.z += omda * cur_col.z * cur_col.w;
        result.w += omda * cur_col.w;
      }

      // assign result color
      output_color = result;
    }
    // local shading
    if (output_color.w > m_depth_enhancement_params->shade_h)
    {
      // get gradient normal
      const float3 vs_grad = volume.get_gradient(sample_position); // compute R3 gradient
      const float3 iso_normal = -normalize(vs_grad);               // get isosurface normal
      // const float vs_grad_mag = length(vs_grad);                            // get gradient
      // magnitude

      // set up lighting parameters
      // const float3 light_dir = normalize(light_pos - wp);
      const float3 view_dir = state.m_ray_direction;

      const float3 diffuse_color = make_float3(output_color);

      const float diff_amnt = fabsf(dot(light_dir, iso_normal)); // two sided shading
      float spec_amnt = 0.0f;

      if (diff_amnt > 0.0f)
      {
        // this is blinn phong
        const float3 H = normalize(light_dir + view_dir);
        const float NH = fabsf(dot(H, iso_normal)); // two sided shading
        spec_amnt = powf(NH, m_depth_enhancement_params->shininess);
      }

      // compute final color (RGB)
      const float3 color_linear =
        diffuse_color * (m_depth_enhancement_params->amb_fac + diff_amnt) +
        m_depth_enhancement_params->spec_color * (spec_amnt * m_depth_enhancement_params->spec_fac);

      // apply gamma correction (assume ambient_color, diffuse_color and spec_color
      // have been linearized, i.e. have no gamma correction in them)
      output_color.x = powf(color_linear.x, float(1.0f / m_depth_enhancement_params->screen_gamma));
      output_color.y = powf(color_linear.y, float(1.0f / m_depth_enhancement_params->screen_gamma));
      output_color.z = powf(color_linear.z, float(1.0f / m_depth_enhancement_params->screen_gamma));

      // apply build in gamma function
      // output_color = gamma_correct(output_color, m_depth_enhancement_params->screen_gamma);

      // clamp result color
      output_color = clamp(output_color, 0.0f, 1.0f);
    }

    return NV_IDX_PROG_OK;
  }
}; // class NV_IDX_volume_sample_program)";

const char* KERNEL_EDGE_ENHANCEMENT_STRING = R
  "(/******************************************************************************
  *
  Copyright 2018 NVIDIA Corporation.All rights reserved.*
    **************************************************************************** /

  // # RTC Kernel:
  // ** Volume Edge Enhancement **

  // # Summary:
  // Compute the colormap alpha gradient in the current volume and darken samples with high gradient
  // magnitude a a user-defined rate.

  // Define the user-defined data structure
  struct Edge_enhancement_params
{
  float sample_range; // [10, 10] 1.0 sample range
  float rh;           // 1.0f ray sampling difference
  int stp_num;        // 6 [GUI] additional samples along ray
  float min_alpha;    // 0.2f minimum alpha for sampling (improves performance)
};

NV_IDX_VOLUME_SAMPLE_PROGRAM_PREFIX
class NV_IDX_volume_sample_program
{
  NV_IDX_VOLUME_SAMPLE_PROGRAM

public:
  const Edge_enhancement_params*
    m_edge_enhancement_params; // define variables to bind user-defined buffer to

public:
  NV_IDX_DEVICE_INLINE_MEMBER
  void init_instance()
  {
    // Bind the contents of the buffer slot 0 to the variable
    m_edge_enhancement_params = NV_IDX_bind_parameter_buffer<Edge_enhancement_params>(0);
  }

  NV_IDX_DEVICE_INLINE_MEMBER
  int execute(const NV_IDX_sample_info_self& sample_info, float4& output_color)
  {
    const NV_IDX_volume& volume = state.self;
    const float3& ps = sample_info.sample_position;
    const NV_IDX_colormap& colormap = volume.get_colormap();

    // sample volume
    const float3 p0 = ps - (state.m_ray_direction * (m_edge_enhancement_params->rh *
                                                      m_edge_enhancement_params->sample_range));
    const float3 p2 = ps + (state.m_ray_direction * (m_edge_enhancement_params->rh *
                                                      m_edge_enhancement_params->sample_range));

    const float vs_p0 = volume.sample<float>(p0);
    const float vs_ps = volume.sample<float>(ps);
    const float vs_p2 = volume.sample<float>(p2);

    const float4 ps_color = colormap.lookup(vs_ps);
    const float ps_alpha = ps_color.w;
    const int min_hits = m_edge_enhancement_params->stp_num;
    const int num_steps = min_hits * 2;

    if (ps_alpha < m_edge_enhancement_params->min_alpha)
    {
      // sample below given alpha threshold
      // return NV_IDX_PROG_DISCARD_SAMPLE;
    }

    if ((num_steps > 0) && (ps_alpha > m_edge_enhancement_params->min_alpha))
    {
      // init result color
      int sum_over_b = 0;
      int sum_under_b = 0;
      int sum_over_a = 0;
      int sum_under_a = 0;

      // iterate steps
      for (int sc = 0; sc <= num_steps; sc++)
      {
        // get step size
        const float rt = float(sc) / float(num_steps);
        const float3 pi_b = (1.0f - rt) * p0 + rt * ps;
        const float3 pi_a = (1.0f - rt) * ps + rt * p2;

        const float vs_pi_b = volume.sample<float>(pi_b);
        const float vs_pi_a = volume.sample<float>(pi_a);

        const float4 pia_color = colormap.lookup(vs_pi_b);
        const float4 pib_color = colormap.lookup(vs_pi_a);

        // check by alpha
        if (pib_color.w > ps_alpha)
          sum_over_b += 1;
        if (pib_color.w < ps_alpha)
          sum_under_b += 1;
        if (pia_color.w > ps_alpha)
          sum_over_a += 1;
        if (pia_color.w < ps_alpha)
          sum_under_a += 1;
      }

      // check if all are larger
      // if (sum_over > 1 && sum_under == 0)
      //    output_color = make_float4(ps_color.x, ps_color.y, ps_color.z, ps_color.w);

      // check numer of samples below / above reference
      if (((sum_under_b > min_hits) && (sum_under_a > min_hits)) ||
        ((sum_over_b > min_hits) && (sum_over_a > min_hits)))
      {
        output_color = ps_color / float(sum_under_b + sum_under_a - 2 * min_hits + 1);
        output_color.w = ps_color.w;

        return NV_IDX_PROG_OK;
      }
      else
      {
        output_color = ps_color;

        // return NV_IDX_PROG_DISCARD_SAMPLE;
      }
    }
    else
    {
      output_color = ps_color;
    }

    return NV_IDX_PROG_OK;
  }
}; // class NV_IDX_volume_sample_program)";

const char* KERNEL_SINGLE_SCATTERING_STRING = R
  "(/******************************************************************************
  *
  Copyright 2018 NVIDIA Corporation.All rights reserved.*
    **************************************************************************** /

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

NV_IDX_VOLUME_SAMPLE_PROGRAM_PREFIX
class NV_IDX_volume_sample_program
{
  NV_IDX_VOLUME_SAMPLE_PROGRAM

public:
  const Single_scattering_params*
    m_single_scattering_params; // define variables to bind user-defined buffer to

public:
  NV_IDX_DEVICE_INLINE_MEMBER
  void init_instance()
  {
    // Bind the contents of the buffer slot 0 to the variable
    m_single_scattering_params = NV_IDX_bind_parameter_buffer<Single_scattering_params>(0);
  }

  NV_IDX_DEVICE_INLINE_MEMBER
  int execute(const NV_IDX_sample_info_self& sample_info, float4& output_color)
  {
    // retrieve parameter buffer contents (fixed values in code definition)
    const NV_IDX_volume volume = state.scene.get_volume(m_single_scattering_params->volume_id);
    const float3& sample_position = sample_info.sample_position;
    const NV_IDX_colormap& colormap = state.self.get_colormap();

    // sample volume and colormap
    const float volume_sample = volume.sample<float>(sample_position);
    const float4 sample_color = colormap.lookup(volume_sample);

    if (sample_color.w < m_single_scattering_params->min_samp_alpha)
    {
      output_color = sample_color;
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
      output_color = blinn_shading(
        sample_position, sample_color, -normalize(light_dir), m_single_scattering_params->dh);
      output_color.w = sample_color.w;
    }
    else
    {
      output_color = sample_color;
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
        const float3 sub_pos = state.self.transform_scene_to_sample(sray_pos);

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
      output_color *= fmaxf(fminf(acc_shadow, 1.0f), 0.0f);
      output_color.w = sample_color.w;
    }

    return NV_IDX_PROG_OK;
  }

  NV_IDX_DEVICE_INLINE_MEMBER
  float4 blinn_shading(const float3& sample_position, const float4& sample_color,
    const float3& light_dir, const float dh)
  {
    // define parameters
    const float3 diff_color = make_float3(sample_color);
    const float3 spec_color = make_float3(1.0f); // specular color
    const float screen_gamma = 0.7f;             // gamma correction parameter

    // get R3 gradient vector
    const float3 vs_grad = state.self.get_gradient(sample_position, dh); // compute R3 gradient
    const float3 iso_normal = -normalize(vs_grad);                       // get isosurface normal

    // set up lighting parameters
    const float3 view_dir = state.m_ray_direction;

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
    return make_float4(max(min(color.x, 1.0f), 0.0f), max(min(color.y, 1.0f), 0.0f),
      max(min(color.z, 1.0f), 0.0f), max(min(color.w, 1.0f), 0.0f));
  }
}; // class NV_IDX_volume_sample_program)";

const char* KERNEL_ISORAYCAST_STRING = R
  "(/******************************************************************************
  *
  Copyright 2018 NVIDIA Corporation.All rights reserved.*
    **************************************************************************** /

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

NV_IDX_VOLUME_SAMPLE_PROGRAM_PREFIX
class NV_IDX_volume_sample_program
{
  NV_IDX_VOLUME_SAMPLE_PROGRAM

  const float3 ray_dir = state.m_ray_direction;

public:
  const Isoraycast_params* m_isoraycast_params; // define variables to bind user-defined buffer to

public:
  NV_IDX_DEVICE_INLINE_MEMBER
  void init_instance()
  {
    // Bind the contents of the buffer slot 0 to the variable
    m_isoraycast_params = NV_IDX_bind_parameter_buffer<Isoraycast_params>(0);
  }

  NV_IDX_DEVICE_INLINE_MEMBER
  int execute(const NV_IDX_sample_info_self& sample_info, float4& output_color)
  {
    const NV_IDX_volume& volume = state.self;
    const float3& sample_position = sample_info.sample_position;
    const NV_IDX_colormap& colormap = volume.get_colormap();

    const float volume_sample = volume.sample<float>(sample_position);

    // get spatial sample points for each dimensions
    const float rh = state.m_ray_stepsize_min; // ray sampling difference
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
        output_color = blinn_shader(sample_position, c1);
      }
      else
      {
        // use sample color
        output_color = c1;
        output_color.w = 1.0f;
      }

      return NV_IDX_PROG_OK;
    }
    else
    {
      // use sample color
      output_color = c1;
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
  float4 blinn_shader(const float3& sample_position, const float4& sample_color)
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
}; // class NV_IDX_volume_sample_program)";
