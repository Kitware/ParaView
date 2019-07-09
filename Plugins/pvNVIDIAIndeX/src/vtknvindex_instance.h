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

#ifndef vtknvindex_instance_h
#define vtknvindex_instance_h

#include <map>
#include <vector>

//#include <mi/dice.h>
#include "vtknvindex_opengl_app_buffer.h"
#include "vtknvindex_opengl_canvas.h"
#include <nv/index/iindex_debug_configuration.h>

class vtknvindex_colormap;

class vtknvindex_instance
{
public:
  vtknvindex_instance();
  ~vtknvindex_instance();

  static vtknvindex_instance* get();
  static vtknvindex_instance* create();

  bool is_index_viewer() const;
  bool is_index_rank() const;
  mi::Sint32 get_cur_local_rank_id() const;

  // Initialize IndeX
  void init_index();
  bool is_index_initialized() const;

  // Cameras
  mi::neuraylib::Tag get_perspective_camera() const;
  mi::neuraylib::Tag get_parallel_camera() const;

  // Colormaps
  vtknvindex_colormap* get_index_colormaps() const;
  mi::neuraylib::Tag get_slice_colormap() const;

  // Geom group
  mi::neuraylib::Tag get_scene_geom_group() const;

  // Get the NVIDIA IndeX interface handle.
  mi::base::Handle<nv::index::IIndex>& get_interface();

  // Plug-in version
  const char* get_version() const;

public:
  // DiCE database
  mi::base::Handle<mi::neuraylib::IDatabase> m_database;
  // DiCE global scope
  mi::base::Handle<mi::neuraylib::IScope> m_global_scope;
  // NVIDIA IndeX session
  mi::base::Handle<nv::index::IIndex_session> m_iindex_session;
  // NVIDIA IndeX rendering
  mi::base::Handle<nv::index::IIndex_rendering> m_iindex_rendering;
  // NVIDIA IndeX cluster configuration
  mi::base::Handle<nv::index::ICluster_configuration> m_icluster_configuration;
  // NVIDIA IndeX debug configuration
  mi::base::Handle<nv::index::IIndex_debug_configuration> m_iindex_debug_configuration;
  // Session tag
  mi::neuraylib::Tag m_session_tag;
  // Opengl canvas to the NVIDIA IndeX render call, a render target.
  vtknvindex_opengl_canvas m_opengl_canvas;
  // Opengl application buffer supplied to NVIDIA IndeX.
  vtknvindex_opengl_app_buffer m_opengl_app_buffer;

private:
  vtknvindex_instance(vtknvindex_instance const&) = delete;
  void operator=(vtknvindex_instance const&) = delete;

  void build_cluster_info();

  // Load/Unload NVIDIA IndeX library.
  bool load_nvindex();
  bool unload_nvindex();

  // Authenticate NVIDIA IndeX license.
  bool authenticate_nvindex();

  // Setup and start NVIDIA IndeX library.
  mi::Uint32 setup_nvindex();

  // Shutting down NVIDIA IndeX library.
  bool shutdown_nvindex();

  // Initialize application rendering context.
  void initialize_arc();

  // Initialize IndeX scene graph
  void init_scene_graph();

  bool m_is_index_rank;
  bool m_is_index_viewer;
  bool m_is_index_initialized;

  std::map<std::string, std::vector<mi::Sint32> > m_hostmane_to_rankids; // Hostname to rank_id.
  std::vector<std::string> m_host_list;                                  // List of host

  std::string m_nvindexlib_fname;                          // libnvindex.so/dll string name.
  void* m_p_handle;                                        // Library handle.
  mi::base::Handle<nv::index::IIndex> m_nvindex_interface; // Entry point to NVIDIA IndeX.

  // Scene cameras
  mi::neuraylib::Tag m_perspective_camera_tag; // Perspective camera database tag.
  mi::neuraylib::Tag m_parallel_camera_tag;    // Parallel camera database tag.

  // Scene graph tags
  mi::neuraylib::Tag m_volume_colormap_tag; // Colormap shared among all volumes.
  mi::neuraylib::Tag m_slice_colormap_tag;  // Colormap shared among all slices.
  mi::neuraylib::Tag m_geom_group_tag;      // Volumes, slices root group.

  // Colormaps manager
  vtknvindex_colormap* m_nvindex_colormaps; // NVIDIA IndeX colormaps manager.

  // IndeX instance
  static vtknvindex_instance* s_index_instance;
};
#endif // vtknvindex_instance_h
