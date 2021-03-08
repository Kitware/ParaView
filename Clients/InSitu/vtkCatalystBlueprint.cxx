/*=========================================================================

  Program:   ParaView
  Module:    vtkCatalystBlueprint.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCatalystBlueprint.h"

#include "vtkPVLogger.h"

#include <conduit_blueprint.hpp>
#include <inttypes.h>

namespace initialize
{
namespace scripts
{
bool verify(const std::string& protocol, const conduit::Node& n)
{
  vtkVLogScopeF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "%s: verify", protocol.c_str());
  if (!n.dtype().is_object() && !n.dtype().is_list())
  {
    vtkLogF(ERROR, "node must be an 'object' or 'list'.");
    return false;
  }
  if (n.number_of_children() == 0)
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "empty 'n' provided.");
  }
  auto iter = n.children();
  while (iter.has_next())
  {
    auto& script = iter.next();
    if (!script.dtype().is_string())
    {
      vtkLogF(ERROR, "child-node must be a 'string'.");
      return false;
    }
    else
    {
      vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "script: '%s'", script.as_string().c_str());
    }
  }
  return true;
}
} // namespace scripts

namespace pipeline
{
bool verify(const std::string& protocol, const conduit::Node& n)
{
  vtkVLogScopeF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "%s: verify", protocol.c_str());
  if (!n.dtype().is_object())
  {
    vtkLogF(ERROR, "node must be an 'object'.");
    return false;
  }

  if (!n.has_child("type"))
  {
    vtkLogF(ERROR, "missing 'type'.");
    return false;
  }

  if (n["type"].as_string() == "io")
  {
    if (!n.has_child("filename") || !n["filename"].dtype().is_string())
    {
      vtkLogF(ERROR, "missing 'filename' or not of type 'string'.");
      return false;
    }

    if (!n.has_child("channel"))
    {
      vtkLogF(ERROR, "missing 'channel'.");
      return false;
    }
    else if (!n["channel"].dtype().is_string())
    {
      vtkLogF(ERROR, "channel must be a string.");
      return false;
    }

    return true;
  }
  else
  {
    vtkLogF(ERROR, "unsupported type '%s'", n["type"].as_string().c_str());
    return false;
  }
}

} // namespace pipeline
namespace pipelines
{
bool verify(const std::string& protocol, const conduit::Node& n)
{
  vtkVLogScopeF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "%s: verify", protocol.c_str());
  if (!n.dtype().is_object() && !n.dtype().is_list())
  {
    vtkLogF(ERROR, "node must be an 'object' or 'list'.");
    return false;
  }
  if (n.number_of_children() == 0)
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "empty 'n' provided.");
  }
  auto iter = n.children();
  while (iter.has_next())
  {
    auto& pipeline = iter.next();
    if (!pipeline::verify(protocol + "::pipeline", pipeline))
    {
      return false;
    }
  }
  return true;
}
} // namespace pipelines

bool verify(const std::string& protocol, const conduit::Node& n)
{
  vtkVLogScopeF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "%s: verify", protocol.c_str());
  if (n.dtype().is_empty())
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "node is empty.");
  }
  if (!n.dtype().is_object())
  {
    vtkLogF(ERROR, "node must be an 'object'.");
    return false;
  }
  if (n.has_child("scripts"))
  {
    if (!scripts::verify(protocol + "::scripts", n["scripts"]))
    {
      return false;
    }
  }
  else if (n.has_child("pipelines"))
  {
    // hard-coded pipelines.
    if (!pipelines::verify(protocol + "::pipelines", n["pipelines"]))
    {
      return false;
    }
  }
  else
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "no 'scripts' or 'pipelines' provided.");
  }
  if (n.has_child("mpi_comm"))
  {
    if (!n["mpi_comm"].dtype().is_integer())
    {
      vtkLogF(ERROR, "'mpi_comm' must be an integer. Did you forget to use 'MPI_Type_c2f()'?");
      return false;
    }
  }
  return true;
}

} // namespace initialize

namespace execute
{
namespace state
{
bool verify(const std::string& protocol, const conduit::Node& n)
{
  vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "%s: verify", protocol.c_str());
  if (!n.dtype().is_object())
  {
    vtkLogF(ERROR, "node must be an 'object'.");
    return false;
  }

  if (!n.has_child("timestep") && !n.has_child("cycle"))
  {
    vtkLogF(ERROR, "'timestep' or 'cycle' must be provided.");
    return false;
  }
  else if (n.has_child("timestep"))
  {
    if (!n["timestep"].dtype().is_integer())
    {
      vtkLogF(ERROR, "'timestep' must be an integer.");
      return false;
    }
    vtkVLogF(
      PARAVIEW_LOG_CATALYST_VERBOSITY(), "'timestep' set to %" PRIi64, n["timestep"].as_int64());
  }
  else if (n.has_child("cycle"))
  {
    if (!n["cycle"].dtype().is_integer())
    {
      vtkLogF(ERROR, "'cycle' must be an integer.");
      return false;
    }
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "'cycle' set to %" PRIi64, n["cycle"].as_int64());
  }

  if (!n.has_child("time"))
  {
    vtkLogF(ERROR, "'time' must be provided.");
    return false;
  }
  else if (!n["time"].dtype().is_number())
  {
    vtkLogF(ERROR, "'time' must be a number.");
    return false;
  }
  else
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "'time' set to %lf", n["time"].as_float64());
  }
  return true;
}
} // namespace state

namespace channel
{
bool verify(const std::string& protocol, const conduit::Node& n)
{
  vtkVLogScopeF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "%s: verify", protocol.c_str());
  if (!n.dtype().is_object())
  {
    vtkLogF(ERROR, "node must be an 'object'.");
    return false;
  }

  if (!n.has_child("type"))
  {
    vtkLogF(ERROR, "'type' node is required.");
    return false;
  }
  else if (!n["type"].dtype().is_string())
  {
    vtkLogF(ERROR, "'type' must be a string.");
    return false;
  }

  if (!n.has_child("data"))
  {
    vtkLogF(ERROR, "'data' node is required.");
    return false;
  }
  else if (!n["data"].dtype().is_object())
  {
    vtkLogF(ERROR, "'data' must be an 'object'.");
    return false;
  }

  auto type = n["type"].as_string();
  if (type == "mesh")
  {
    conduit::Node info;
    if (conduit::blueprint::verify("mesh", n["data"], info))
    {
      vtkVLogScopeF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "Conduit Mesh blueprint verified.");
    }
    else
    {
      vtkLogF(ERROR, "Conduit Mesh blueprint validate failed!");
      vtkVLog(PARAVIEW_LOG_CATALYST_VERBOSITY(), << info.to_json());
      return false;
    }
  }
  else
  {
    vtkLogF(ERROR, "unsupported channel type '%s' specified.", type.c_str());
    return false;
  }
  return true;
}
}
namespace channels
{
bool verify(const std::string& protocol, const conduit::Node& n)
{
  vtkVLogScopeF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "%s: verify", protocol.c_str());
  if (!n.dtype().is_object())
  {
    vtkLogF(ERROR, "node must be an 'object'.");
    return false;
  }

  auto iter = n.children();
  while (iter.has_next())
  {
    iter.next();
    const auto& name = iter.name();
    if (!channel::verify(protocol + "::channel['" + name + "']", iter.node()))
    {
      return false;
    }
  }
  return true;
}

} // namespace channels
bool verify(const std::string& protocol, const conduit::Node& n)
{
  vtkVLogScopeF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "%s: verify", protocol.c_str());
  if (!n.dtype().is_object())
  {
    vtkLogF(ERROR, "node must be an 'object'.");
    return false;
  }

  if (!n.has_child("state"))
  {
    vtkLogF(ERROR, "no 'state' specified. time information may be communicated correctly!");
    return false;
  }
  else if (!state::verify(protocol + "::state", n["state"]))
  {
    return false;
  }

  if (!n.has_child("channels"))
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "no 'channels' specified.");
  }
  else if (!channels::verify(protocol + "::channels", n["channels"]))
  {
    return false;
  }
  return true;
}

} // namespace execute

//----------------------------------------------------------------------------
vtkCatalystBlueprint::vtkCatalystBlueprint() = default;

//----------------------------------------------------------------------------
vtkCatalystBlueprint::~vtkCatalystBlueprint() = default;

//----------------------------------------------------------------------------
bool vtkCatalystBlueprint::Verify(const std::string& protocol, const conduit::Node& n)
{
  bool res = false;
  if (protocol == "initialize")
  {
    res = initialize::verify("catalyst", n);
  }
  else if (protocol == "execute")
  {
    res = execute::verify("catalyst", n);
  }
  else if (protocol == "finalize")
  {
    res = true;
  }
  return res;
}

//----------------------------------------------------------------------------
void vtkCatalystBlueprint::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
