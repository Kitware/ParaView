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

#if defined(__APPLE__)
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkIntArray.h"
#include "vtkMultiThreader.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPVRenderViewSettings.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include <nv/index/ilight.h>
#include <nv/index/imaterial.h>

#include "vtknvindex_cluster_properties.h"
#include "vtknvindex_forwarding_logger.h"
#include "vtknvindex_instance.h"
#include "vtknvindex_irregular_volume_mapper.h"
#include "vtknvindex_utilities.h"

static const int TET_EDGES[6][2] = { { 0, 1 }, { 1, 2 }, { 2, 0 }, { 0, 3 }, { 1, 3 }, { 2, 3 } };

vtkStandardNewMacro(vtknvindex_irregular_volume_mapper);

//----------------------------------------------------------------------------
vtknvindex_irregular_volume_mapper::vtknvindex_irregular_volume_mapper()
  : m_is_mapper_initialized(false)
  , m_is_data_prepared(false)
  , m_config_settings_changed(false)
  , m_opacity_changed(false)
  , m_volume_changed(false)
  , m_rtc_kernel_changed(false)
  , m_rtc_param_changed(false)

{
  m_index_instance = vtknvindex_instance::get();
  m_controller = vtkMultiProcessController::GetGlobalController();
  m_prev_property = "";
  m_last_MTime = 0;

  vtkMath::UninitializeBounds(m_whole_bounds);
  vtkMath::UninitializeBounds(m_subregion_bounds);
}

//----------------------------------------------------------------------------
vtknvindex_irregular_volume_mapper::~vtknvindex_irregular_volume_mapper()
{
  // empty
}

//----------------------------------------------------------------------------
double* vtknvindex_irregular_volume_mapper::GetBounds()
{
  // Return the whole volume bounds instead of the local one to avoid that
  // ParaView culls off any rank

  return m_whole_bounds;
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_mapper::set_whole_bounds(
  const mi::math::Bbox<mi::Float64, 3> bounds)
{
  m_whole_bounds[0] = bounds.min.x;
  m_whole_bounds[1] = bounds.max.x;
  m_whole_bounds[2] = bounds.min.y;
  m_whole_bounds[3] = bounds.max.y;
  m_whole_bounds[4] = bounds.min.z;
  m_whole_bounds[5] = bounds.max.z;
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_mapper::shutdown()
{
  // TODO: Delete volume from scene graph
}

//----------------------------------------------------------------------------
void vtknvindex_irregular_volume_mapper::PrintSelf(ostream& os, vtkIndent indent)
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
bool vtknvindex_irregular_volume_mapper::prepare_data()
{
  vtkTimerLog::MarkStartEvent("NVIDIA-IndeX: Preparing data");

  if (!m_index_instance->is_index_rank())
  {
    if (!m_cluster_properties->get_regular_volume_properties()->write_shared_memory(&m_volume_data,
          this->GetInput(),
          m_cluster_properties->get_host_properties(m_controller->GetLocalProcessId()),
          0)) // time step
    {
      ERROR_LOG << "Failed to write vtkImageData piece into shared memory"
                << " in vtknvindex_representation::RequestData().";
      return false;
    }
  }

  m_is_data_prepared = true;

  vtkTimerLog::MarkEndEvent("NVIDIA-IndeX: Preparing data");

  return true;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_irregular_volume_mapper::volume_changed()
{
  m_volume_changed = true;
}

//-------------------------------------------------------------------------------------------------
bool vtknvindex_irregular_volume_mapper::initialize_mapper(vtkRenderer* /*ren*/, vtkVolume* vol)
{
  vtkTimerLog::MarkStartEvent("NVIDIA-IndeX: Initialization");

  // Initialize and start IndeX (if not initialized yet and if this is an IndeX rank)
  m_index_instance->init_index();

  const bool is_MPI = (m_controller->GetNumberOfProcesses() > 1);
  const mi::Sint32 cur_global_rank = is_MPI ? m_controller->GetLocalProcessId() : 0;

  // Update volume first to make sure that the states are current.
  vol->Update();

  // Get the unstructured_grid.
  vtkUnstructuredGridBase* unstructured_grid = this->GetInput();

  mi::Sint32 cell_flag;
  m_scalar_array = this->GetScalars(unstructured_grid, this->ScalarMode, this->ArrayAccessMode,
    this->ArrayId, this->ArrayName, cell_flag);

  bool is_data_supported = true;

  // Data can only have a single component (not RGB/HSV format)
  const int components = m_scalar_array->GetNumberOfComponents();
  if (components > 1)
  {
    ERROR_LOG << "The data array '" << this->ArrayName << "' has " << components << " components, "
              << "which is not supported by NVIDIA IndeX.";
    is_data_supported = false;
  }

  // Only per point scalars and cell scalars are allowed
  if (cell_flag != 0 && cell_flag != 1)
  {
    ERROR_LOG << "The data array '" << this->ArrayName << "' uses per field scalars, but "
              << "only per point and per cell scalars are supported by NVIDIA IndeX (cellFlag is "
              << cell_flag << ").";
    is_data_supported = false;
  }

  // Check for valid data types
  const std::string scalar_type = m_scalar_array->GetDataTypeAsString();
  if (scalar_type != "unsigned char" && scalar_type != "unsigned short" && scalar_type != "float" &&
    scalar_type != "double")
  {
    ERROR_LOG << "The data array '" << this->ArrayName << "' uses the scalar type '" << scalar_type
              << "', which is not supported for unstructured grids by NVIDIA IndeX.";
    is_data_supported = false;
  }
  else if (scalar_type == "double" && is_data_supported)
  {
    // Only print the warning once per data array, and do not repeat when switching between arrays.
    if (m_data_array_warning_printed.find(this->ArrayName) == m_data_array_warning_printed.end())
    {
      WARN_LOG << "The data array '" << this->ArrayName << "' has scalar values "
               << "in double precision format, which is not natively supported by NVIDIA IndeX. "
               << "The plugin will proceed to convert the values from double to float with the "
               << "corresponding overhead.";
      m_data_array_warning_printed.emplace(this->ArrayName);
    }
  }

  if (!is_data_supported)
  {
    return false;
  }

  if (true) //   (this->InputAnalyzedTime < this->MTime) || (this->InputAnalyzedTime <
            //   unstructured_grid->GetMTime()) )
  {
    if (unstructured_grid->GetNumberOfCells() == 0)
    {
      // Apparently, the unstructured_grid has no cells.  Just do nothing.
      return true;
    }

    // subset subregion
    // get geometry bounds
    const mi::Float64* bounds = unstructured_grid->GetBounds();

    // get subset bounds
    if (vtkMath::AreBoundsInitialized(m_subregion_bounds))
    {
      m_volume_data.subregion_id = cur_global_rank;

      m_volume_data.subregion_bbox.min.x = static_cast<mi::Float32>(m_subregion_bounds[0]);
      m_volume_data.subregion_bbox.min.y = static_cast<mi::Float32>(m_subregion_bounds[2]);
      m_volume_data.subregion_bbox.min.z = static_cast<mi::Float32>(m_subregion_bounds[4]);

      m_volume_data.subregion_bbox.max.x = static_cast<mi::Float32>(m_subregion_bounds[1]);
      m_volume_data.subregion_bbox.max.y = static_cast<mi::Float32>(m_subregion_bounds[3]);
      m_volume_data.subregion_bbox.max.z = static_cast<mi::Float32>(m_subregion_bounds[5]);
    }
    else
    {
      m_volume_data.subregion_id = 0;

      m_volume_data.subregion_bbox.min.x = static_cast<mi::Float32>(bounds[0]);
      m_volume_data.subregion_bbox.min.y = static_cast<mi::Float32>(bounds[2]);
      m_volume_data.subregion_bbox.min.z = static_cast<mi::Float32>(bounds[4]);

      m_volume_data.subregion_bbox.max.x = static_cast<mi::Float32>(bounds[1]);
      m_volume_data.subregion_bbox.max.y = static_cast<mi::Float32>(bounds[3]);
      m_volume_data.subregion_bbox.max.z = static_cast<mi::Float32>(bounds[5]);
    }

    // Tetrahedral mesh points.
    m_volume_data.num_points = static_cast<mi::Uint32>(unstructured_grid->GetNumberOfPoints());

    // Tetrahedral mesh cells.
    bool gave_error = 0;
    mi::Float64 max_cell_edge_size2 = 0.0;
    mi::Float64 min_cell_edge_size2 = std::numeric_limits<double>::max();
    mi::Float64 sum_inv_cell_edge_size2 = 0.0;
    mi::Uint64 num_cells = 0ull;

    m_volume_data.num_cells = 0;

    vtkSmartPointer<vtkCellIterator> cellIter =
      vtkSmartPointer<vtkCellIterator>::Take(unstructured_grid->NewCellIterator());
    for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
    {
      const vtkIdType npts = cellIter->GetNumberOfPoints();
      if (npts != 4)
      {
        if (!gave_error)
        {
          ERROR_LOG << "Encountered non-tetrahedral cell with " << npts
                    << " points. The NVIDIA IndeX plugin currently "
                       "supports tetrahedral cells only.";
          gave_error = true;
        }
        continue;
      }

      const vtkIdType* cell_point_ids = cellIter->GetPointIds()->GetPointer(0);

      m_volume_data.num_cells++;

      mi::Float64 avg_edge_size2 = 0.0;
      for (mi::Uint32 i = 0; i < 6; i++)
      {
        mi::Float64 p1[3], p2[3];
        unstructured_grid->GetPoint(cell_point_ids[TET_EDGES[i][0]], p1);
        unstructured_grid->GetPoint(cell_point_ids[TET_EDGES[i][1]], p2);
        mi::Float64 size2 = vtkMath::Distance2BetweenPoints(p1, p2);

        if (size2 > max_cell_edge_size2)
          max_cell_edge_size2 = size2;

        if (size2 < min_cell_edge_size2)
          min_cell_edge_size2 = size2;

        avg_edge_size2 += size2;
      }

      sum_inv_cell_edge_size2 += 1.0 / sqrt(avg_edge_size2 / 6.0);
      num_cells++;
    }

    m_volume_data.max_edge_length2 = static_cast<mi::Float32>(max_cell_edge_size2);

    if (is_MPI)
    {
      sum_inv_cell_edge_size2 = static_cast<mi::Float64>(num_cells) / sum_inv_cell_edge_size2;

      mi::Float64 avg_edge = 0.0;
      m_controller->Reduce(&sum_inv_cell_edge_size2, &avg_edge, 1, vtkCommunicator::SUM_OP, 0);

      if (m_index_instance->is_index_viewer())
      {
        avg_edge /= static_cast<mi::Float64>(m_controller->GetNumberOfProcesses());
        m_cluster_properties->get_config_settings()->set_ivol_step_size(
          static_cast<mi::Float32>(avg_edge * 0.1));
      }
    }
    else
    {
      sum_inv_cell_edge_size2 = static_cast<mi::Float64>(num_cells) / sum_inv_cell_edge_size2;
      m_cluster_properties->get_config_settings()->set_ivol_step_size(
        static_cast<mi::Float32>(sum_inv_cell_edge_size2 * 0.1));
    }

    // scalars
    m_volume_data.scalars = m_scalar_array->GetVoidPointer(0);
    m_volume_data.cell_flag = cell_flag;
    m_volume_data.num_scalars =
      (cell_flag == 1 ? m_volume_data.num_cells : m_volume_data.num_points);

    if (m_index_instance->is_index_rank())
      m_volume_data.pv_unstructured_grid = unstructured_grid;
    else
      m_volume_data.pv_unstructured_grid = nullptr;

    // fill dataset parameters struct.
    vtknvindex_dataset_parameters dataset_parameters;
    dataset_parameters.volume_type = vtknvindex_scene::VOLUME_TYPE_IRREGULAR;
    dataset_parameters.scalar_type = scalar_type;
    dataset_parameters.voxel_range[0] = static_cast<mi::Float32>(m_scalar_array->GetRange(0)[0]);
    dataset_parameters.voxel_range[1] = static_cast<mi::Float32>(m_scalar_array->GetRange(0)[1]);
    dataset_parameters.scalar_range[0] = static_cast<mi::Float32>(m_scalar_array->GetRange()[0]);
    dataset_parameters.scalar_range[1] = static_cast<mi::Float32>(m_scalar_array->GetRange()[1]);

    dataset_parameters.bounds[0] = m_volume_data.subregion_bbox.min.x;
    dataset_parameters.bounds[1] = m_volume_data.subregion_bbox.max.x;
    dataset_parameters.bounds[2] = m_volume_data.subregion_bbox.min.y;
    dataset_parameters.bounds[3] = m_volume_data.subregion_bbox.max.y;
    dataset_parameters.bounds[4] = m_volume_data.subregion_bbox.min.z;
    dataset_parameters.bounds[5] = m_volume_data.subregion_bbox.max.z;

    dataset_parameters.volume_data = &m_volume_data;

    // clean shared memory
    m_cluster_properties->unlink_shared_memory(true);

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

    m_is_mapper_initialized = true;
  }

  m_controller->Barrier();

  vtkTimerLog::MarkEndEvent("NVIDIA-IndeX: Initialization");

  return true;
}

//-------------------------------------------------------------------------------------------------
mi::Sint32 vtknvindex_irregular_volume_mapper::get_local_hostid()
{
  return m_index_instance->m_icluster_configuration->get_local_host_id();
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_irregular_volume_mapper::set_cluster_properties(
  vtknvindex_cluster_properties* cluster_properties)
{
  m_cluster_properties = cluster_properties;
  m_scene.set_cluster_properties(cluster_properties);
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_irregular_volume_mapper::set_raw_cuts(
  const std::vector<vtkBoundingBox>& raw_cuts, const std::vector<int>& ranks)
{
  m_raw_cuts = raw_cuts;
  m_raw_cuts_ranks = ranks;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_irregular_volume_mapper::set_subregion_bounds(const vtkBoundingBox& bbox)
{
  if (bbox.IsValid())
  {
    bbox.GetBounds(m_subregion_bounds);
  }
  else
  {
    vtkMath::UninitializeBounds(m_subregion_bounds);
  }
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_irregular_volume_mapper::update_canvas(vtkRenderer* ren)
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

    vtkOpenGLRenderWindow* vtk_gl_render_window =
      vtkOpenGLRenderWindow::SafeDownCast(ren->GetVTKWindow());
    mi::Sint32 depth_bits = vtk_gl_render_window->GetDepthBufferSize();
    m_index_instance->m_opengl_app_buffer.set_z_buffer_precision(depth_bits);

    mi::Uint32* pv_z_buffer = m_index_instance->m_opengl_app_buffer.get_z_buffer_ptr();
    glReadPixels(
      0, 0, window_size[0], window_size[1], GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, pv_z_buffer);
  }

  reset_orthogonal_projection_matrix(window_size[0], window_size[1]);
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_irregular_volume_mapper::config_settings_changed()
{
  m_config_settings_changed = true;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_irregular_volume_mapper::opacity_changed()
{
  m_opacity_changed = true;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_irregular_volume_mapper::rtc_kernel_changed(vtknvindex_rtc_kernels kernel,
  const std::string& kernel_program, const void* params_buffer, mi::Uint32 buffer_size)
{
  if (kernel != m_volume_rtc_kernel.rtc_kernel)
  {
    m_volume_rtc_kernel.rtc_kernel = kernel;
    m_rtc_kernel_changed = true;
  }

  if (kernel_program != m_volume_rtc_kernel.kernel_program)
  {
    m_volume_rtc_kernel.kernel_program = kernel_program;
    m_rtc_kernel_changed = true;
  }

  m_volume_rtc_kernel.params_buffer = params_buffer;
  m_volume_rtc_kernel.buffer_size = buffer_size;
  m_rtc_param_changed = true;
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_irregular_volume_mapper::Render(vtkRenderer* ren, vtkVolume* vol)
{
  if (!m_cluster_properties->is_active_instance())
  {
    return;
  }

  // Ensure IceT is disabled in MPI mode, print warning only on first rank
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

  // Check for volume property changed.
  std::string cur_property(this->GetArrayName());
  if (cur_property != m_prev_property)
  {
    m_volume_changed = true;
    m_prev_property = cur_property;
  }

  // Initialize the mapper.
  if ((!m_is_mapper_initialized || m_volume_changed) && !initialize_mapper(ren, vol))
  {
    ERROR_LOG << "Failed to initialize the mapper in "
              << "vtknvindex_irregular_volume_mapper::Render.";
    ERROR_LOG << "NVIDIA IndeX rendering was aborted.";
    return;
  }

  bool needs_activate = m_cluster_properties->activate();
  if (needs_activate)
  {
    // Importers might be triggered again, so data must be made available
    m_is_data_prepared = false;
  }

  // Prepare data to be rendered.
  if ((!m_is_data_prepared || m_volume_changed) && !prepare_data())
  {
    ERROR_LOG << "Failed to prepare data in "
              << "vtknvindex_irregular_volume_mapper::Render.";
    ERROR_LOG << "NVIDIA IndeX rendering was aborted.";
    return;
  }

  // Wait all ranks finish to write volume data before the render starts.
  m_controller->Barrier();

  if (m_index_instance->is_index_viewer() && m_index_instance->is_index_initialized())
  {
    vtkTimerLog::MarkStartEvent("NVIDIA-IndeX: Rendering");

    {
      // DiCE database access.
      mi::base::Handle<mi::neuraylib::IDice_transaction> dice_transaction(
        m_index_instance->m_global_scope->create_transaction<mi::neuraylib::IDice_transaction>());
      assert(dice_transaction.is_valid_interface());

      // Setup scene information.
      if (!m_scene.scene_created())
      {
        if (!m_raw_cuts.empty())
        {
          vtknvindex_KDTree_affinity* affinity_vtk_kdtree =
            m_cluster_properties->get_affinity_kdtree();
          if (affinity_vtk_kdtree)
          {
            affinity_vtk_kdtree->build(m_raw_cuts, m_raw_cuts_ranks);
          }
        }

        m_scene.create_scene(ren, vol, dice_transaction, vtknvindex_scene::VOLUME_TYPE_IRREGULAR);
        needs_activate = true;
      }
      else if (m_volume_changed)
      {
        m_scene.update_volume(dice_transaction, vtknvindex_scene::VOLUME_TYPE_IRREGULAR);
        needs_activate = true;
      }

      if (needs_activate)
      {
        m_cluster_properties->warn_if_multiple_visible_instances(this->GetArrayName());
        m_scene.activate(dice_transaction.get());
        m_config_settings_changed = true; // force setting region-of-interest
      }

      // Update scene parameters.
      m_scene.update_scene(
        ren, vol, dice_transaction, m_config_settings_changed, m_opacity_changed, false);
      m_config_settings_changed = false;
      m_opacity_changed = false;

      // Update CUDA code.
      if (m_rtc_kernel_changed || m_rtc_param_changed)
      {
        m_scene.update_rtc_kernel(dice_transaction, m_volume_rtc_kernel,
          vtknvindex_scene::VOLUME_TYPE_IRREGULAR, m_rtc_kernel_changed);
        m_rtc_kernel_changed = false;
        m_rtc_param_changed = false;
      }

      // Update render canvas.
      update_canvas(ren);

      dice_transaction->commit();
    }

    // Render the scene.
    {
      // DiCE database access.
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

        // Synchronize and update the NVIDIA IndeX session with the
        // scene (volume data, transformation matrix, camera, etc.).
        m_index_instance->m_iindex_session->update(
          m_index_instance->m_session_tag, dice_transaction.get());

        // NVIDIA IndeX render call returns frame results
        mi::base::Handle<nv::index::IFrame_results> frame_results(
          m_index_instance->m_iindex_rendering->render(m_index_instance->m_session_tag,
            &(m_index_instance->m_opengl_canvas), // Opengl canvas.
            dice_transaction.get(),
            0,    // No progress_callback.
            0,    // No Frame information
            true, // = g_immediate_final_parallel_compositing
            ren->GetNumberOfPropsRendered() ? &(m_index_instance->m_opengl_app_buffer)
                                            : NULL) // ParaView depth buffer, if present.
          );

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

        if (m_cluster_properties->get_config_settings()->is_log_performance())
          m_performance_values.print_perf_values(frame_results);
      }

      dice_transaction->commit();
    }

    vtkTimerLog::MarkEndEvent("NVIDIA-IndeX: Rendering");
  }
  else if (m_index_instance->is_index_viewer())
  {
    static bool first = true;
    if (first)
      ERROR_LOG << "The NVIDIA IndeX plugin was not initialized! See the log output for details.";
    first = false;
  }

  m_volume_changed = false;

  // clean shared memory
  m_controller->Barrier();
  m_cluster_properties->unlink_shared_memory(false);
}

//-------------------------------------------------------------------------------------------------
void vtknvindex_irregular_volume_mapper::set_visibility(bool visibility)
{
  m_cluster_properties->set_visibility(visibility);
}
