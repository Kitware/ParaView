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

#ifndef vtknvindex_rtc_kernel_params_h
#define vtknvindex_rtc_kernel_params_h

// list of kernels
enum vtknvindex_rtc_kernels
{
  RTC_KERNELS_NONE = 0,
  RTC_KERNELS_ISOSURFACE,
  RTC_KERNELS_DEPTH_ENHANCEMENT,
  RTC_KERNELS_EDGE_ENHANCEMENT,
  RTC_KERNELS_GRADIENT,
  RTC_KERNELS_CUSTOM
};

// kernel parameter buffer
struct vtknvindex_rtc_params_buffer
{
  vtknvindex_rtc_kernels rtc_kernel;
  std::string kernel_program;
  const void* params_buffer;
  int buffer_size;

  vtknvindex_rtc_params_buffer()
    : rtc_kernel(RTC_KERNELS_NONE)
    , kernel_program("")
    , params_buffer(0)
    , buffer_size(0)
  {
  }
};

// Isosurface CUDA code parameters.
struct vtknvindex_isosurface_params
{
  // Common lighting parameter.
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

  float spec_color[3]; // make_float3(1.0f), specular color

  float dummy[2]; // for memory 16 bytes alignment

  vtknvindex_isosurface_params()
    : light_mode(0)
    , angle(0.0f)
    , elevation(0.0f)
    , iso_min(0.5f)
    , iso_max(0.5f)
    , fill_up(1)
    , use_shading(1)
    , min_alpha(0.05f)
    , spec_fac(1.0f)
    , shininess(50.0f)
    , amb_fac(0.2f)
    , diff_exp(2.0f)
    , show_grid(0)
    , ng_num(16)
    , ng_width(0.01f)
  {
    spec_color[0] = spec_color[1] = spec_color[2] = 1.0f;
  }
};

// Depth enhancement CUDA code parameters.
struct vtknvindex_depth_enhancement_params
{
  // Common lighting parameter.
  int light_mode;  // 0=headlight, 1=orbital
  float angle;     // 0.0 angle
  float elevation; // 0.0 elevation

  // additional depth samples
  int max_dsteps; // = 8 [GUI] number of additional samples
  float ash;      // 0.01f adaptive sampling threshold

  // screen effects
  float screen_gamma; // 0.9f [GUI] gamma correction parameter

  // shading parameters [GUI / scene]
  float spec_color[3]; // make_float3(1.0f) specular color
  float spec_fac;      // 0.2f specular factor (phong)
  float shininess;     // 50.0f shininess parameter (phong)
  float amb_fac;       // 0.4f ambient factor

  float shade_h;   // 0.5f [GUI] min alpha value for shading
  float min_alpha; // 0.001f min alpha for sampling (improves performance)

  float dummy[2];

  vtknvindex_depth_enhancement_params()
    : light_mode(0)
    , angle(0.0f)
    , elevation(0.0f)
    , max_dsteps(8)
    , ash(0.01f)
    , screen_gamma(0.9f)
    , spec_fac(0.2f)
    , shininess(50.0f)
    , amb_fac(0.4f)
    , shade_h(0.5f)
    , min_alpha(0.001f)
  {
    spec_color[0] = spec_color[1] = spec_color[2] = 1.0f;
  }
};

// Edge enhancement CUDA code parameters.
struct vtknvindex_edge_enhancement_params
{
  float sample_range; // [10, 10] 1.0 sample range
  float rh;           // 1.0f ray sampling difference
  int stp_num;        // 6 [GUI] additional samples along ray
  float min_alpha;    // 0.2f minimum alpha for sampling (improves performance)

  vtknvindex_edge_enhancement_params()
    : sample_range(1.0f)
    , rh(1.0f)
    , stp_num(6)
    , min_alpha(0.2f)
  {
  }
};

// Gradient CUDA code parameters.
struct vtknvindex_gradient_params
{
  float gradient; // [0.1, 1] gradient value
  float grad_max; // 20.0f maximum gradient scale
  float dummy1;
  float dummy2;

  vtknvindex_gradient_params()
    : gradient(0.5f)
    , grad_max(20.0f)
  {
  }
};

// Custom CUDA code parameters.
struct vtknvindex_custom_params
{
  float floats[4];
  int ints[4];

  vtknvindex_custom_params() {}
};

// IVOL isosurface CUDA code parameters.
struct vtknvindex_ivol_isosurface_params
{
  float rh; // raycast step size
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

  float spec_color[3]; // make_float3(1.0f), specular color

  float dummy[2]; // for memory 16 bytes alignment

  vtknvindex_ivol_isosurface_params()
    : rh(1.0f)
    , light_mode(0)
    , angle(0.0f)
    , elevation(0.0f)
    , iso_min(0.5f)
    , iso_max(0.5f)
    , fill_up(1)
    , use_shading(1)
    , spec_fac(1.0f)
    , shininess(50.0f)
    , amb_fac(0.2f)
  {
    spec_color[0] = spec_color[1] = spec_color[2] = 1.0f;
  }
};

// IVOL depth enhancement CUDA code parameters.
struct vtknvindex_ivol_depth_enhancement_params
{
  float rh; // raycast step size
  // Common lighting parameter.
  int light_mode;  // 0=headlight, 1=orbital
  float angle;     // 0.0 angle
  float elevation; // 0.0 elevation

  // additional depth samples
  int max_dsteps; // = 8 [GUI] number of additional samples

  // screen effects
  float screen_gamma; // 0.9f [GUI] gamma correction parameter

  // shading parameters [GUI / scene]
  float spec_color[3]; // make_float3(1.0f) specular color
  float spec_fac;      // 0.2f specular factor (phong)
  float shininess;     // 50.0f shininess parameter (phong)
  float amb_fac;       // 0.4f ambient factor

  vtknvindex_ivol_depth_enhancement_params()
    : rh(0.0f)
    , light_mode(0)
    , angle(0.0f)
    , elevation(0.0f)
    , max_dsteps(8)
    , screen_gamma(0.9f)
    , spec_fac(0.2f)
    , shininess(50.0f)
    , amb_fac(0.4f)
  {
    spec_color[0] = spec_color[1] = spec_color[2] = 1.0f;
  }
};

// IVOL edge enhancement CUDA code parameters.
struct vtknvindex_ivol_edge_enhancement_params
{
  float rh;           // 1.0f ray sampling difference
  float sample_range; // [10, 10] 1.0 sample range
  int stp_num;        // 6 [GUI] additional samples along ray
  float dummy;

  vtknvindex_ivol_edge_enhancement_params()
    : rh(1.0f)
    , sample_range(1.0f)
    , stp_num(6)
  {
  }
};

// IVOL custom CUDA code parameters.
struct vtknvindex_ivol_custom_params
{
  float floats[4];
  int ints[4];

  vtknvindex_ivol_custom_params() {}
};

// rtc sparse volume programs
extern const char* KERNEL_ISOSURFACE_STRING;
extern const char* KERNEL_DEPTH_ENHANCEMENT_STRING;
extern const char* KERNEL_EDGE_ENHANCEMENT_STRING;
extern const char* KERNEL_GRADIENT_STRING;

// rtc irregular volume programs
extern const char* KERNEL_IRREGULAR_ISOSURFACE_STRING;
extern const char* KERNEL_IRREGULAR_DEPTH_ENHANCEMENT_STRING;
extern const char* KERNEL_IRREGULAR_EDGE_ENHANCEMENT_STRING;

// rtc surface programs
extern const char* KERNEL_PLANE_SURFACE_MAPPING_STRING;

#endif // vtknvindex_rtc_kernel_params_h
