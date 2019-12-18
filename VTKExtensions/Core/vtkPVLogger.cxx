/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLogger.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVLogger.h"

#include "vtkObjectFactory.h"

#include <map>
#include <vtksys/SystemTools.hxx>

namespace
{
static vtkLogger::Verbosity DefaultVerbosity = vtkLogger::VERBOSITY_TRACE;

using MapType = std::map<int, vtkLogger::Verbosity>;
static MapType& get_map()
{
  static MapType the_map{};
  return the_map;
}

static vtkLogger::Verbosity get_verbosity(int key, const char* envvar = nullptr)
{
  auto& map = get_map();
  auto iter = map.find(key);
  if (iter != map.end())
  {
    return iter->second != vtkLogger::VERBOSITY_INVALID ? iter->second : DefaultVerbosity;
  }

  if (envvar != nullptr)
  {
    if (const char* envval = vtksys::SystemTools::GetEnv(envvar))
    {
      auto env_verbosity = vtkLogger::ConvertToVerbosity(envval);
      if (env_verbosity != vtkLogger::VERBOSITY_INVALID)
      {
        vtkLogF(TRACE, "processed environment variable `%s=%s` for log verbosity", envvar, envval);

        // cache for later.
        map.insert(std::make_pair(key, env_verbosity));
        return env_verbosity;
      }
      else
      {
        vtkLogF(TRACE, "ignoring environment variable `%s`, invalid value `%s`", envvar, envval);
      }
    }
  }

  // either envvar not defined or was invalid. we still cache the value since
  // lookup is faster but flag it so that if DefaultVerbosity changes, we report
  // the changed value correctly. We use VERBOSITY_INVALID as an indicator that
  // default value should be used.
  map.insert(std::make_pair(key, vtkLogger::VERBOSITY_INVALID));
  return DefaultVerbosity;
}

static void set_verbosity(int key, vtkLogger::Verbosity verbosity)
{
  get_map().insert(std::make_pair(key, verbosity));
}

static const int PipelineVerbosityKey = 1;
static const int PluginVerbosityKey = 2;
static const int DataMovementVerbosityKey = 3;
static const int RenderingVerbosityKey = 4;
static const int ApplicationVerbosityKey = 5;
static const int ExecutionVerbosityKey = 6;
}

//----------------------------------------------------------------------------
vtkPVLogger::vtkPVLogger()
{
}

//----------------------------------------------------------------------------
vtkPVLogger::~vtkPVLogger()
{
}

//----------------------------------------------------------------------------
vtkLogger::Verbosity vtkPVLogger::GetDefaultVerbosity()
{
  return DefaultVerbosity;
}

//----------------------------------------------------------------------------
void vtkPVLogger::SetDefaultVerbosity(vtkLogger::Verbosity value)
{
  DefaultVerbosity = value;
}

//----------------------------------------------------------------------------
vtkLogger::Verbosity vtkPVLogger::GetPipelineVerbosity()
{
  return get_verbosity(PipelineVerbosityKey, "PARAVIEW_LOG_PIPELINE_VERBOSITY");
}

//----------------------------------------------------------------------------
void vtkPVLogger::SetPipelineVerbosity(vtkLogger::Verbosity value)
{
  if (value > vtkLogger::VERBOSITY_INVALID && value <= vtkLogger::VERBOSITY_MAX)
  {
    set_verbosity(PipelineVerbosityKey, value);
  }
  else
  {
    vtkLogF(WARNING, "ignoring invalid verbosity %d", value);
  }
}

//----------------------------------------------------------------------------
vtkLogger::Verbosity vtkPVLogger::GetExecutionVerbosity()
{
  return get_verbosity(ExecutionVerbosityKey, "PARAVIEW_LOG_EXECUTION_VERBOSITY");
}

//----------------------------------------------------------------------------
void vtkPVLogger::SetExecutionVerbosity(vtkLogger::Verbosity value)
{
  if (value > vtkLogger::VERBOSITY_INVALID && value <= vtkLogger::VERBOSITY_MAX)
  {
    set_verbosity(ExecutionVerbosityKey, value);
  }
  else
  {
    vtkLogF(WARNING, "ignoring invalid verbosity %d", value);
  }
}

//----------------------------------------------------------------------------
vtkLogger::Verbosity vtkPVLogger::GetPluginVerbosity()
{
  return get_verbosity(PluginVerbosityKey, "PARAVIEW_LOG_PLUGIN_VERBOSITY");
}

//----------------------------------------------------------------------------
void vtkPVLogger::SetPluginVerbosity(vtkLogger::Verbosity value)
{
  if (value > vtkLogger::VERBOSITY_INVALID && value <= vtkLogger::VERBOSITY_MAX)
  {
    set_verbosity(PluginVerbosityKey, value);
  }
  else
  {
    vtkLogF(WARNING, "ignoring invalid verbosity %d", value);
  }
}

//----------------------------------------------------------------------------
vtkLogger::Verbosity vtkPVLogger::GetDataMovementVerbosity()
{
  return get_verbosity(DataMovementVerbosityKey, "PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY");
}

//----------------------------------------------------------------------------
void vtkPVLogger::SetDataMovementVerbosity(vtkLogger::Verbosity value)
{
  if (value > vtkLogger::VERBOSITY_INVALID && value <= vtkLogger::VERBOSITY_MAX)
  {
    set_verbosity(DataMovementVerbosityKey, value);
  }
  else
  {
    vtkLogF(WARNING, "ignoring invalid verbosity %d", value);
  }
}

//----------------------------------------------------------------------------
vtkLogger::Verbosity vtkPVLogger::GetRenderingVerbosity()
{
  return get_verbosity(RenderingVerbosityKey, "PARAVIEW_LOG_RENDERING_VERBOSITY");
}

//----------------------------------------------------------------------------
void vtkPVLogger::SetRenderingVerbosity(vtkLogger::Verbosity value)
{
  if (value > vtkLogger::VERBOSITY_INVALID && value <= vtkLogger::VERBOSITY_MAX)
  {
    set_verbosity(RenderingVerbosityKey, value);
  }
  else
  {
    vtkLogF(WARNING, "ignoring invalid verbosity %d", value);
  }
}

//----------------------------------------------------------------------------
vtkLogger::Verbosity vtkPVLogger::GetApplicationVerbosity()
{
  return get_verbosity(ApplicationVerbosityKey, "PARAVIEW_LOG_APPLICATION_VERBOSITY");
}

//----------------------------------------------------------------------------
void vtkPVLogger::SetApplicationVerbosity(vtkLogger::Verbosity value)
{
  if (value > vtkLogger::VERBOSITY_INVALID && value <= vtkLogger::VERBOSITY_MAX)
  {
    set_verbosity(ApplicationVerbosityKey, value);
  }
  else
  {
    vtkLogF(WARNING, "ignoring invalid verbosity %d", value);
  }
}

//----------------------------------------------------------------------------
void vtkPVLogger::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
