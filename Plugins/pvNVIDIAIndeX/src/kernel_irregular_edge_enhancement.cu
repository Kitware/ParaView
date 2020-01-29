/* Copyright 2020 NVIDIA Corporation. All rights reserved.
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
// ** Volume Edge Enhancement **

// # Summary:
// Compute the colormap alpha gradient in the current volume and darken samples with high gradient
// magnitude a a user-defined rate.

// Define the user-defined data structure
struct Edge_enhancement_params
{
  float rh;           // 1.0f ray sampling difference
  float sample_range; // [10, 10] 1.0 sample range
  int stp_num;        // 6 [GUI] additional samples along ray
};

using namespace nv::index::xac;
using namespace nv::index::xaclib;

class Volume_sample_program
{
  NV_IDX_VOLUME_SAMPLE_PROGRAM

  const uint field_index  = 0u;
  const float min_alpha   = 0.2f; //minimum alpha for sampling

public:
  const Edge_enhancement_params*
    m_edge_enhancement_params; // define variables to bind user-defined buffer to

public:
  NV_IDX_DEVICE_INLINE_MEMBER
  void initialize()
  {
    // Bind the contents of the buffer slot 0 to the variable
    m_edge_enhancement_params = state.bind_parameter_buffer<Edge_enhancement_params>(0);
  }

  NV_IDX_DEVICE_INLINE_MEMBER
  int execute(const Sample_info_self&   sample_info,
                    Sample_output&      sample_output)
  {

    const auto& cell_info = sample_info.sample_cell_info;
    const auto& volume = state.self;
    const float3& ps = sample_info.scene_position;
    const float3 ray_dir = sample_info.ray_direction;
    const Colormap& colormap = volume.get_colormap();

    // sample volume
    const float3 p0 = ps - (ray_dir * (m_edge_enhancement_params->rh * m_edge_enhancement_params->sample_range));
    const float3 p2 = ps + (ray_dir * (m_edge_enhancement_params->rh * m_edge_enhancement_params->sample_range));

    const float3 vs0 = ps - p0;
    const float3 vs2 = ps - p2;

    const float vs_p0 = volume.fetch_attribute_offset<float>(field_index, cell_info, vs0);
    const float vs_ps = volume.fetch_attribute<float>(field_index, cell_info);
    const float vs_p2 = volume.fetch_attribute_offset<float>(field_index, cell_info, vs2);

    const float4 ps_color = colormap.lookup(vs_ps);
    const float ps_alpha = ps_color.w;
    const int min_hits = m_edge_enhancement_params->stp_num;
    const int num_steps = min_hits * 2;

    if (ps_alpha <= min_alpha)
    {
      // sample below given alpha threshold
      return NV_IDX_PROG_DISCARD_SAMPLE;
    }

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

      const float3 vs_ia = pi_a - ps;
      const float3 vs_ib = pi_b - ps;

      const float vs_pi_b = volume.fetch_attribute_offset<float>(field_index, cell_info, vs_ib);
      const float vs_pi_a = volume.fetch_attribute_offset<float>(field_index, cell_info, vs_ia);

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
    //    sample_output.color = make_float4(ps_color.x, ps_color.y, ps_color.z, ps_color.w);

    // check numer of samples below / above reference
    if (((sum_under_b > min_hits) && (sum_under_a > min_hits)) ||
      ((sum_over_b > min_hits) && (sum_over_a > min_hits)))
    {
      sample_output.color = ps_color / float(sum_under_b + sum_under_a - 2 * min_hits + 1);
      sample_output.color.w = ps_color.w;

      return NV_IDX_PROG_OK;
    }
    else
    {
      sample_output.color = ps_color;

      // return NV_IDX_PROG_DISCARD_SAMPLE;
    }

    return NV_IDX_PROG_OK;
  }
}; // class Volume_sample_program
