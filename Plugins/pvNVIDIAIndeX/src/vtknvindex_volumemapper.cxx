/* Copyright 2025 NVIDIA Corporation. All rights reserved.
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
// SPDX-FileCopyrightText: Copyright 2025 NVIDIA Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include <cassert>
#include <cstdio>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif // _WIN32

#include "vtk_glad.h"

#include "vtkCellData.h"
#include "vtkCommand.h"

#include "vtkFixedPointVolumeRayCastMapper.h"
#include "vtkImageData.h"

#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkMatrix4x4.h"
#include "vtkMultiThreader.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPKdTree.h"
#include "vtkPVRenderViewSettings.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include <nv/index/ilight.h>
#include <nv/index/imaterial.h>

#include "vtknvindex_cluster_properties.h"
#include "vtknvindex_forwarding_logger.h"
#include "vtknvindex_global_settings.h"
#include "vtknvindex_instance.h"
#include "vtknvindex_utilities.h"
#include "vtknvindex_volumemapper.h"

vtkStandardNewMacro(vtknvindex_volumemapper);

//----------------------------------------------------------------------------
vtknvindex_volumemapper::vtknvindex_volumemapper()
  : m_is_caching(false)
  , m_is_mapper_initialized(false)
  , m_config_settings_changed(false)
  , m_opacity_changed(false)
  , m_slices_changed(false)
  , m_volume_changed(false)
  , m_rtc_kernel_changed(false)
  , m_rtc_param_changed(false)
  , m_last_MTime(0)
  , m_prev_vector_component(-1)
  , m_prev_vector_mode(-1)
  , m_is_mpi_rendering(false)
  , m_region_of_interest_deprecated_warning_printed(false)

{
  m_index_instance = vtknvindex_instance::get();
  m_controller = vtkMultiProcessController::GetGlobalController();
}

//----------------------------------------------------------------------------
vtknvindex_volumemapper::~vtknvindex_volumemapper() = default;

//----------------------------------------------------------------------------
double* vtknvindex_volumemapper::GetBounds()
{
  // Return the whole volume bounds instead of the local one to avoid that
  // ParaView culls off any rank

  return m_whole_bounds;
}

//----------------------------------------------------------------------------
void vtknvindex_volumemapper::set_whole_bounds(const mi::math::Bbox<mi::Float64, 3>& bounds)
{
  m_whole_bounds[0] = bounds.min.x;
  m_whole_bounds[1] = bounds.max.x;
  m_whole_bounds[2] = bounds.min.y;
  m_whole_bounds[3] = bounds.max.y;
  m_whole_bounds[4] = bounds.min.z;
  m_whole_bounds[5] = bounds.max.z;
}

//----------------------------------------------------------------------------
void vtknvindex_volumemapper::shutdown()
{
  // TODO: Remove volume from scene graph
}

//----------------------------------------------------------------------------
void vtknvindex_volumemapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
static void reset_orthogonal_projection_matrix(mi::Sint32& win_width, mi::Sint32& win_height)
{
  assert((win_width > 0) && (win_height));

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0f, static_cast<mi::Float32>(win_width), 0.0f, static_cast<mi::Float32>(win_height),
    0.0f, 1.0f);
  glMatrixMode(GL_MODELVIEW);
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_volumemapper::prepare_data(mi::Sint32 time_step)
{
  vtkTimerLog::MarkStartEvent("NVIDIA-IndeX: Preparing data");

  int extent[6];
  vtkImageData* image_piece = vtkImageData::SafeDownCast(this->GetInput());
  image_piece->GetExtent(extent);

  // Get ParaView's volume data
  vtkDataArray* scalar_array = m_scalar_array;
  if (m_cluster_properties->get_regular_volume_properties()->is_timeseries_data())
  {
    // Get the input
    if (image_piece == nullptr)
    {
      ERROR_LOG << "vtkImageData in representation is invalid!";
      return false;
    }

    mi::Sint32 cell_flag;
    scalar_array = this->GetScalars(image_piece, this->ScalarMode, this->ArrayAccessMode,
      this->ArrayId, this->ArrayName, cell_flag);

    vtkDataArray* converted_array = this->convert_scalar_array(scalar_array);
    if (converted_array)
    {
      scalar_array = converted_array;
      // Keep track of converted array, to ensure it will be freed
      m_converted_scalar_arrays[time_step] = vtkSmartPointer<vtkDataArray>::Take(converted_array);
    }
    else
    {
      // Conversion not necessary, use scalar_array directly
      m_converted_scalar_arrays[time_step] = nullptr;
    }

    m_subset_ptrs[time_step] = scalar_array->GetVoidPointer(0);
  }

  // Write volume data to shared memory.
  if (!m_cluster_properties->get_regular_volume_properties()->write_shared_memory(scalar_array,
        extent, m_cluster_properties->get_host_properties(m_controller->GetLocalProcessId()),
        time_step, !m_index_instance->is_index_rank()))
  {
    ERROR_LOG << "Failed to write vtkImageData piece into shared memory"
              << " in vtknvindex_representation::RequestData().";
    return false;
  }

  m_time_step_data_prepared[time_step] = true;

  vtkTimerLog::MarkEndEvent("NVIDIA-IndeX: Preparing data");

  return true;
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_volumemapper::initialize_mapper(vtkVolume* vol)
{
  vtkTimerLog::MarkStartEvent("NVIDIA-IndeX: Initialization");

  // Initialize and start IndeX (if not initialized yet and if this is an IndeX rank)
  m_index_instance->init_index();

  m_is_mpi_rendering = (m_controller->GetNumberOfProcesses() > 1);
  const mi::Sint32 cur_global_rank = m_is_mpi_rendering ? m_controller->GetLocalProcessId() : 0;

  // Init scalar pointers array
  if (m_cluster_properties->get_regular_volume_properties()->is_timeseries_data())
  {
    m_subset_ptrs.resize(
      m_cluster_properties->get_regular_volume_properties()->get_nb_time_steps());
  }
  else
  {
    m_subset_ptrs.resize(1);
  }
  m_converted_scalar_arrays.resize(m_subset_ptrs.size());

  // Update in_volume first to make sure states are current.
  vol->Update();

  // Get the input.
  vtkImageData* image_piece = vtkImageData::SafeDownCast(this->GetInput());
  if (image_piece == nullptr)
  {
    ERROR_LOG << "vtkImageData in representation is invalid.";
    return false;
  }

  mi::Sint32 cell_flag;
  vtkDataArray* scalar_array = this->GetScalars(image_piece, this->ScalarMode,
    this->ArrayAccessMode, this->ArrayId, this->ArrayName, cell_flag);
  m_scalar_array = scalar_array;

  bool is_data_supported = true;

  // Only per cell scalars are allowed
  if (cell_flag > 0)
  {
    ERROR_LOG << "The data array '" << this->ArrayName << "' uses "
              << (cell_flag == 1 ? "per cell" : "per field")
              << " scalars, but only per point scalars are supported by NVIDIA IndeX "
              << "(cellFlag is " << cell_flag << ").";
    is_data_supported = false;
  }

  // Check for valid data types
  const std::string scalar_type = scalar_array->GetDataTypeAsString();
  if (vtknvindex_regular_volume_properties::get_scalar_size(scalar_type) == 0)
  {
    ERROR_LOG << "The data array '" << this->ArrayName << "' uses the scalar type '" << scalar_type
              << "', which is not supported by NVIDIA IndeX.";
    return false;
  }
  else if (scalar_type == "double" && is_data_supported)
  {
    // Only print the warning once per data array, and do not repeat when switching between arrays.
    // No warning for "int" and "unsigned int" because there is no memory overhead and only minimal
    // CPU overhead.
    if (m_data_array_warning_printed.find(this->ArrayName) == m_data_array_warning_printed.end())
    {
      WARN_LOG << "The data array '" << this->ArrayName << "' has scalar values "
               << "in " << scalar_type << " format, which is not natively "
               << "supported by NVIDIA IndeX. "
               << "The plugin will proceed to convert the values from " << scalar_type << " "
               << "to float with the corresponding overhead.";
      m_data_array_warning_printed.emplace(this->ArrayName);
    }
  }
  else if (scalar_type == "char")
  {
    if (m_data_array_warning_printed.find(this->ArrayName) == m_data_array_warning_printed.end())
    {
      WARN_LOG << "The data array '" << this->ArrayName << "' has scalar values "
               << "in '" << scalar_type << "' format which is platform dependent. "
               << "Use 'signed char' or 'unsigned char' instead.";
      m_data_array_warning_printed.emplace(this->ArrayName);
    }
  }

  if (!is_data_supported)
  {
    return false;
  }

  vtkDataArray* converted_array = this->convert_scalar_array(scalar_array);
  if (converted_array)
  {
    scalar_array = converted_array;
    // Keep track of converted array, to ensure it will be freed
    m_converted_scalar_arrays[0] = vtkSmartPointer<vtkDataArray>::Take(converted_array);
  }
  else
  {
    // Conversion not necessary, use scalar_array directly
    m_converted_scalar_arrays[0] = nullptr;
  }

  m_subset_ptrs[0] = scalar_array->GetVoidPointer(0);

  vtknvindex_regular_volume_data volume_data;
  volume_data.scalars = &m_subset_ptrs[0];

  int extent[6];
  image_piece->GetExtent(extent);

  vtknvindex_dataset_parameters dataset_parameters;
  dataset_parameters.volume_type =
    vtknvindex_scene::VOLUME_TYPE_REGULAR; // vtknviVTKNVINDEX_VOLUME_TYPE_REGULAR;
  dataset_parameters.scalar_type = scalar_type;

  int nb_components = m_scalar_array->GetNumberOfComponents(); // use unconverted array
  if (scalar_array->GetNumberOfComponents() != nb_components)
  {
    // negative means the data was already converted to a single component
    nb_components = -nb_components;
  }
  dataset_parameters.scalar_components = nb_components;

  dataset_parameters.voxel_range[0] = static_cast<mi::Float32>(
    scalar_array->GetRange(0)[0]); // '0' is component ID TODO: do this in a clean way
  dataset_parameters.voxel_range[1] = static_cast<mi::Float32>(
    scalar_array->GetRange(0)[1]); // '0' is component ID TODO: do this in a clean way
  dataset_parameters.scalar_range[0] = static_cast<mi::Float32>(scalar_array->GetDataTypeMin());
  dataset_parameters.scalar_range[1] = static_cast<mi::Float32>(scalar_array->GetDataTypeMax());

  dataset_parameters.bounds[0] = extent[0];
  dataset_parameters.bounds[1] = extent[1];
  dataset_parameters.bounds[2] = extent[2];
  dataset_parameters.bounds[3] = extent[3];
  dataset_parameters.bounds[4] = extent[4];
  dataset_parameters.bounds[5] = extent[5];

  dataset_parameters.volume_data = &volume_data;

  if (m_is_mpi_rendering)
  {
    bool matches_whole_bounds = true;
    for (int i = 0; i < 6; ++i)
    {
      if (static_cast<int>(m_whole_bounds[i]) != extent[i])
      {
        matches_whole_bounds = false;
        break;
      }
    }

    if (matches_whole_bounds)
    {
      WARN_LOG << "Parallel rendering disabled, only a single rank will be used! "
               << "The extent of the volume piece on MPI rank " << cur_global_rank << " (of "
               << m_controller->GetNumberOfProcesses() << ") "
               << "is equal to the extent of the entire volume: [" << extent[0] << " " << extent[2]
               << " " << extent[4] << "; " << extent[1] << " " << extent[3] << " " << extent[5]
               << "]. "
               << "This typically happens when using a file format that does not support parallel "
               << "processing, such as the legacy VTK file format.";
      m_is_mpi_rendering = false;
    }
  }

  // clean shared memory
  m_cluster_properties->unlink_shared_memory(true);

  // Collect dataset type, ranges, bounding boxes, scalar values and affinity to be passed to NVIDIA
  // IndeX.
  if (m_is_mpi_rendering)
  {
    mi::Sint32 current_hostid = 0;
    if (m_index_instance->is_index_rank())
    {
      // This is generated by NVIDIA IndeX.
      current_hostid = get_local_hostid();
    }

    if (!m_cluster_properties->retrieve_cluster_configuration(
          dataset_parameters, current_hostid, m_index_instance->is_index_rank()))
    {
      ERROR_LOG << "Failed to retrieve cluster configuration"
                << " in vtknvindex_representation::RequestData().";
      return false;
    }
  }
  else
  {
    if (!m_cluster_properties->retrieve_process_configuration(dataset_parameters))
    {
      ERROR_LOG << "Failed to retrieve process configuration"
                << " in vtknvindex_representation::RequestData().";
      return false;
    }
  }

  m_is_mapper_initialized = true;

  if (m_is_mpi_rendering)
  {
    m_controller->Barrier();
  }

  vtkTimerLog::MarkEndEvent("NVIDIA-IndeX: Initialization");

  return true;
}

//-------------------------------------------------------------------------------------------------
mi::Sint32 vtknvindex_volumemapper::get_local_hostid()
{
  return m_index_instance->m_icluster_configuration->get_local_host_id();
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_volumemapper::set_cluster_properties(
  vtknvindex_cluster_properties* cluster_properties)
{
  m_cluster_properties = cluster_properties;
  m_scene.set_cluster_properties(cluster_properties);
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_volumemapper::update_canvas(vtkRenderer* ren)
{
  vtkWindow* win = ren->GetVTKWindow();
  int* window_size = win->GetSize();

  const mi::math::Vector_struct<mi::Sint32, 2> main_window_resolution = { window_size[0],
    window_size[1] };

  m_index_instance->m_opengl_canvas.set_vtk_renderer(ren);
  m_index_instance->m_opengl_canvas.set_buffer_resolution(main_window_resolution);

  if (ren->GetNumberOfPropsRendered())
  {
    vtkOpenGLRenderWindow* vtk_gl_render_window = vtkOpenGLRenderWindow::SafeDownCast(win);
    const mi::Sint32 depth_bits = vtk_gl_render_window->GetDepthBufferSize();
    if (depth_bits > 0)
    {
      m_index_instance->m_opengl_app_buffer.set_z_buffer_precision(depth_bits);
      m_index_instance->m_opengl_app_buffer.resize_buffer(main_window_resolution);

      mi::Uint32* pv_z_buffer = m_index_instance->m_opengl_app_buffer.get_z_buffer_ptr();
      glReadPixels(
        0, 0, window_size[0], window_size[1], GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, pv_z_buffer);
    }
    else
    {
      // This can happen when enabling "Axes Grid", as those are rendered without a depth buffer.
      m_index_instance->m_opengl_app_buffer.set_z_buffer_precision(0);
    }
  }

  reset_orthogonal_projection_matrix(window_size[0], window_size[1]);
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_volumemapper::config_settings_changed()
{
  m_config_settings_changed = true;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_volumemapper::opacity_changed()
{
  m_opacity_changed = true;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_volumemapper::slices_changed()
{
  m_slices_changed = true;
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_volumemapper::rtc_kernel_changed(vtknvindex_rtc_kernels kernel,
  const std::string& kernel_program, const void* params_buffer, mi::Uint32 buffer_size)
{
  bool kernel_changed = false;
  if (kernel != m_volume_rtc_kernel.rtc_kernel)
  {
    m_volume_rtc_kernel.rtc_kernel = kernel;
    kernel_changed = true;
  }

  if (kernel_program != m_volume_rtc_kernel.kernel_program)
  {
    m_volume_rtc_kernel.kernel_program = kernel_program;
    kernel_changed = true;
  }

  if (kernel_changed)
  {
    m_rtc_kernel_changed = true;
  }

  m_volume_rtc_kernel.params_buffer = params_buffer;
  m_volume_rtc_kernel.buffer_size = buffer_size;
  m_rtc_param_changed = true;

  return kernel_changed;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_volumemapper::Render(vtkRenderer* ren, vtkVolume* vol)
{
  if (!m_cluster_properties->is_active_instance())
  {
    return;
  }

  // Ensure IceT is enabled in MPI mode, print warning only on first rank
  if (m_controller->GetNumberOfProcesses() > 1 && m_controller->GetLocalProcessId() == 0)
  {
    static bool IceT_was_enabled = false;
    if (!vtkPVRenderViewSettings::GetInstance()->GetDisableIceT())
    {
      WARN_LOG << "IceT compositing must be disabled when using the NVIDIA IndeX plugin with MPI, "
                  "otherwise nothing will be rendered. "
               << "Please open 'Edit | Settings', go to the 'Render View' tab, activate 'Advanced "
                  "Properties' (gear icon) and check 'Disable IceT'. "
               << "Then restart ParaView for the setting to take effect.";
      IceT_was_enabled = true;
    }
    else if (IceT_was_enabled)
    {
      WARN_LOG << "Please restart ParaView so that IceT compositing is fully disabled.";
      IceT_was_enabled = false; // only print this once
    }
  }

  // check if volume data was modified
  if (!m_cluster_properties->get_regular_volume_properties()->is_timeseries_data())
  {
    mi::Sint32 cell_flag;
    vtkDataArray* scalar_array = this->GetScalars(this->GetInput(), this->ScalarMode,
      this->ArrayAccessMode, this->ArrayId, this->ArrayName, cell_flag);

    vtkMTimeType cur_MTime = scalar_array->GetMTime();

    if (m_last_MTime == 0)
    {
      m_last_MTime = cur_MTime;
    }
    else if (m_last_MTime != cur_MTime)
    {
      m_last_MTime = cur_MTime;
      m_volume_changed = true;
    }
  }

  // check for changed volume properties
  vtkVolumeProperty* property = vol->GetProperty();
  const std::string cur_array_name = this->GetArrayName();
  if (cur_array_name != m_prev_array_name || this->VectorComponent != m_prev_vector_component ||
    this->VectorMode != m_prev_vector_mode)
  {
    m_volume_changed = true;
    m_prev_array_name = cur_array_name;
    m_prev_vector_component = this->VectorComponent;
    m_prev_vector_mode = this->VectorMode;
  }

  static bool independent_components_warning_printed = false;
  if (!property->GetIndependentComponents() && !independent_components_warning_printed)
  {
    ERROR_LOG << "Setting 'Map Scalars' (IndependentComponents) to 'off' is currently not "
              << "supported by the NVIDIA IndeX plugin. "
              << "This warning will only be printed once.";
    independent_components_warning_printed = true;
  }

  // Initialize the mapper
  if ((!m_is_mapper_initialized || m_volume_changed) && !initialize_mapper(vol))
  {
    ERROR_LOG << "Failed to initialize the mapper in "
              << "vtknvindex_volumemapper::Render().";
    ERROR_LOG << "NVIDIA IndeX rendering was aborted.";
    return;
  }

  // Prepare data to be rendered

  mi::Sint32 cur_time_step =
    (m_cluster_properties->get_regular_volume_properties()->is_timeseries_data()
        ? m_cluster_properties->get_regular_volume_properties()->get_current_time_step()
        : 0);

  bool needs_activate = m_cluster_properties->activate();
  if (needs_activate)
  {
    // Importers might be triggered again, so data must be made available
    m_time_step_data_prepared.clear();
  }

  if (!is_data_prepared(cur_time_step) || m_volume_changed)
  {
    if (!prepare_data(cur_time_step))
    {
      ERROR_LOG << "Failed to prepare data in "
                << "vtknvindex_volumemapper::Render().";
      ERROR_LOG << "NVIDIA IndeX rendering was aborted.";
      return;
    }

    if (m_is_mpi_rendering)
    {
      // Ensure border data is available, fetching it from other hosts if necessary
      vtknvindex_host_properties* host_props =
        m_cluster_properties->get_host_properties(m_controller->GetLocalProcessId());

      const void* piece_data = m_subset_ptrs[cur_time_step];

      host_props->fetch_remote_volume_border_data(m_controller, cur_time_step, piece_data,
        m_cluster_properties->get_regular_volume_properties()->get_scalar_type());
    }
  }

  if (m_is_mpi_rendering)
  {
    // Wait for all ranks to finish writing volume data before the render starts.
    m_controller->Barrier();
  }

  if (m_index_instance->is_index_viewer() && m_index_instance->ensure_index_initialized())
  {
    vtkTimerLog::MarkStartEvent("NVIDIA-IndeX: Rendering");
    bool skip_rendering = false;

    {
      // DiCE database access
      mi::base::Handle<mi::neuraylib::IDice_transaction> dice_transaction(
        m_index_instance->m_global_scope->create_transaction<mi::neuraylib::IDice_transaction>());
      assert(dice_transaction.is_valid_interface());

      // Setup scene information
      if (!m_scene.scene_created())
      {
        m_scene.create_scene(ren, vol, dice_transaction, vtknvindex_scene::VOLUME_TYPE_REGULAR);
        needs_activate = true;
      }
      else if (m_volume_changed)
      {
        m_scene.update_volume(dice_transaction, vtknvindex_scene::VOLUME_TYPE_REGULAR);
        needs_activate = true;
      }

      if (needs_activate)
      {
        m_cluster_properties->warn_if_multiple_visible_instances(this->GetArrayName());
        m_scene.activate(dice_transaction.get());
        m_config_settings_changed = true; // force setting region-of-interest
      }

      {
        // Handle cropping
        const mi::math::Bbox<mi::Float32, 3> whole_bounds(m_whole_bounds[0], m_whole_bounds[2],
          m_whole_bounds[4], m_whole_bounds[1], m_whole_bounds[3], m_whole_bounds[5]);

        mi::math::Bbox<mi::Sint32, 3> volume_extents;
        m_cluster_properties->get_regular_volume_properties()->get_volume_extents(volume_extents);
        const mi::math::Vector<mi::Float32, 3> volume_size(
          volume_extents.max - volume_extents.min); // in voxels

        mi::math::Bbox<mi::Float32, 3> roi;
        if (this->Cropping)
        {
          roi = mi::math::Bbox<mi::Float32, 3>(this->CroppingRegionPlanes[0],
            this->CroppingRegionPlanes[2], this->CroppingRegionPlanes[4],
            this->CroppingRegionPlanes[1], this->CroppingRegionPlanes[3],
            this->CroppingRegionPlanes[5]);

          roi = mi::math::clip(roi, whole_bounds); // Limit to bounds of the volume

          // Translate to origin
          roi.min -= whole_bounds.min;
          roi.max -= whole_bounds.min;

          // Transform to volume size in voxels
          roi.min = (roi.min / (whole_bounds.max - whole_bounds.min)) * volume_size;
          roi.max = (roi.max / (whole_bounds.max - whole_bounds.min)) * volume_size;
        }
        else if (!m_region_of_interest_deprecated.empty())
        {
          // Backward-compatibility: Apply deprecated "region of interest" if set
          roi = mi::math::Bbox<mi::Float32, 3>(volume_size * m_region_of_interest_deprecated.min,
            volume_size * m_region_of_interest_deprecated.max);

          if (!m_region_of_interest_deprecated_warning_printed)
          {
            const mi::math::Vector<mi::Float32, 3> scale(
              m_region_of_interest_deprecated.max - m_region_of_interest_deprecated.min);
            const mi::math::Vector<mi::Float32, 3> origin(
              (whole_bounds.min - whole_bounds.min * scale) +
              (whole_bounds.max - whole_bounds.min) * m_region_of_interest_deprecated.min);

            WARN_LOG << "The 'NVIDIA IndeX Region of Interest' setting is deprecated, "
                     << "please use 'Cropping' instead (origin: " << origin << ", scale: " << scale
                     << ").";
            m_region_of_interest_deprecated_warning_printed = true;
          }
        }
        else
        {
          // No cropping, show entire volume
          roi = mi::math::Bbox<mi::Float32, 3>(mi::math::Vector<mi::Float32, 3>(0.f), volume_size);
        }

        if (roi.is_volume())
        {
          m_cluster_properties->get_config_settings()->set_region_of_interest(roi);
        }
        else
        {
          // Nothing to render
          skip_rendering = true;
        }
      }

      // Update scene parameters
      m_scene.update_scene(
        ren, vol, dice_transaction, m_config_settings_changed, m_opacity_changed, m_slices_changed);
      m_config_settings_changed = false;
      m_opacity_changed = false;
      m_slices_changed = false;

      // Update CUDA code.
      if (m_rtc_kernel_changed || m_rtc_param_changed)
      {
        m_scene.update_rtc_kernel(dice_transaction, m_volume_rtc_kernel,
          vtknvindex_scene::VOLUME_TYPE_REGULAR, m_rtc_kernel_changed);
        m_rtc_kernel_changed = false;
        m_rtc_param_changed = false;
      }

      // Update render canvas
      update_canvas(ren);

      // commit DiCE transaction
      dice_transaction->commit();
    }

    // Render the scene
    if (!skip_rendering)
    {
      // DiCE database access
      mi::base::Handle<mi::neuraylib::IDice_transaction> dice_transaction(
        m_index_instance->m_global_scope->create_transaction<mi::neuraylib::IDice_transaction>());
      assert(dice_transaction.is_valid_interface());

      {
        // Access the session instance from the database.
        mi::base::Handle<const nv::index::ISession> session(
          dice_transaction->access<const nv::index::ISession>(m_index_instance->m_session_tag));
        assert(session.is_valid_interface());

        // Access the scene instance from the database.
        mi::base::Handle<const nv::index::IScene> scene(
          dice_transaction->access<const nv::index::IScene>(session->get_scene()));
        assert(scene.is_valid_interface());

        // Synchronize and update the IndeX session with the
        // scene (volume data, transformation matrix, camera, and so on.)
        m_index_instance->m_iindex_session->update(
          m_index_instance->m_session_tag, dice_transaction.get());

        // Use ParaView depth buffer, if present.
        const bool use_depth_buffer =
          (ren->GetNumberOfPropsRendered() > 0 && m_index_instance->m_opengl_app_buffer.is_valid());

        // NVIDIA IndeX render call returns frame results
        mi::base::Handle<nv::index::IFrame_results> frame_results(
          m_index_instance->m_iindex_rendering->render(m_index_instance->m_session_tag,
            &(m_index_instance->m_opengl_canvas), // Opengl canvas.
            dice_transaction.get(),
            nullptr, // No progress_callback.
            nullptr, // No Frame information.
            true,    // = g_immediate_final_parallel_compositing
            use_depth_buffer ? &m_index_instance->m_opengl_app_buffer : nullptr));

        // check for errors during rendering
        const mi::base::Handle<nv::index::IError_set> err_set(frame_results->get_error_set());
        if (err_set->any_errors())
        {
          std::ostringstream os;
          const mi::Uint32 nb_err = err_set->get_nb_errors();
          for (mi::Uint32 e = 0; e < nb_err; ++e)
          {
            if (e != 0)
              os << '\n';
            const mi::base::Handle<nv::index::IError> err(err_set->get_error(e));
            os << err->get_error_string();
          }
          ERROR_LOG << "The NVIDIA IndeX rendering call failed with the following error(s): "
                    << '\n'
                    << os.str();
        }

        // log performance values if requested
        if (vtknvindex_global_settings::GetInstance()->GetOutputPerformanceValues())
          m_performance_values.print_perf_values(frame_results, cur_time_step);
      }

      dice_transaction->commit();
    }

    vtkTimerLog::MarkEndEvent("NVIDIA-IndeX: Rendering");
  }

  m_volume_changed = false;

  if (m_is_mpi_rendering)
  {
    m_controller->Barrier();
  }

  // clean shared memory
  m_cluster_properties->unlink_shared_memory(false);
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_volumemapper::is_data_prepared(mi::Sint32 time_step)
{
  std::map<mi::Sint32, bool>::iterator it = m_time_step_data_prepared.find(time_step);

  if (it == m_time_step_data_prepared.end())
  {
    m_time_step_data_prepared.insert(std::pair<mi::Sint32, bool>(time_step, false));
    return false;
  }
  else
  {
    return it->second;
  }
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_volumemapper::is_caching(bool is_caching)
{
  m_is_caching = is_caching;
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_volumemapper::is_caching() const
{
  return m_is_caching;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_volumemapper::set_visibility(bool visibility)
{
  m_cluster_properties->set_visibility(visibility);
}

//-------------------------------------------------------------------------------------------------
vtkDataArray* vtknvindex_volumemapper::convert_scalar_array(vtkDataArray* scalar_array) const
{
  vtkDataArray* converted_array = nullptr;

  if (scalar_array->GetNumberOfComponents() > 1)
  {
    // Extract only the selected component from the vector data
    converted_array = scalar_array->NewInstance();
    converted_array->SetNumberOfComponents(1);
    converted_array->SetNumberOfTuples(scalar_array->GetNumberOfTuples());
    if (this->VectorMode == vtkColorTransferFunction::MAGNITUDE)
    {
      // Calculate vector magnitude
      for (vtkIdType t = 0; t < scalar_array->GetNumberOfTuples(); ++t)
      {
        converted_array->SetTuple1(t, vtkMath::Norm(scalar_array->GetTuple3(t)));
      }
    }
    else
    {
      // Copy the requested vector component
      converted_array->CopyComponent(0, scalar_array, this->VectorComponent);
    }
  }

  return converted_array;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_volumemapper::set_region_of_interest_deprecated(
  const mi::math::Bbox<mi::Float32, 3>& roi)
{
  if (m_region_of_interest_deprecated.empty())
  {
    const mi::math::Bbox<mi::Float32, 3> unit_bbox(0.f, 0.f, 0.f, 1.f, 1.f, 1.f);
    if (roi.empty() || roi == unit_bbox)
    {
      // Nothing will be cropped
      return;
    }
  }

  m_region_of_interest_deprecated = roi;
}
