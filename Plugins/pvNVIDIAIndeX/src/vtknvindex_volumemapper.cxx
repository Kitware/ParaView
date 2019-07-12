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
#include <cstdio>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

#if defined(__APPLE__)
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#include "vtkCellData.h"
#include "vtkCommand.h"

#include "vtkFixedPointVolumeRayCastMapper.h"
#include "vtkImageData.h"

#include "vtkDataArray.h"
#include "vtkMatrix4x4.h"
#include "vtkMultiThreader.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
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
#include "vtknvindex_instance.h"
#include "vtknvindex_utilities.h"
#include "vtknvindex_volumemapper.h"

vtkStandardNewMacro(vtknvindex_volumemapper);

//----------------------------------------------------------------------------
vtknvindex_volumemapper::vtknvindex_volumemapper()
  : m_is_caching(false)
  , m_is_mapper_intialized(false)
  , m_config_settings_changed(false)
  , m_opacity_changed(false)
  , m_slices_changed(false)
  , m_volume_changed(false)
  , m_rtc_kernel_changed(false)
  , m_rtc_param_changed(false)

{
  m_index_instance = vtknvindex_instance::get();
  m_controller = vtkMultiProcessController::GetGlobalController();

  for (mi::Uint32 i = 0; i < 6; i++)
    m_whole_bounds[i] = 0.0;

  m_prev_property = "";
  m_last_MTime = 0;
}

//----------------------------------------------------------------------------
vtknvindex_volumemapper::~vtknvindex_volumemapper()
{
  // empty
}

//----------------------------------------------------------------------------
double* vtknvindex_volumemapper::GetBounds()
{
  // Return the whole volume bounds instead of the local one to avoid that
  // ParaView culls off any rank

  return m_whole_bounds;
}

//----------------------------------------------------------------------------
void vtknvindex_volumemapper::set_whole_bounds(const mi::math::Bbox<mi::Float64, 3> bounds)
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
bool vtknvindex_volumemapper::prepare_data(mi::Sint32 time_step, vtkVolume* /*vol*/)
{
  vtkTimerLog::MarkStartEvent("NVIDIA-IndeX: Preparing data");

  int extent[6];
  this->GetInput()->GetExtent(extent);

  // Get ParaView's volume data
  vtkDataArray* scalar_array = m_scalar_array;
  if (m_cluster_properties->get_regular_volume_properties()->is_timeseries_data())
  {
    // Get the input
    vtkImageData* image_piece = this->GetInput();
    if (image_piece == NULL)
    {
      ERROR_LOG << "vtkImageData in representation is invalid!";
      return false;
    }

    mi::Sint32 use_cell_colors;
    scalar_array = this->GetScalars(image_piece, this->ScalarMode, this->ArrayAccessMode,
      this->ArrayId, this->ArrayName,
      use_cell_colors); // CellFlag

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
bool vtknvindex_volumemapper::initialize_mapper(vtkRenderer* /*ren*/, vtkVolume* vol)
{
  vtkTimerLog::MarkStartEvent("NVIDIA-IndeX: Initialization");

  bool is_MPI = (m_controller->GetNumberOfProcesses() > 1);
  const mi::Sint32 cur_global_rank = is_MPI ? m_controller->GetLocalProcessId() : 0;

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

  // Update in_volume first to make sure states are current.
  vol->Update();

  // Get the input.
  vtkImageData* image_piece = this->GetInput();
  if (image_piece == NULL)
  {
    ERROR_LOG << "vtkImageData in representation is invalid.";
    return false;
  }

  mi::Sint32 use_cell_colors;
  m_scalar_array = this->GetScalars(image_piece, this->ScalarMode, this->ArrayAccessMode,
    this->ArrayId, this->ArrayName,
    use_cell_colors); // CellFlag

  // check for scalar per cell values
  if (use_cell_colors)
  {
    ERROR_LOG << "Scalar values per cell are not supported in NVIDIA IndeX.";
    return false;
  }

  // check for valid data types
  const std::string scalar_type = m_scalar_array->GetDataTypeAsString();
  if (scalar_type != "unsigned char" && scalar_type != "unsigned short" && scalar_type != "char" &&
    scalar_type != "short" && scalar_type != "float" && scalar_type != "double")
  {
    ERROR_LOG << "The scalar type: " << scalar_type << " is not supported by NVIDIA IndeX.";
    return false;
  }
  else if (scalar_type == "double")
  {
    WARN_LOG
      << "Datasets with scalar values in double precision are not natively supported by IndeX.";
    WARN_LOG << "The plug-in will proceed to convert those values from double to float with the "
                "corresponding overhead.";
  }

  m_subset_ptrs[0] = m_scalar_array->GetVoidPointer(0);

  vtknvindex_regular_volume_data volume_data;
  volume_data.scalars = &m_subset_ptrs[0];

  int extent[6];
  image_piece->GetExtent(extent);

  vtknvindex_dataset_parameters dataset_parameters;
  dataset_parameters.volume_type =
    vtknvindex_scene::VOLUME_TYPE_REGULAR; // vtknviVTKNVINDEX_VOLUME_TYPE_REGULAR;
  dataset_parameters.scalar_type = scalar_type;
  dataset_parameters.voxel_range[0] = static_cast<mi::Float32>(
    m_scalar_array->GetRange(0)[0]); // '0' is component ID TODO: do this in a clean way
  dataset_parameters.voxel_range[1] = static_cast<mi::Float32>(
    m_scalar_array->GetRange(0)[1]); // '0' is component ID TODO: do this in a clean way
  dataset_parameters.scalar_range[0] = static_cast<mi::Float32>(m_scalar_array->GetDataTypeMin());
  dataset_parameters.scalar_range[1] = static_cast<mi::Float32>(m_scalar_array->GetDataTypeMax());

  dataset_parameters.bounds[0] = extent[0];
  dataset_parameters.bounds[1] = extent[1];
  dataset_parameters.bounds[2] = extent[2];
  dataset_parameters.bounds[3] = extent[3];
  dataset_parameters.bounds[4] = extent[4];
  dataset_parameters.bounds[5] = extent[5];

  dataset_parameters.volume_data = static_cast<void*>(&volume_data);

  // clean shared memory
  m_cluster_properties->unlink_shared_memory(true);

  // Collect dataset type, ranges, bounding boxes, scalar values and affinity to be passed to NVIDIA
  // IndeX.
  if (is_MPI)
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

  m_is_mapper_intialized = true;

  m_controller->Barrier();

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
    m_index_instance->m_opengl_app_buffer.resize_buffer(main_window_resolution);

    vtkOpenGLRenderWindow* vtk_gl_render_window = vtkOpenGLRenderWindow::SafeDownCast(win);
    mi::Sint32 depth_bits = vtk_gl_render_window->GetDepthBufferSize();
    m_index_instance->m_opengl_app_buffer.set_z_buffer_precision(depth_bits);

    mi::Uint32* pv_z_buffer = m_index_instance->m_opengl_app_buffer.get_z_buffer_ptr();
    glReadPixels(
      0, 0, window_size[0], window_size[1], GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, pv_z_buffer);
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
void vtknvindex_volumemapper::rtc_kernel_changed(
  vtknvindex_rtc_kernels kernel, const void* params_buffer, mi::Uint32 buffer_size)
{
  if (kernel != m_volume_rtc_kernel.rtc_kernel)
  {
    m_volume_rtc_kernel.rtc_kernel = kernel;
    m_rtc_kernel_changed = true;
  }

  m_volume_rtc_kernel.params_buffer = params_buffer;
  m_volume_rtc_kernel.buffer_size = buffer_size;
  m_rtc_param_changed = true;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_volumemapper::Render(vtkRenderer* ren, vtkVolume* vol)
{
  // check if volume data was modified
  if (!m_cluster_properties->get_regular_volume_properties()->is_timeseries_data())
  {
    mi::Sint32 use_cell_colors;
    vtkDataArray* scalar_array = this->GetScalars(this->GetInput(), this->ScalarMode,
      this->ArrayAccessMode, this->ArrayId, this->ArrayName, use_cell_colors);

    vtkMTimeType cur_MTime = scalar_array->GetMTime();

    if (m_last_MTime == 0)
    {
      m_last_MTime = cur_MTime;
    }
    else if (m_last_MTime < cur_MTime)
    {
      m_last_MTime = cur_MTime;
      m_volume_changed = true;
    }
  }

  // check for volume property changed
  std::string cur_property(this->GetArrayName());
  if (cur_property != m_prev_property)
  {
    m_volume_changed = true;
    m_prev_property = cur_property;
  }

  // Initialize the mapper
  if ((!m_is_mapper_intialized || m_volume_changed) && !initialize_mapper(ren, vol))
  {
    ERROR_LOG << "Failed to initialize the mapper in "
              << "vtknvindex_volumemapper::Render().";
    ERROR_LOG << "NVIDIA IndeX rendering was aborted.";
    return;
  }

  // Prepare data to be rendered

  mi::Sint32 cur_time_step =
    m_cluster_properties->get_regular_volume_properties()->get_current_time_step();

  if ((!is_data_prepared(cur_time_step) || m_volume_changed) && !prepare_data(cur_time_step, vol))
  {
    ERROR_LOG << "Failed to prepare data in "
              << "vtknvindex_volumemapper::Render().";
    ERROR_LOG << "NVIDIA IndeX rendering was aborted.";
    return;
  }

  // Wait all ranks finish to write volume data before the render starts.
  m_controller->Barrier();

  if (m_index_instance->is_index_viewer() && m_index_instance->is_index_initialized())
  {
    vtkTimerLog::MarkStartEvent("NVIDIA-IndeX: Rendering");

    {
      // DiCE database access
      mi::base::Handle<mi::neuraylib::IDice_transaction> dice_transaction(
        m_index_instance->m_global_scope->create_transaction<mi::neuraylib::IDice_transaction>());
      assert(dice_transaction.is_valid_interface());

      // Setup scene information
      if (!m_scene.scene_created())
        m_scene.create_scene(ren, vol, dice_transaction, vtknvindex_scene::VOLUME_TYPE_REGULAR);
      else if (m_volume_changed)
        m_scene.update_volume(dice_transaction, vtknvindex_scene::VOLUME_TYPE_REGULAR);

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

        // NVIDIA IndeX render call returns frame results
        mi::base::Handle<nv::index::IFrame_results> frame_results(
          m_index_instance->m_iindex_rendering->render(m_index_instance->m_session_tag,
            &(m_index_instance->m_opengl_canvas), // Opengl canvas.
            dice_transaction.get(),
            0,    // No progress_callback.
            0,    // No Frame information.
            true, // = g_immediate_final_parallel_compositing
            ren->GetNumberOfPropsRendered() ? &(m_index_instance->m_opengl_app_buffer)
                                            : NULL) // ParaView depth buffer, if present.
          );

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
        if (m_cluster_properties->get_config_settings()->is_log_performance())
          m_performance_values.print_perf_values(frame_results, cur_time_step);
      }

      dice_transaction->commit();
    }

    vtkTimerLog::MarkEndEvent("NVIDIA-IndeX: Rendering");
  }

  m_volume_changed = false;

  // clean shared memory
  m_controller->Barrier();
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
  m_scene.set_visibility(visibility);
}
