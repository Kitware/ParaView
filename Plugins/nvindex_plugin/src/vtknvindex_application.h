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

#ifndef vtknvindex_application_h
#define vtknvindex_application_h

#include <vector>

#include "vtkObject.h"

#include <nv/index/iindex_debug_configuration.h>

#include "vtknvindex_opengl_app_buffer.h"
#include "vtknvindex_opengl_canvas.h"

// ------------------------------------------------------------------------------------------------
// The class vtknvindex_application represents the application rendering context.
class vtknvindex_application
{
public:
  vtknvindex_application();
  ~vtknvindex_application();

  // Load/Unload NVIDIA IndeX library.
  bool load_nvindex_library();
  bool unload_iindex();

  // Authenticate NVIDIA IndeX license.
  bool authenticate_nvindex_library();

  // Setup and start NVIDIA IndeX library.
  mi::Uint32 setup_nvindex_library(const std::vector<std::string>& host_names);

  // Get the NVIDIA IndeX interface handle.
  mi::base::Handle<nv::index::IIndex>& get_interface();

  // Validity check of the license on the viewer host.
  // A remote host always returns false since there is no
  // iindex_session and m_iindex_rendering in a remote host.
  bool is_valid() const;

  // Shutting down NVIDIA IndeX library.
  bool shutdown();

  // Verifying that local host has joined.
  bool is_local_host_joined() const;

  // Initialize application rendering context.
  void initialize_arc();

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
  vtknvindex_application(vtknvindex_application const&) = delete;
  void operator=(vtknvindex_application const&) = delete;

  std::string m_nvindexlib_fname;                          // libnvindex.so/dll string name.
  void* m_p_handle;                                        // Library handle.
  mi::base::Handle<nv::index::IIndex> m_nvindex_interface; // Entry point to NVIDIA IndeX.
};

#endif
