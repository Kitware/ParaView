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

#include <cassert>

#include "vtksys/SystemInformation.hxx"

#include "vtknvindex_scene.h"

#include <nv/index/idistributed_compute_algorithm.h>
#include <nv/index/idistributed_data_import_callback.h>
#include <nv/index/iirregular_volume_rendering_properties.h>
#include <nv/index/ilight.h>
#include <nv/index/imaterial.h>
#include <nv/index/iplane.h>
#include <nv/index/iregular_volume_rendering_properties.h>
#include <nv/index/iregular_volume_texture.h>
#include <nv/index/irendering_kernel_programs.h>
#include <nv/index/iscene.h>

#include <nv/index/isparse_volume_rendering_properties.h>
#include <nv/index/ivolume_filter_mode.h>

#include "vtkCamera.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include "vtknvindex_clock_pulse_generator.h"
#include "vtknvindex_cluster_properties.h"
#include "vtknvindex_forwarding_logger.h"
#include "vtknvindex_instance.h"
#include "vtknvindex_irregular_volume_importer.h"
#include "vtknvindex_rtc_kernel_params.h"
#include "vtknvindex_sparse_volume_importer.h"
#include "vtknvindex_volume_compute.h"

//-------------------------------------------------------------------------------------------------
bool file_to_string(const std::string& file_path, std::string& out_file_string)
{
  std::ifstream src_file;

  src_file.open(file_path.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

  if (!src_file.is_open())
    return false;

  const std::ifstream::pos_type src_file_size = src_file.tellg();
  src_file.seekg(std::ios::beg);

  out_file_string.resize(src_file_size, '\0');
  src_file.read(&out_file_string[0], src_file_size);

  if (!src_file)
  {
    out_file_string.clear();
    return false;
  }

  src_file.close();

  return true;
}

//-------------------------------------------------------------------------------------------------
void update_slice_planes(nv::index::IPlane* plane, const vtknvindex_slice_params& slice_params,
  const mi::math::Vector_struct<mi::Uint32, 3>& volume_size)
{
  mi::math::Vector_struct<mi::Uint32, 3> volume_size_ed = volume_size;
  volume_size_ed.x -= 1;
  volume_size_ed.y -= 1;
  volume_size_ed.z -= 1;

  plane->set_enabled(slice_params.enable);

  const mi::Float32 displace = 0.5f * slice_params.displace *
    mi::math::max(volume_size_ed.x, mi::math::max(volume_size_ed.y, volume_size_ed.z));

  mi::math::Vector<mi::Float32, 3> pos, normal, up;
  mi::math::Vector<mi::Float32, 2> extent;
  switch (slice_params.mode)
  {
    default:
    case vtknvindex_slice_params::X_NORMAL:
      pos = mi::math::Vector<mi::Float32, 3>(volume_size_ed.x / 2.f, 0.f, 0.f);
      normal = mi::math::Vector<mi::Float32, 3>(1.f, 0.f, 0.f);
      up = mi::math::Vector<mi::Float32, 3>(0.f, 0.f, 1.f);
      extent = mi::math::Vector<mi::Float32, 2>(volume_size_ed.y, volume_size_ed.z);
      break;
    case vtknvindex_slice_params::Y_NORMAL:
      pos = mi::math::Vector<mi::Float32, 3>(volume_size_ed.x, volume_size_ed.y / 2.f, 0.f);
      normal = mi::math::Vector<mi::Float32, 3>(0.f, 1.f, 0.f);
      up = mi::math::Vector<mi::Float32, 3>(0.f, 0.f, 1.f);
      extent = mi::math::Vector<mi::Float32, 2>(volume_size_ed.x, volume_size_ed.z);
      break;
    case vtknvindex_slice_params::Z_NORMAL:
      pos = mi::math::Vector<mi::Float32, 3>(volume_size_ed.x, 0.f, volume_size_ed.z / 2.f);
      normal = mi::math::Vector<mi::Float32, 3>(0.f, 0.f, 1.f);
      up = mi::math::Vector<mi::Float32, 3>(-1.f, 0.f, 0.f);
      extent = mi::math::Vector<mi::Float32, 2>(volume_size_ed.y, volume_size.x);
      break;
  }

  plane->set_normal(normal);
  plane->set_up(up);
  plane->set_point(pos + normal * displace);
  plane->set_extent(extent);
}

vtknvindex_scene::vtknvindex_scene()
  : m_scene_created(false)
  , m_only_init(true)
  , m_is_parallel(false)
  , m_vol_properties_tag(mi::neuraylib::NULL_TAG)
  , m_timeseries_tag(mi::neuraylib::NULL_TAG)
  , m_root_group_tag(mi::neuraylib::NULL_TAG)
  , m_static_group_tag(mi::neuraylib::NULL_TAG)
  , m_volume_tag(mi::neuraylib::NULL_TAG)
  , m_prog_se_mapping_tag(mi::neuraylib::NULL_TAG)
  , m_rtc_program_params_tag(mi::neuraylib::NULL_TAG)
  , m_rtc_program_tag(mi::neuraylib::NULL_TAG)
  , m_volume_compute_attrib_tag(mi::neuraylib::NULL_TAG)
  , m_sparse_volume_render_properties_tag(mi::neuraylib::NULL_TAG)
{
  m_cluster_properties = NULL;
  m_index_instance = vtknvindex_instance::get();
}

vtknvindex_scene::~vtknvindex_scene()
{
  // Remove this volume instance and complementary elements from scene graph
  if (m_index_instance->is_index_viewer() && m_root_group_tag != mi::neuraylib::NULL_TAG)
  {
    mi::base::Handle<mi::neuraylib::IDice_transaction> dice_transaction(
      m_index_instance->m_global_scope->create_transaction<mi::neuraylib::IDice_transaction>());
    assert(dice_transaction.is_valid_interface());

    {
      mi::base::Handle<nv::index::IStatic_scene_group> scene_geom_group_edit(
        dice_transaction->edit<nv::index::IStatic_scene_group>(
          m_index_instance->get_scene_geom_group()));

      assert(scene_geom_group_edit.is_valid_interface());

      scene_geom_group_edit->remove(m_root_group_tag, dice_transaction.get());

      // ERROR_LOG << "Removing tag: " << m_root_group_tag.id
      //          << " from Geom group: " << m_index_instance->get_scene_geom_group();
    }

    dice_transaction->commit();
  }

  m_vol_properties_tag = mi::neuraylib::NULL_TAG;
  m_timeseries_tag = mi::neuraylib::NULL_TAG;
  m_root_group_tag = mi::neuraylib::NULL_TAG;
  m_volume_tag = mi::neuraylib::NULL_TAG;
  m_prog_se_mapping_tag = mi::neuraylib::NULL_TAG;
  m_rtc_program_params_tag = mi::neuraylib::NULL_TAG;
  m_rtc_program_tag = mi::neuraylib::NULL_TAG;
  m_volume_compute_attrib_tag = mi::neuraylib::NULL_TAG;
  m_sparse_volume_render_properties_tag = mi::neuraylib::NULL_TAG;
  m_static_group_tag = mi::neuraylib::NULL_TAG;

  for (mi::Uint32 i = 0; i < m_plane_tags.size(); i++)
    m_plane_tags[i] = mi::neuraylib::NULL_TAG;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_scene::set_visibility(bool visibility)
{
  if (m_root_group_tag != mi::neuraylib::NULL_TAG)
  {
    mi::base::Handle<mi::neuraylib::IDice_transaction> dice_transaction(
      m_index_instance->m_global_scope->create_transaction<mi::neuraylib::IDice_transaction>());
    assert(dice_transaction.is_valid_interface());

    {
      mi::base::Handle<nv::index::IStatic_scene_group> root_group_edit(
        dice_transaction->edit<nv::index::IStatic_scene_group>(m_root_group_tag));
      assert(root_group_edit.is_valid_interface());

      root_group_edit->set_enabled(visibility);

      // Update scene transformation
      if (visibility)
      {
        // Access the session instance from the database.
        mi::base::Handle<const nv::index::ISession> session(
          dice_transaction->access<const nv::index::ISession>(m_index_instance->m_session_tag));
        assert(session.is_valid_interface());

        // Access (edit mode) the scene instance from the database.
        mi::base::Handle<nv::index::IScene> scene(
          dice_transaction->edit<nv::index::IScene>(session->get_scene()));
        assert(scene.is_valid_interface());

        mi::base::Handle<const mi::neuraylib::IElement> vol_elem(
          dice_transaction->access<mi::neuraylib::IElement>(m_volume_tag));

        mi::base::Handle<const nv::index::ISparse_volume_scene_element> sparse_vol_elem(
          vol_elem->get_interface<nv::index::ISparse_volume_scene_element>());

        // Set global scene transform (only required for regular volumes).
        mi::math::Matrix<mi::Float32, 4, 4> scene_rotation_matrix(1.0f);
        mi::math::Vector_struct<mi::Float32, 3> scene_translate_vec;
        mi::math::Vector_struct<mi::Float32, 3> scene_scaling_vec;

        // Is regular volume?
        if (sparse_vol_elem.is_valid_interface())
        {
          vtknvindex_regular_volume_properties* regular_volume_properties =
            m_cluster_properties->get_regular_volume_properties();

          mi::math::Vector<mi::Float32, 3> translation, scaling;
          regular_volume_properties->get_volume_translation(translation);
          regular_volume_properties->get_volume_scaling(scaling);

          // Need to translate if the volume doesn't start at [0,0,0].
          scene_translate_vec.x = translation.x;
          scene_translate_vec.y = translation.y;
          scene_translate_vec.z = translation.z;

          scene_scaling_vec.x = scaling.x;
          scene_scaling_vec.y = scaling.y;
          scene_scaling_vec.z = scaling.z;
        }
        else
        {
          scene_translate_vec.x = scene_translate_vec.y = scene_translate_vec.z = 0.0f;
          scene_scaling_vec.x = scene_scaling_vec.y = scene_scaling_vec.z = 1.0f;
        }
        scene->set_transform_matrix(scene_translate_vec, scene_rotation_matrix, scene_scaling_vec);
      }
    }

    dice_transaction->commit();
  }
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_scene::create_scene(vtkRenderer* ren, vtkVolume* vol,
  const mi::base::Handle<mi::neuraylib::IDice_transaction>& dice_transaction,
  Volume_type volume_type)

{
  if (m_scene_created)
    return;

  // Set the affinity information.
  mi::base::Handle<vtknvindex_affinity> affinity = m_cluster_properties->get_affinity();

  std::ostringstream s;
  affinity->scene_dump_affinity_info(s);
  INFO_LOG << s.str();

  // NVIDIA IndeX session will take ownership of the affinity.
  affinity->retain();

  m_index_instance->m_iindex_session->set_affinity_information(affinity.get());

  // // DiCE database access.
  // mi::base::Handle<mi::neuraylib::IDice_transaction> dice_transaction(
  //  m_index_instance.m_global_scope->create_transaction<mi::neuraylib::IDice_transaction>());
  // assert(dice_transaction.is_valid_interface());

  // GUI settings.
  vtknvindex_config_settings* pv_config_settings = m_cluster_properties->get_config_settings();

  // Create and setup the scene.
  {
    // Access the session instance from the database.
    mi::base::Handle<const nv::index::ISession> session(
      dice_transaction->access<const nv::index::ISession>(m_index_instance->m_session_tag));
    assert(session.is_valid_interface());

    // Access (edit mode) the scene instance from the database.
    mi::base::Handle<nv::index::IScene> scene(
      dice_transaction->edit<nv::index::IScene>(session->get_scene()));
    assert(scene.is_valid_interface());

    // Dataset subcube border size.
    mi::Sint32 subcube_border = m_cluster_properties->get_config_settings()->get_subcube_border();

    // Print cluster details in ParaView's console.
    m_cluster_properties->print_info();

    vtknvindex_regular_volume_properties* regular_volume_properties =
      m_cluster_properties->get_regular_volume_properties();

    std::string scalar_type;
    regular_volume_properties->get_scalar_type(scalar_type);

    // Scene contains a regular volume.
    if (volume_type == VOLUME_TYPE_REGULAR)
    {
      mi::math::Vector_struct<mi::Uint32, 3> volume_size;
      regular_volume_properties->get_volume_size(volume_size);

      // Create shared memory regular volume data importer.
      vtknvindex_sparse_volume_importer* volume_importer =
        new vtknvindex_sparse_volume_importer(volume_size, subcube_border, scalar_type);

      assert(volume_importer);

      volume_importer->set_cluster_properties(m_cluster_properties);
      mi::math::Vector<mi::Float32, 3> translation, scaling;
      regular_volume_properties->get_volume_translation(translation);
      regular_volume_properties->get_volume_scaling(scaling);

      // Create the sparse volume scene element
      mi::math::Bbox<mi::Float32, 3> svol_bbox = mi::math::Bbox<mi::Float32, 3>(
        mi::math::Vector<mi::Float32, 3>(0.f), mi::math::Vector<mi::Float32, 3>(volume_size));
      mi::math::Matrix<mi::Float32, 4, 4> transform_mat(1.0f);

      mi::base::Handle<nv::index::ISparse_volume_scene_element> volume_scene_element(
        scene->create_sparse_volume(
          svol_bbox, transform_mat, volume_importer, dice_transaction.get()));

      assert(volume_scene_element.is_valid_interface());

      if (volume_scene_element)
      {
        volume_scene_element->set_enabled(pv_config_settings->get_enable_volume());

        // Store the volume scene element in the distributed database.
        m_volume_tag = dice_transaction->store_for_reference_counting(volume_scene_element.get());
        assert(m_volume_tag.is_valid());
      }
      else
      {
        ERROR_LOG << "Unable to create a regular volume scene element.";
      }

      // Load and create colormap from ParaView
      m_index_instance->get_index_colormaps()->update_scene_colormaps(
        vol, dice_transaction, regular_volume_properties);
    }
    else // scene contains an irregular volume (unstructured volume grid)
    {
      mi::math::Bbox<mi::Float32, 3> ivol_bbox;
      regular_volume_properties->get_ivol_volume_extents(ivol_bbox);

      // Create shared memory irregular volume data importer.
      vtknvindex_irregular_volume_importer* volume_importer =
        new vtknvindex_irregular_volume_importer(subcube_border, scalar_type);

      assert(volume_importer);

      // Create a scene element that represents an irregular volume.
      volume_importer->set_cluster_properties(m_cluster_properties);
      mi::base::Handle<nv::index::IIrregular_volume_scene_element> volume_scene_element(
        scene->create_irregular_volume(ivol_bbox, -1.0f, volume_importer, dice_transaction.get()));
      assert(volume_scene_element.is_valid_interface());

      if (volume_scene_element)
      {
        volume_scene_element->set_enabled(true);

        // Store the volume scene element in the distributed database
        m_volume_tag = dice_transaction->store_for_reference_counting(volume_scene_element.get());
        assert(m_volume_tag.is_valid());
      }
      else
      {
        ERROR_LOG << "Unable to create an irregular volume scene element.";
      }

      // Load and create colormap from ParaView
      m_index_instance->get_index_colormaps()->update_scene_colormaps(
        vol, dice_transaction, regular_volume_properties);
    }

    // Setup animation settings for datasets with time series
    if (volume_type == VOLUME_TYPE_REGULAR &&
      m_cluster_properties->get_regular_volume_properties()->is_timeseries_data())
    {
      mi::base::Handle<nv::index::ITime_step_assignment> timeseries_assignment(
        scene->create_attribute<nv::index::ITime_step_assignment>());

      if (timeseries_assignment)
      {
        mi::Uint32 nb_time_steps =
          m_cluster_properties->get_regular_volume_properties()->get_nb_time_steps();
        timeseries_assignment->set_nb_time_steps(nb_time_steps);
        timeseries_assignment->set_enabled(true);

        m_timeseries_tag =
          dice_transaction->store_for_reference_counting(timeseries_assignment.get());
        assert(m_timeseries_tag.is_valid());

        vtknvindex_clock_pulse_generator* app_clock_pulse =
          new vtknvindex_clock_pulse_generator(0.0, nb_time_steps);
        m_index_instance->m_iindex_session->set_clock(app_clock_pulse);
      }
    }

    // Create root group node for volume and slices
    mi::base::Handle<nv::index::IStatic_scene_group> root_group(
      scene->create_scene_group<nv::index::IStatic_scene_group>());
    assert(root_group.is_valid_interface());

    // Create static group node for large data and append the volume.
    mi::base::Handle<nv::index::IStatic_scene_group> static_group(
      scene->create_scene_group<nv::index::IStatic_scene_group>());
    assert(static_group.is_valid_interface());

    // Create transform group node for slices.
    mi::base::Handle<nv::index::ITransformed_scene_group> transform_group(
      scene->create_scene_group<nv::index::ITransformed_scene_group>());
    assert(transform_group.is_valid_interface());

    if (m_volume_tag)
    {
      // Create CUDA code volume properties.
      mi::base::Handle<nv::index::IRendering_kernel_program_parameters> rtc_program_parameters(
        scene->create_attribute<nv::index::IRendering_kernel_program_parameters>());
      assert(rtc_program_parameters.is_valid_interface());

      mi::base::Handle<nv::index::IRendering_kernel_program> rtc_program(
        scene->create_attribute<nv::index::IVolume_sample_program>());
      assert(rtc_program.is_valid_interface());

      if (volume_type == VOLUME_TYPE_REGULAR)
      {
        switch (m_volume_rtc_kernel.rtc_kernel)
        {
          case RTC_KERNELS_ISOSURFACE:
          {
            vtknvindex_isosurface_params iso_params;
            rtc_program->set_program_source(KERNEL_ISOSURFACE_STRING);
            rtc_program_parameters->set_buffer_data(
              0, reinterpret_cast<void*>(&iso_params), sizeof(iso_params));
            break;
          }

          case RTC_KERNELS_DEPTH_ENHANCEMENT:
          {
            vtknvindex_depth_enhancement_params depth_params;
            rtc_program->set_program_source(KERNEL_DEPTH_ENHANCEMENT_STRING);
            rtc_program_parameters->set_buffer_data(
              0, reinterpret_cast<void*>(&depth_params), sizeof(depth_params));
            break;
          }

          case RTC_KERNELS_EDGE_ENHANCEMENT:
          {
            vtknvindex_edge_enhancement_params edge_params;
            rtc_program->set_program_source(KERNEL_EDGE_ENHANCEMENT_STRING);
            rtc_program_parameters->set_buffer_data(
              0, reinterpret_cast<void*>(&edge_params), sizeof(edge_params));
            break;
          }

          case RTC_KERNELS_GRADIENT:
          {
            vtknvindex_gradient_params gradient_params;
            rtc_program->set_program_source(KERNEL_GRADIENT_STRING);
            rtc_program_parameters->set_buffer_data(
              0, reinterpret_cast<void*>(&gradient_params), sizeof(gradient_params));
            break;
          }

          default:
            rtc_program->set_enabled(false);
            rtc_program_parameters->set_enabled(false);
            break;
        }
      }
      else // kernels for irregular volumes
      {
        switch (m_volume_rtc_kernel.rtc_kernel)
        {
          case RTC_KERNELS_ISOSURFACE:
          {
            vtknvindex_ivol_isosurface_params iso_params;
            iso_params.rh = m_cluster_properties->get_config_settings()->get_ivol_step_size();
            rtc_program->set_program_source(KERNEL_IRREGULAR_ISOSURFACE_STRING);
            rtc_program_parameters->set_buffer_data(
              0, reinterpret_cast<void*>(&iso_params), sizeof(iso_params));
            break;
          }

          case RTC_KERNELS_DEPTH_ENHANCEMENT:
          {
            vtknvindex_ivol_depth_enhancement_params depth_params;
            depth_params.rh = m_cluster_properties->get_config_settings()->get_ivol_step_size();
            rtc_program->set_program_source(KERNEL_IRREGULAR_DEPTH_ENHANCEMENT_STRING);
            rtc_program_parameters->set_buffer_data(
              0, reinterpret_cast<void*>(&depth_params), sizeof(depth_params));
            break;
          }

          case RTC_KERNELS_EDGE_ENHANCEMENT:
          {
            vtknvindex_ivol_edge_enhancement_params edge_params;
            edge_params.rh = m_cluster_properties->get_config_settings()->get_ivol_step_size();
            rtc_program->set_program_source(KERNEL_IRREGULAR_EDGE_ENHANCEMENT_STRING);
            rtc_program_parameters->set_buffer_data(
              0, reinterpret_cast<void*>(&edge_params), sizeof(edge_params));
            break;
          }

          default:
            rtc_program->set_enabled(false);
            rtc_program_parameters->set_enabled(false);
            break;
        }
      }

      m_rtc_program_params_tag =
        dice_transaction->store_for_reference_counting(rtc_program_parameters.get());
      assert(m_rtc_program_params_tag.is_valid());

      m_rtc_program_tag = dice_transaction->store_for_reference_counting(rtc_program.get());
      assert(m_rtc_program_tag.is_valid());

      // Create scene volume properties attribute.
      if (volume_type == VOLUME_TYPE_REGULAR)
      {
        mi::base::Handle<nv::index::IRegular_volume_rendering_properties> vol_render_properties(
          scene->create_attribute<nv::index::IRegular_volume_rendering_properties>());
        assert(vol_render_properties.is_valid_interface());

        vol_render_properties->set_reference_step_size(calculate_volume_reference_step_size(vol,
          pv_config_settings->get_opacity_mode(), pv_config_settings->get_opacity_reference()));

        m_vol_properties_tag =
          dice_transaction->store_for_reference_counting(vol_render_properties.get());
      }
      else // irregular volume properties
      {
        mi::base::Handle<nv::index::IIrregular_volume_rendering_properties> vol_render_properties(
          scene->create_attribute<nv::index::IIrregular_volume_rendering_properties>());
        assert(vol_render_properties.is_valid_interface());

        nv::index::IIrregular_volume_rendering_properties::Rendering render_properties;
        vol_render_properties->get_rendering(render_properties);

        render_properties.sampling_mode =
          (m_volume_rtc_kernel.rtc_kernel == RTC_KERNELS_NONE) ? 0 : 1;

        render_properties.sampling_segment_length =
          m_cluster_properties->get_config_settings()->get_ivol_step_size() *
          m_cluster_properties->get_config_settings()->get_step_size();

        render_properties.sampling_reference_segment_length = calculate_volume_reference_step_size(
          vol, pv_config_settings->get_opacity_mode(), pv_config_settings->get_opacity_reference());

        vol_render_properties->set_rendering(render_properties);

        m_vol_properties_tag =
          dice_transaction->store_for_reference_counting(vol_render_properties.get());
      }
      assert(m_vol_properties_tag.is_valid());

      if (volume_type == VOLUME_TYPE_REGULAR)
      {
        // Volume compute attribute
        mi::math::Vector_struct<mi::Uint32, 3> volume_size;
        regular_volume_properties->get_volume_size(volume_size);

        mi::base::Handle<vtknvindex_volume_compute> vol_compute(new vtknvindex_volume_compute(
          volume_size, subcube_border, scalar_type, m_cluster_properties));
        assert(vol_compute.is_valid_interface());

        vol_compute->set_enabled(false);

        m_volume_compute_attrib_tag =
          dice_transaction->store_for_reference_counting(vol_compute.get());
        assert(m_volume_compute_attrib_tag.is_valid());
      }

      // Filter mode attribute
      mi::base::Handle<nv::index::ISparse_volume_rendering_properties>
        sparse_volume_render_properties(
          scene->create_attribute<nv::index::ISparse_volume_rendering_properties>());

      assert(sparse_volume_render_properties.is_valid_interface());

      // filter mode
      const nv::index::Sparse_volume_filter_mode filter_mode =
        static_cast<nv::index::Sparse_volume_filter_mode>(pv_config_settings->get_filter_mode());

      sparse_volume_render_properties->set_filter_mode(filter_mode);

      // use pre-integration
      const vtknvindex_rtc_kernels rtc_kernel = pv_config_settings->get_rtc_kernel();
      bool use_preintegration =
        rtc_kernel == RTC_KERNELS_NONE && pv_config_settings->is_preintegration();
      sparse_volume_render_properties->set_preintegrated_volume_rendering(use_preintegration);

      // sampling distance
      sparse_volume_render_properties->set_sampling_distance(pv_config_settings->get_step_size());

      // set reference sampling distance
      sparse_volume_render_properties->set_reference_sampling_distance(
        calculate_volume_reference_step_size(vol, pv_config_settings->get_opacity_mode(),
          pv_config_settings->get_opacity_reference()));

      // sparse volume render properties tag
      m_sparse_volume_render_properties_tag =
        dice_transaction->store_for_reference_counting(sparse_volume_render_properties.get());
      assert(m_sparse_volume_render_properties_tag.is_valid());

      static_group->append(m_sparse_volume_render_properties_tag, dice_transaction.get());

      // Volume properties.
      if (volume_type == VOLUME_TYPE_REGULAR)
      {
        // Append the static scene group to the hierarchical scene description.
        if (m_cluster_properties->get_regular_volume_properties()->is_timeseries_data())
          static_group->append(m_timeseries_tag, dice_transaction.get());

        static_group->append(m_volume_compute_attrib_tag, dice_transaction.get());
      }

      static_group->append(m_rtc_program_params_tag, dice_transaction.get());
      static_group->append(m_rtc_program_tag, dice_transaction.get());
      static_group->append(m_vol_properties_tag, dice_transaction.get());
      static_group->append(m_volume_tag, dice_transaction.get());

      // Slice properties, only for regular volumes.
      if (volume_type == VOLUME_TYPE_REGULAR)
      {
        // Plane surface shader
        mi::base::Handle<nv::index::IRendering_kernel_program> rtc_surface_program(
          scene->create_attribute<nv::index::ISurface_sample_program>());
        assert(rtc_surface_program.is_valid_interface());

        rtc_surface_program->set_program_source(KERNEL_PLANE_SURFACE_MAPPING_STRING);

        const mi::neuraylib::Tag rtc_surface_program_tag =
          dice_transaction->store_for_reference_counting(rtc_surface_program.get());
        assert(rtc_surface_program_tag.is_valid());

        // RTC program scene element mapping
        mi::base::Handle<nv::index::IRendering_kernel_program_scene_element_mapping>
          prog_se_mapping(
            scene->create_attribute<nv::index::IRendering_kernel_program_scene_element_mapping>());
        assert(prog_se_mapping.is_valid_interface());

        // set volume mapping
        prog_se_mapping->set_mapping(1, m_volume_tag);
        // set colormap mapping
        prog_se_mapping->set_mapping(2, m_index_instance->get_slice_colormap());

        m_prog_se_mapping_tag =
          dice_transaction->store_for_reference_counting(prog_se_mapping.get());
        assert(m_prog_se_mapping_tag.is_valid());

        // Pure white ambient material.
        mi::base::Handle<nv::index::IPhong_gl> material(
          scene->create_attribute<nv::index::IPhong_gl>());
        assert(material.is_valid_interface());

        material->set_ambient(mi::math::Color(1.f));
        material->set_diffuse(mi::math::Color(0.f));
        material->set_specular(mi::math::Color(0.f));

        const mi::neuraylib::Tag material_tag =
          dice_transaction->store_for_reference_counting(material.get());
        assert(material_tag.is_valid());

        // Planes
        mi::math::Vector_struct<mi::Uint32, 3> volume_size;
        regular_volume_properties->get_volume_size(volume_size);

        m_plane_tags.resize(MAX_SLICES);
        for (mi::Uint32 i = 0; i < MAX_SLICES; i++)
        {
          const vtknvindex_slice_params& slice_params = pv_config_settings->get_slice_params(i);

          mi::base::Handle<nv::index::IPlane> plane(scene->create_shape<nv::index::IPlane>());

          update_slice_planes(plane.get(), slice_params, volume_size);

          m_plane_tags[i] = dice_transaction->store_for_reference_counting(plane.get());
          assert(m_plane_tags[i].is_valid());
        }

        // Append slice and properties to transformed group.
        transform_group->append(rtc_surface_program_tag, dice_transaction.get());
        transform_group->append(m_prog_se_mapping_tag, dice_transaction.get());
        transform_group->append(material_tag, dice_transaction.get());

        for (mi::Uint32 i = 0; i < MAX_SLICES; i++)
          transform_group->append(m_plane_tags[i], dice_transaction.get());
      }
    }

    m_static_group_tag = dice_transaction->store_for_reference_counting(static_group.get());
    assert(m_static_group_tag.is_valid());
    mi::neuraylib::Tag transform_group_tag =
      dice_transaction->store_for_reference_counting(transform_group.get());
    assert(transform_group_tag.is_valid());

    root_group->append(m_static_group_tag, dice_transaction.get());
    root_group->append(transform_group_tag, dice_transaction.get());

    m_root_group_tag = dice_transaction->store_for_reference_counting(root_group.get());
    assert(m_root_group_tag.is_valid());

    // Added root group (volume and slices) to scene geom group
    mi::base::Handle<nv::index::IStatic_scene_group> scene_geom_group_edit(
      dice_transaction->edit<nv::index::IStatic_scene_group>(
        m_index_instance->get_scene_geom_group()));
    assert(scene_geom_group_edit.is_valid_interface());

    scene_geom_group_edit->append(m_root_group_tag, dice_transaction.get());

    // Setup camera parameters.
    update_camera(ren, dice_transaction);

    // Add the camera to the scene.
    if (m_is_parallel)
      scene->set_camera(m_index_instance->get_parallel_camera());
    else
      scene->set_camera(m_index_instance->get_perspective_camera());

    // Set global scene transform (only required for regular volumes).
    mi::math::Matrix<mi::Float32, 4, 4> scene_rotation_matrix(1.0f);
    mi::math::Vector_struct<mi::Float32, 3> scene_translate_vec;
    mi::math::Vector_struct<mi::Float32, 3> scene_scaling_vec;

    if (volume_type == VOLUME_TYPE_REGULAR)
    {
      mi::math::Vector<mi::Float32, 3> translation, scaling;
      regular_volume_properties->get_volume_translation(translation);
      regular_volume_properties->get_volume_scaling(scaling);

      // Need to translate if the volume doesn't start at [0,0,0].
      scene_translate_vec.x = translation.x;
      scene_translate_vec.y = translation.y;
      scene_translate_vec.z = translation.z;

      scene_scaling_vec.x = scaling.x;
      scene_scaling_vec.y = scaling.y;
      scene_scaling_vec.z = scaling.z;
    }
    else
    {
      scene_translate_vec.x = scene_translate_vec.y = scene_translate_vec.z = 0.0f;
      scene_scaling_vec.x = scene_scaling_vec.y = scene_scaling_vec.z = 1.0f;
    }
    scene->set_transform_matrix(scene_translate_vec, scene_rotation_matrix, scene_scaling_vec);
  }

  // Commit transaction.
  m_scene_created = true;
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_scene::scene_created() const
{
  return m_scene_created;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_scene::update_volume(
  const mi::base::Handle<mi::neuraylib::IDice_transaction>& dice_transaction,
  Volume_type volume_type)
{
  // Use volume compute interface when it's a regular volume and its size wasn't changed.
  if (volume_type == VOLUME_TYPE_REGULAR)
  {
    mi::base::Handle<const nv::index::ISparse_volume_scene_element> volume_scene_element(
      dice_transaction->access<const nv::index::ISparse_volume_scene_element>(m_volume_tag));
    assert(volume_scene_element.is_valid_interface());

    mi::math::Bbox<mi::Float32, 3> svol_prev_bbox = volume_scene_element->get_bounding_box();

    vtknvindex_regular_volume_properties* regular_volume_properties =
      m_cluster_properties->get_regular_volume_properties();

    mi::math::Vector_struct<mi::Uint32, 3> volume_size;
    regular_volume_properties->get_volume_size(volume_size);
    mi::math::Bbox<mi::Float32, 3> svol_cur_bbox = mi::math::Bbox<mi::Float32, 3>(
      mi::math::Vector<mi::Float32, 3>(0.f), mi::math::Vector<mi::Float32, 3>(volume_size));

    // In the case the volume size changed the compute interface cannot be used
    // and a new volume needs to be re-created in the scene graph
    if (svol_cur_bbox == svol_prev_bbox)
    {
      update_compute(dice_transaction);
      return;
    }
  }

  // Set the affinity information.
  mi::base::Handle<vtknvindex_affinity> affinity = m_cluster_properties->get_affinity();

  std::ostringstream s;
  affinity->scene_dump_affinity_info(s);
  INFO_LOG << s.str();

  // NVIDIA IndeX session takes ownership of the affinity.
  affinity->retain();
  m_index_instance->m_iindex_session->set_affinity_information(affinity.get());

  // Create and setup the scene.
  {
    // Access the session instance from the database.
    mi::base::Handle<const nv::index::ISession> session(
      dice_transaction->access<const nv::index::ISession>(m_index_instance->m_session_tag));
    assert(session.is_valid_interface());

    // Access (edit mode) the scene instance from the database.
    mi::base::Handle<nv::index::IScene> scene(
      dice_transaction->edit<nv::index::IScene>(session->get_scene()));
    assert(scene.is_valid_interface());

    // Dataset subcube border size-
    mi::Sint32 subcube_border = m_cluster_properties->get_config_settings()->get_subcube_border();

    // Print cluster details to the console.
    m_cluster_properties->print_info();

    vtknvindex_regular_volume_properties* regular_volume_properties =
      m_cluster_properties->get_regular_volume_properties();

    std::string scalar_type;
    regular_volume_properties->get_scalar_type(scalar_type);

    // Remove previous volume.
    mi::base::Handle<nv::index::IStatic_scene_group> static_group_edit(
      dice_transaction->edit<nv::index::IStatic_scene_group>(m_static_group_tag));
    assert(static_group_edit.is_valid_interface());

    static_group_edit->remove(m_volume_tag, dice_transaction.get());

    // Create new volume and replace.

    // Scene contains a regular volume.
    if (volume_type == VOLUME_TYPE_REGULAR)
    {
      mi::math::Vector_struct<mi::Uint32, 3> volume_size;
      regular_volume_properties->get_volume_size(volume_size);

      // Create shared memory regular volume data importer.
      vtknvindex_sparse_volume_importer* volume_importer =
        new vtknvindex_sparse_volume_importer(volume_size, subcube_border, scalar_type);
      assert(volume_importer);

      volume_importer->set_cluster_properties(m_cluster_properties);

      // Create the sparse volume scene element
      mi::math::Bbox<mi::Float32, 3> svol_bbox = mi::math::Bbox<mi::Float32, 3>(
        mi::math::Vector<mi::Float32, 3>(0.f), mi::math::Vector<mi::Float32, 3>(volume_size));
      mi::math::Matrix<mi::Float32, 4, 4> transform_mat(1.0f); // Identity matrix

      mi::base::Handle<nv::index::ISparse_volume_scene_element> volume_scene_element(
        scene->create_sparse_volume(
          svol_bbox, transform_mat, volume_importer, dice_transaction.get()));
      assert(volume_scene_element.is_valid_interface());

      if (volume_scene_element)
      {
        vtknvindex_config_settings* pv_config_settings =
          m_cluster_properties->get_config_settings();

        // Update colormap assignment.
        volume_scene_element->set_enabled(pv_config_settings->get_enable_volume());

        // Store the volume scene element in the distributed database.
        m_volume_tag = dice_transaction->store_for_reference_counting(volume_scene_element.get());
        assert(m_volume_tag.is_valid());

        // Update RTC program scene element mapping
        mi::base::Handle<nv::index::IRendering_kernel_program_scene_element_mapping>
          prog_se_mapping(
            dice_transaction->edit<nv::index::IRendering_kernel_program_scene_element_mapping>(
              m_prog_se_mapping_tag));
        assert(prog_se_mapping.is_valid_interface());

        // set volume mapping
        prog_se_mapping->set_mapping(1, m_volume_tag);
      }
      else
      {
        ERROR_LOG << "Unable to create a regular volume scene element.";
      }
    }
    else // Scene contains an irregular volume (unstructured volume grid).
    {
      mi::math::Bbox<mi::Float32, 3> ivol_bbox;
      regular_volume_properties->get_ivol_volume_extents(ivol_bbox);

      // Create shared memory irregular volume data importer.
      vtknvindex_irregular_volume_importer* volume_importer =
        new vtknvindex_irregular_volume_importer(subcube_border, scalar_type);

      assert(volume_importer);

      // Create a scene element that represents an irregular volume.
      volume_importer->set_cluster_properties(m_cluster_properties);
      mi::base::Handle<nv::index::IIrregular_volume_scene_element> volume_scene_element(
        scene->create_irregular_volume(ivol_bbox, -1.0f, volume_importer, dice_transaction.get()));
      assert(volume_scene_element.is_valid_interface());

      if (volume_scene_element)
      {
        volume_scene_element->set_enabled(true);

        // Store the volume scene element in the distributed database.
        m_volume_tag = dice_transaction->store_for_reference_counting(volume_scene_element.get());
        assert(m_volume_tag.is_valid());
      }
      else
      {
        ERROR_LOG << "Unable to create an irregular volume scene element.";
      }
    }

    // Append new volume to the static scene group.
    static_group_edit->append(m_volume_tag, dice_transaction.get());

    if (volume_type == VOLUME_TYPE_REGULAR)
      update_config_settings(dice_transaction);
  }
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_scene::update_scene(vtkRenderer* ren, vtkVolume* vol,
  const mi::base::Handle<mi::neuraylib::IDice_transaction>& dice_transaction,
  bool config_settings_changed, bool opacity_changed, bool slices_changed)
{

  const bool is_timeseries =
    m_cluster_properties->get_regular_volume_properties()->is_timeseries_data();

  // Update colormap.
  update_colormap(vol, dice_transaction, m_cluster_properties->get_regular_volume_properties());

  // Update config settings.
  if (config_settings_changed)
    update_config_settings(dice_transaction);

  // Update volume opacity.
  if (opacity_changed)
    update_volume_opacity(vol, dice_transaction);

  // Update slices.
  if (slices_changed || is_timeseries)
    update_slices(dice_transaction);

  // Update camera.
  update_camera(ren, dice_transaction);

  // update time series animation settings.
  if (is_timeseries)
  {
    vtknvindex_clock_pulse_generator* app_clock_pulse =
      static_cast<vtknvindex_clock_pulse_generator*>(
        m_index_instance->m_iindex_session->get_clock());

    if (app_clock_pulse)
    {
      mi::Sint32 cur_time_step =
        m_cluster_properties->get_regular_volume_properties()->get_current_time_step();
      app_clock_pulse->set_tick(cur_time_step);
    }
  }
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_scene::update_rtc_kernel(
  const mi::base::Handle<mi::neuraylib::IDice_transaction>& dice_transaction,
  const vtknvindex_rtc_params_buffer& rtc_param_buffer, Volume_type volume_type,
  bool kernel_changed)
{
  // Does it need to update kernel programs?
  if (kernel_changed)
  {
    assert(m_rtc_program_tag.is_valid());

    mi::base::Handle<nv::index::IRendering_kernel_program> rtc_program(
      dice_transaction->edit<nv::index::IRendering_kernel_program>(m_rtc_program_tag));
    assert(rtc_program.is_valid_interface());

    if (volume_type == VOLUME_TYPE_REGULAR)
    {
      switch (rtc_param_buffer.rtc_kernel)
      {
        case RTC_KERNELS_ISOSURFACE:
          rtc_program->set_program_source(KERNEL_ISOSURFACE_STRING);
          rtc_program->set_enabled(true);
          break;

        case RTC_KERNELS_DEPTH_ENHANCEMENT:
          rtc_program->set_program_source(KERNEL_DEPTH_ENHANCEMENT_STRING);
          rtc_program->set_enabled(true);
          break;

        case RTC_KERNELS_EDGE_ENHANCEMENT:
          rtc_program->set_program_source(KERNEL_EDGE_ENHANCEMENT_STRING);
          rtc_program->set_enabled(true);
          break;

        case RTC_KERNELS_GRADIENT:
          rtc_program->set_program_source(KERNEL_GRADIENT_STRING);
          rtc_program->set_enabled(true);
          break;

        case RTC_KERNELS_NONE:
        default:
          rtc_program->set_enabled(false);
          break;
      }
    }
    else // irregular volume kernels
    {
      switch (rtc_param_buffer.rtc_kernel)
      {
        case RTC_KERNELS_ISOSURFACE:
          rtc_program->set_program_source(KERNEL_IRREGULAR_ISOSURFACE_STRING);
          rtc_program->set_enabled(true);
          break;

        case RTC_KERNELS_DEPTH_ENHANCEMENT:
          rtc_program->set_program_source(KERNEL_IRREGULAR_DEPTH_ENHANCEMENT_STRING);
          rtc_program->set_enabled(true);
          break;

        case RTC_KERNELS_EDGE_ENHANCEMENT:
          rtc_program->set_program_source(KERNEL_IRREGULAR_EDGE_ENHANCEMENT_STRING);
          rtc_program->set_enabled(true);
          break;

        case RTC_KERNELS_NONE:
        default:
          rtc_program->set_enabled(false);
          break;
      }
    }
  }

  // Does it need to update kernel's parameter buffer?
  assert(m_rtc_program_params_tag.is_valid());

  mi::base::Handle<nv::index::IRendering_kernel_program_parameters> rtc_program_parameters(
    dice_transaction->edit<nv::index::IRendering_kernel_program_parameters>(
      m_rtc_program_params_tag));
  assert(rtc_program_parameters.is_valid_interface());

  if (rtc_param_buffer.rtc_kernel != RTC_KERNELS_NONE)
  {
    if (volume_type == VOLUME_TYPE_REGULAR)
    {
      rtc_program_parameters->set_buffer_data(
        0, rtc_param_buffer.params_buffer, rtc_param_buffer.buffer_size);
      rtc_program_parameters->set_enabled(true);
    }
    else // irregular volume kernels
    {
      switch (rtc_param_buffer.rtc_kernel)
      {
        case RTC_KERNELS_ISOSURFACE:
        {
          vtknvindex_ivol_isosurface_params iso_params =
            *(reinterpret_cast<const vtknvindex_ivol_isosurface_params*>(
              rtc_param_buffer.params_buffer));

          iso_params.rh = m_cluster_properties->get_config_settings()->get_ivol_step_size();
          rtc_program_parameters->set_buffer_data(0, &iso_params, rtc_param_buffer.buffer_size);
          rtc_program_parameters->set_enabled(true);
        }
        break;

        case RTC_KERNELS_DEPTH_ENHANCEMENT:
        {
          vtknvindex_ivol_depth_enhancement_params depth_params =
            *(reinterpret_cast<const vtknvindex_ivol_depth_enhancement_params*>(
              rtc_param_buffer.params_buffer));

          depth_params.rh = m_cluster_properties->get_config_settings()->get_ivol_step_size();
          rtc_program_parameters->set_buffer_data(0, &depth_params, rtc_param_buffer.buffer_size);
          rtc_program_parameters->set_enabled(true);
        }
        break;

        case RTC_KERNELS_EDGE_ENHANCEMENT:
        {
          vtknvindex_ivol_edge_enhancement_params edge_params =
            *(reinterpret_cast<const vtknvindex_ivol_edge_enhancement_params*>(
              rtc_param_buffer.params_buffer));

          edge_params.rh = m_cluster_properties->get_config_settings()->get_ivol_step_size();
          rtc_program_parameters->set_buffer_data(0, &edge_params, rtc_param_buffer.buffer_size);
          rtc_program_parameters->set_enabled(true);
        }
        break;

        case RTC_KERNELS_NONE:
        default:
          rtc_program_parameters->set_buffer_data(
            0, rtc_param_buffer.params_buffer, rtc_param_buffer.buffer_size);
          rtc_program_parameters->set_enabled(true);
          break;
      }
    }
  }
  else
  {
    rtc_program_parameters->set_enabled(false);
  }

  // Update irregular volume properties
  if (kernel_changed && (volume_type == VOLUME_TYPE_IRREGULAR))
  {
    mi::base::Handle<nv::index::IIrregular_volume_rendering_properties> vol_render_properties(
      dice_transaction->edit<nv::index::IIrregular_volume_rendering_properties>(
        m_vol_properties_tag));
    assert(vol_render_properties.is_valid_interface());

    nv::index::IIrregular_volume_rendering_properties::Rendering render_properties;
    vol_render_properties->get_rendering(render_properties);

    render_properties.sampling_mode = (rtc_param_buffer.rtc_kernel == RTC_KERNELS_NONE) ? 0 : 1;

    vol_render_properties->set_rendering(render_properties);
  }
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_scene::update_camera(
  vtkRenderer* ren, const mi::base::Handle<mi::neuraylib::IDice_transaction>& dice_transaction)
{
  vtkSmartPointer<vtkCamera> app_camera = ren->GetActiveCamera();
  bool is_parallel = app_camera->GetParallelProjection() != 0;

  mi::Float64 x, y, z;
  app_camera->GetPosition(x, y, z);
  mi::math::Vector<mi::Float32, 3> eye((mi::Float32)x, (mi::Float32)y, (mi::Float32)z);

  app_camera->GetFocalPoint(x, y, z);
  mi::math::Vector<mi::Float32, 3> center((mi::Float32)x, (mi::Float32)y, (mi::Float32)z);

  app_camera->GetViewUp(x, y, z);
  mi::math::Vector<mi::Float32, 3> up((mi::Float32)x, (mi::Float32)y, (mi::Float32)z);

  mi::math::Vector<mi::Float32, 3> view_direction(center - eye);
  view_direction.normalize();

  const mi::Sint32* window_size = ren->GetVTKWindow()->GetActualSize();
  mi::Float32 aspect_ratio = static_cast<mi::Float32>(window_size[0]) / window_size[1];

  mi::Float64* clipping_range = app_camera->GetClippingRange();

  if (is_parallel)
  {
    mi::base::Handle<nv::index::IOrthographic_camera> nvindex_camera(
      dice_transaction->edit<nv::index::IOrthographic_camera>(
        m_index_instance->get_parallel_camera()));
    assert(nvindex_camera.is_valid_interface());

    nvindex_camera->set(eye, view_direction, up);
    nvindex_camera->set_aspect(aspect_ratio);
    nvindex_camera->set_clip_min(clipping_range[0]);
    nvindex_camera->set_clip_max(clipping_range[1]);

    mi::Float32 aperture = app_camera->GetParallelScale() * 2.0 * aspect_ratio;
    nvindex_camera->set_aperture(aperture);
  }
  else
  {
    mi::base::Handle<nv::index::IPerspective_camera> nvindex_camera(
      dice_transaction->edit<nv::index::IPerspective_camera>(
        m_index_instance->get_perspective_camera()));
    assert(nvindex_camera.is_valid_interface());

    nvindex_camera->set(eye, view_direction, up);
    nvindex_camera->set_aspect(aspect_ratio);
    nvindex_camera->set_clip_min(clipping_range[0]);
    nvindex_camera->set_clip_max(clipping_range[1]);

    mi::Float32 aperture = 0.033f;
    nvindex_camera->set_aperture(aperture);

    // Focal distance is set to match ParaView's fixed vertical FOV of 30 deg.
    mi::Float64 focal_distance = (aperture / (aspect_ratio * 0.53589838529));
    nvindex_camera->set_focal(focal_distance);
  }

  // camera projection changed
  if (m_is_parallel != is_parallel)
  {
    // Access the session instance from the database.
    mi::base::Handle<const nv::index::ISession> session(
      dice_transaction->access<const nv::index::ISession>(m_index_instance->m_session_tag));
    assert(session.is_valid_interface());

    // Access (edit mode) the scene instance from the database.
    mi::base::Handle<nv::index::IScene> scene(
      dice_transaction->edit<nv::index::IScene>(session->get_scene()));
    assert(scene.is_valid_interface());

    if (is_parallel)
      scene->set_camera(m_index_instance->get_parallel_camera());
    else
      scene->set_camera(m_index_instance->get_perspective_camera());

    m_is_parallel = is_parallel;
  }
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_scene::update_colormap(vtkVolume* vol,
  const mi::base::Handle<mi::neuraylib::IDice_transaction>& dice_transaction,
  vtknvindex_regular_volume_properties* regular_volume_properties)
{
  m_index_instance->get_index_colormaps()->update_scene_colormaps(
    vol, dice_transaction, regular_volume_properties);
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_scene::update_config_settings(
  const mi::base::Handle<mi::neuraylib::IDice_transaction>& dice_transaction)
{
  // Access the session instance from the database.
  mi::base::Handle<const nv::index::ISession> session(
    dice_transaction->access<const nv::index::ISession>(m_index_instance->m_session_tag));
  assert(session.is_valid_interface());

  // Access (edit mode) the scene instance from the database.
  mi::base::Handle<nv::index::IScene> scene(
    dice_transaction->edit<nv::index::IScene>(session->get_scene()));
  assert(scene.is_valid_interface());

  vtknvindex_config_settings* pv_config_settings = m_cluster_properties->get_config_settings();

  // Set region of interest.
  mi::math::Bbox_struct<mi::Float32, 3> region_of_interest;
  pv_config_settings->get_region_of_interest(region_of_interest);
  scene->set_clipped_bounding_box(region_of_interest);

  // Access (edit mode) the global configurations.
  mi::base::Handle<nv::index::IConfig_settings> config_settings(
    dice_transaction->edit<nv::index::IConfig_settings>(session->get_config()));
  assert(config_settings.is_valid_interface());

  // CUDA shader mode.
  const vtknvindex_rtc_kernels rtc_kernel = pv_config_settings->get_rtc_kernel();
  assert(m_sparse_volume_render_properties_tag.is_valid());

  mi::base::Handle<nv::index::ISparse_volume_rendering_properties> sparse_volume_render_properties(
    dice_transaction->edit<nv::index::ISparse_volume_rendering_properties>(
      m_sparse_volume_render_properties_tag));

  assert(sparse_volume_render_properties.is_valid_interface());

  // filter mode
  const nv::index::Sparse_volume_filter_mode filter_mode =
    static_cast<nv::index::Sparse_volume_filter_mode>(pv_config_settings->get_filter_mode());

  sparse_volume_render_properties->set_filter_mode(filter_mode);

  // use pre-integration
  bool use_preintegration = rtc_kernel == RTC_KERNELS_NONE &&
    !m_index_instance->get_index_colormaps()->changed() && pv_config_settings->is_preintegration();
  sparse_volume_render_properties->set_preintegrated_volume_rendering(use_preintegration);

  // sampling distance
  sparse_volume_render_properties->set_sampling_distance(pv_config_settings->get_step_size());

  // set voxel offset
  sparse_volume_render_properties->set_voxel_offsets(mi::math::Vector<mi::Float32, 3>(0.5f));

  // Set sparse volume brick size and border size.
  // For performance reasons is best to have (BD + 2*BS) to be power of 2.
  nv::index::IConfig_settings::Sparse_volume_config sparse_volume_config;
  sparse_volume_config.brick_dimensions = mi::math::Vector<mi::Uint32, 3>(60u);
  sparse_volume_config.brick_shared_border_size = 2u;

  config_settings->set_sparse_volume_configuration(sparse_volume_config);

  if (m_only_init)
  {
    m_only_init = false;

    // Subcube border size
    mi::Uint32 subcube_border_size = pv_config_settings->get_subcube_border();

    // Subcube size
    mi::math::Vector_struct<mi::Uint32, 3> subcube_size;
    pv_config_settings->get_subcube_size(subcube_size);

    // Adding padding to subcube size in order to build a
    // subcube size multiple of the volume size when border size is present.
    if (vtkMultiProcessController::GetGlobalController()->GetNumberOfProcesses() > 1)
    {
      vtknvindex_regular_volume_properties* regular_volume_properties =
        m_cluster_properties->get_regular_volume_properties();
      mi::math::Vector_struct<mi::Uint32, 3> volume_size;
      regular_volume_properties->get_volume_size(volume_size);

      if (volume_size.x > subcube_size.x)
        subcube_size.x += (volume_size.x / subcube_size.x - 1) * 2;
      if (volume_size.y > subcube_size.y)
        subcube_size.y += (volume_size.y / subcube_size.y - 1) * 2;
      if (volume_size.z > subcube_size.z)
        subcube_size.z += (volume_size.z / subcube_size.z - 1) * 2;
    }

    subcube_size.x -= 2 * subcube_border_size;
    subcube_size.y -= 2 * subcube_border_size;
    subcube_size.z -= 2 * subcube_border_size;

    // Supporting continuous translation requires additional memory compared to when only
    // integer translation is supported.
    bool support_continuous_translation = false;

    // Subcube size will be changed internally if the rotation of a volume scene elements is
    // supported.
    bool support_rotation = false;

    // Same here: if they are scaled with a scale factor < 1, then need to specify the minimum
    // factor.
    mi::Float32_3 min_scale = mi::Float32_3(1, 1, 1);

    const bool is_success = config_settings->set_subcube_configuration(subcube_size,
      subcube_border_size, support_continuous_translation, support_rotation, min_scale);

    if (!is_success)
      ERROR_LOG << "Failed to set the subcube configuration.";

    // Super sampling
    const mi::Uint32 rendering_samples = 1;
    config_settings->set_rendering_samples(rendering_samples);

    // A good number for max spans is (no.of physical cpu cores/2).
    vtksys::SystemInformation sys_info;
    sys_info.RunCPUCheck();
    mi::Uint32 nb_physical_cores = sys_info.GetNumberOfPhysicalCPU();
    if (nb_physical_cores == 0)
      nb_physical_cores = 8; // when information not available, assuming 8 cores.
    config_settings->set_max_spans_per_machine(nb_physical_cores / 2);
    config_settings->set_automatic_span_control(true);

    // Data transfer configuration.
    // TODO: performance implications?
    nv::index::IConfig_settings::Data_transfer_config data_transfer_config;
    data_transfer_config.span_alpha_channel = true;
    data_transfer_config.span_compression_level = 1;
    data_transfer_config.span_image_encoding = true;
    data_transfer_config.tile_compression_level = 1;
    data_transfer_config.tile_image_encoding = true;

    config_settings->set_data_transfer_config(data_transfer_config);
  }

  // Logging performance values.
  if (m_cluster_properties->get_config_settings()->is_log_performance())
    config_settings->set_monitor_performance_values(true);
  else
    config_settings->set_monitor_performance_values(false);

  // Dump internal state.
  if (pv_config_settings->is_dump_internal_state())
    export_session();
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_scene::update_volume_opacity(
  vtkVolume* vol, const mi::base::Handle<mi::neuraylib::IDice_transaction>& dice_transaction)
{
  assert(m_vol_properties_tag.is_valid());

  mi::base::Handle<const mi::neuraylib::IElement> vol_elem(
    dice_transaction->access<mi::neuraylib::IElement>(m_volume_tag));

  mi::base::Handle<const nv::index::ISparse_volume_scene_element> sparse_vol_elem(
    vol_elem->get_interface<nv::index::ISparse_volume_scene_element>());

  // Is regular volume?
  if (sparse_vol_elem.is_valid_interface())
  {
    mi::base::Handle<nv::index::IRegular_volume_rendering_properties> vol_properties(
      dice_transaction->edit<nv::index::IRegular_volume_rendering_properties>(
        m_vol_properties_tag));
    assert(vol_properties.is_valid_interface());

    vtknvindex_config_settings* pv_config_settings = m_cluster_properties->get_config_settings();

    vol_properties->set_reference_step_size(calculate_volume_reference_step_size(
      vol, pv_config_settings->get_opacity_mode(), pv_config_settings->get_opacity_reference()));
  }
  else // It's irregular volume
  {
    mi::base::Handle<nv::index::IIrregular_volume_rendering_properties> vol_render_properties(
      dice_transaction->edit<nv::index::IIrregular_volume_rendering_properties>(
        m_vol_properties_tag));
    assert(vol_render_properties.is_valid_interface());

    nv::index::IIrregular_volume_rendering_properties::Rendering render_properties;
    vol_render_properties->get_rendering(render_properties);

    render_properties.sampling_segment_length =
      m_cluster_properties->get_config_settings()->get_ivol_step_size() *
      m_cluster_properties->get_config_settings()->get_step_size();

    vol_render_properties->set_rendering(render_properties);
  }
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_scene::update_slices(
  const mi::base::Handle<mi::neuraylib::IDice_transaction>& dice_transaction)
{
  // Retrieve current slice parameter
  vtknvindex_config_settings* pv_config_settings = m_cluster_properties->get_config_settings();

  // Update volume visibility.
  const bool enable_volume = pv_config_settings->get_enable_volume();

  mi::base::Handle<nv::index::ISparse_volume_scene_element> volume(
    dice_transaction->edit<nv::index::ISparse_volume_scene_element>(m_volume_tag));
  assert(volume.is_valid_interface());

  if (volume->get_enabled() != enable_volume)
    volume->set_enabled(enable_volume);

  // Update slices.
  vtknvindex_regular_volume_properties* regular_volume_properties =
    m_cluster_properties->get_regular_volume_properties();

  mi::math::Vector_struct<mi::Uint32, 3> volume_size;
  regular_volume_properties->get_volume_size(volume_size);

  for (mi::Uint32 i = 0; i < m_plane_tags.size(); i++)
  {
    const vtknvindex_slice_params& slice_params = pv_config_settings->get_slice_params(i);

    mi::base::Handle<nv::index::IPlane> plane(
      dice_transaction->edit<nv::index::IPlane>(m_plane_tags[i]));
    assert(plane.is_valid_interface());

    update_slice_planes(plane.get(), slice_params, volume_size);
  }
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_scene::update_compute(
  const mi::base::Handle<mi::neuraylib::IDice_transaction>& dice_transaction)
{
  mi::base::Handle<vtknvindex_volume_compute> vol_compute(
    dice_transaction->edit<vtknvindex_volume_compute>(m_volume_compute_attrib_tag));
  assert(vol_compute.is_valid_interface());

  if (vol_compute.is_valid_interface())
    vol_compute->set_enabled(true);
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_scene::set_cluster_properties(vtknvindex_cluster_properties* cluster_properties)
{
  m_cluster_properties = cluster_properties;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_scene::export_session()
{
  // TODO: Add scene lock here before accessing the export file.
  mi::base::Handle<mi::neuraylib::IDice_transaction> dice_transaction(
    m_index_instance->m_global_scope->create_transaction<mi::neuraylib::IDice_transaction>());

  if (!dice_transaction.is_valid_interface())
    return;

  {
    mi::base::Handle<mi::neuraylib::IFactory> factory(
      m_index_instance->get_interface()->get_api_component<mi::neuraylib::IFactory>());
    mi::base::Handle<mi::IString> str(factory->create<mi::IString>("String"));

    mi::base::Handle<const nv::index::ISession> session(
      dice_transaction->access<nv::index::ISession>(m_index_instance->m_session_tag));

    session->export_session(nv::index::ISession::EXPORT_DEFAULT, str.get(), dice_transaction.get());

    std::ostringstream s;
    s << str->get_c_str();

    std::string output_filename = "nvindex_pvplugin.prj";
    if (output_filename.empty() || output_filename == "stdout")
    {
      std::cout << "\n"
                << "----------------8<-------------[ cut here ]------------------\n"
                << s.str() << "----------------8<-------------[ cut here ]------------------\n"
                << std::endl;
    }
    else
    {
      INFO_LOG << "Writing export to file '" << output_filename << "'";
      std::ofstream f(output_filename.c_str());
      f << s.str();
      std::ostringstream af;
      m_cluster_properties->get_affinity()->scene_dump_affinity_info(af);
      f << af.str();
      f.close();
    }
  }
  dice_transaction->commit();
}

//-------------------------------------------------------------------------------------------------
mi::Float32 vtknvindex_scene::calculate_volume_reference_step_size(
  vtkVolume* vol, mi::Uint32 mode, mi::Float32 opacity) const
{
  vtknvindex_regular_volume_properties* regular_volume_properties =
    m_cluster_properties->get_regular_volume_properties();

  mi::math::Vector<mi::Float32, 3> scaling;
  regular_volume_properties->get_volume_scaling(scaling);

  mi::Float32 avg_scaling = (scaling.x + scaling.y + scaling.z) / 3.f;

  vtkVolumeProperty* property = vol->GetProperty();
  mi::Float32 vod = static_cast<mi::Float32>(property->GetScalarOpacityUnitDistance());

  mi::Float32 vol_ref_step_size = vod / avg_scaling;

  switch (mode)
  {
    default:
    case 0: // voxel reference
      break;

    case 1: // volume reference
    {
      mi::math::Vector_struct<mi::Uint32, 3> volume_size;
      regular_volume_properties->get_volume_size(volume_size);

      mi::Float32 avg_size = (volume_size.x + volume_size.y + volume_size.z) / 3.0f;

      vol_ref_step_size *= avg_size / opacity;
    }
    break;

    case 2: // User defined
      vol_ref_step_size *= opacity;
      break;
  }

  return vol_ref_step_size;
}
