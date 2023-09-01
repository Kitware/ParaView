// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCatalystBlueprint.h"

#include "vtkPVLogger.h"

#include <catalyst_conduit_blueprint.hpp>
#include <cinttypes>

namespace
{
void format_error(const conduit_cpp::Node& n)
{
  if (n.has_child("valid") && n["valid"].as_string() == "false")
  {
    vtkLogScopeF(ERROR, "%s", n.name().c_str());
    for (size_t i = 0; i < n.number_of_children(); i++)
    {
      format_error(n.child(i));
    }
    if (n.has_child("errors"))
    {
      vtkLogF(ERROR, "Errors: %zu", n["errors"].number_of_children());
      for (size_t i = 0; i < n["errors"].number_of_children(); i++)
      {
        vtkLogF(ERROR, "Error %zu : %s", i, n["errors"][i].as_string().c_str());
      }
    }
  }
}
}

namespace initialize
{
namespace scripts
{

namespace args
{
bool verify(const std::string& protocol, const conduit_cpp::Node& n)
{
  vtkVLogScopeF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "%s::verify", protocol.c_str());
  if (!n.dtype().is_list())
  {
    vtkLogF(ERROR, "node must be a 'list'.");
    return false;
  }

  if (n.number_of_children() == 0)
  {
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "empty node provided.");
  }

  conduit_index_t nchildren = n.number_of_children();
  for (conduit_index_t i = 0; i < nchildren; ++i)
  {
    const auto param = n.child(i);
    if (!param.dtype().is_string())
    {
      vtkLogF(ERROR, "unsupported type '%s'; only string types are supported.",
        param.dtype().name().c_str());
      return false;
    }
  }

  return true;
}
}

bool verify(const std::string& protocol, const conduit_cpp::Node& n)
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
  int index = 0;
  conduit_index_t nchildren = n.number_of_children();
  for (conduit_index_t i = 0; i < nchildren; ++i)
  {
    const auto script = n.child(i);
    if (script.dtype().is_string())
    {
      // all's well, child node directly a string which is interpreted
      // as filename.
      vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "script: '%s'", script.as_string().c_str());
    }
    else if (script.dtype().is_object())
    {
      if (!script.has_path("filename"))
      {
        vtkLogF(ERROR, "'script/%s' missing required 'filename'", script.name().c_str());
        return false;
      }

      if (!script["filename"].dtype().is_string())
      {
        vtkLogF(ERROR, "'script/%s/filename' must be a 'string'.", script.name().c_str());
        return false;
      }

      vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "script (%s): '%s'",
        n.dtype().is_object() ? script.name().c_str() : std::to_string(index).c_str(),
        script["filename"].as_string().c_str());

      // let's verify "args", if any.
      if (script.has_path("args"))
      {
        if (!args::verify(protocol + "::scripts::args", script["args"]))
        {
          return false;
        }
      }
    }
    else
    {
      vtkLogF(ERROR, "child-node must be a 'string' or 'object'");
      return false;
    }
    ++index;
  }
  return true;
}
} // namespace scripts

namespace pipeline
{
bool verify(const std::string& protocol, const conduit_cpp::Node& n)
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
bool verify(const std::string& protocol, const conduit_cpp::Node& n)
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
  conduit_index_t nchildren = n.number_of_children();
  for (conduit_index_t i = 0; i < nchildren; ++i)
  {
    const auto pipeline = n.child(i);
    if (!pipeline::verify(protocol + "::pipeline", pipeline))
    {
      return false;
    }
  }
  return true;
}
} // namespace pipelines

bool verify(const std::string& protocol, const conduit_cpp::Node& n)
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
bool verify(const std::string& protocol, const conduit_cpp::Node& n)
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
      PARAVIEW_LOG_CATALYST_VERBOSITY(), "'timestep' set to %" PRIi64, n["timestep"].to_int64());
  }
  else if (n.has_child("cycle"))
  {
    if (!n["cycle"].dtype().is_integer())
    {
      vtkLogF(ERROR, "'cycle' must be an integer.");
      return false;
    }
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "'cycle' set to %" PRIi64, n["cycle"].to_int64());
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
    vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "'time' set to %lf", n["time"].to_float64());
  }

  if (n.has_child("multiblock") && !n["multiblock"].dtype().is_integer())
  {
    vtkLogF(ERROR, "'multiblock' must be an integral.");
    return false;
  }
  else if (n.has_child("multiblock"))
  {
    vtkVLogF(
      PARAVIEW_LOG_CATALYST_VERBOSITY(), "'multiblock' set to %" PRIi32, n["multiblock"].to_int());
  }

  return true;
}
} // namespace state

namespace state_fields
{
bool verify(const std::string& protocol, const conduit_cpp::Node& n)
{
  vtkVLogF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "%s: verify", protocol.c_str());
  if (!n.dtype().is_object())
  {
    vtkLogF(ERROR, "node must be an 'object'.");
    return false;
  }

  const conduit_index_t nchildren = n.number_of_children();
  for (conduit_index_t i = 0; i < nchildren; ++i)
  {
    auto child = n.child(i);
    // String nodes are supported, let's check other types.
    if (!child.dtype().is_string())
    {
      conduit_cpp::Node info;
      if (!conduit_cpp::BlueprintMcArray::verify(child, info))
      {
        // in some-cases, this may directly be an array of numeric values; if so, handle that.
        if (!child.dtype().is_number())
        {
          vtkLogF(ERROR,
            "Validation of mesh state field '%s' failed. Expected types are: string, MCArrays or "
            "numeric values.",
            child.name().c_str());
          format_error(info);
          return false;
        }
      }
    }
  }

  return true;
}
} // namespace state_fields

namespace channel
{
bool verify(const std::string& protocol, const conduit_cpp::Node& n)
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
    conduit_cpp::Node info;
    if (conduit_cpp::Blueprint::verify("mesh", n["data"], info))
    {
      vtkVLogScopeF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "Conduit Mesh blueprint verified.");
    }
    else
    {
      vtkLogF(ERROR, "Conduit Mesh blueprint validate failed!");
      format_error(info);
      return false;
    }

    if (n.has_path("state/fields"))
    {
      if (!state_fields::verify(protocol + "::state::fields", n["state/fields"]))
      {
        return false;
      }
    }
  }
  else if (type == "multimesh" || type == "amrmesh")
  {
    const auto data = n["data"];
    const conduit_index_t nchildren = data.number_of_children();
    for (conduit_index_t i = 0; i < nchildren; ++i)
    {
      auto child = data.child(i);
      conduit_cpp::Node info;
      if (conduit_cpp::Blueprint::verify("mesh", child, info))
      {
        vtkVLogScopeF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "%s: Conduit Mesh blueprint verified.",
          child.name().c_str());
      }
      else
      {
        vtkLogF(ERROR, "%s: Conduit Mesh blueprint validate failed!", child.name().c_str());
        format_error(info);
        return false;
      }

      if (child.has_path("state/fields"))
      {
        if (!state_fields::verify(protocol + "::state::fields", child["state/fields"]))
        {
          return false;
        }
      }
    }
    vtkVLogScopeF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "multimesh blueprint verified.");
  }
  else if (type == "ioss")
  {
    // no additional verification at this time.
  }
  else if (type == "fides")
  {
    // no additional verification at this time.
  }
  else if (type == "amrmesh")
  {
    // no additional verification at this time.
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
bool verify(const std::string& protocol, const conduit_cpp::Node& n)
{
  vtkVLogScopeF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "%s: verify", protocol.c_str());
  if (!n.dtype().is_object())
  {
    vtkLogF(ERROR, "node must be an 'object'.");
    return false;
  }

  conduit_index_t nchildren = n.number_of_children();
  for (conduit_index_t i = 0; i < nchildren; ++i)
  {
    const auto channel = n.child(i);
    const auto& name = channel.name();
    const std::string completeName =
      std::string(protocol).append("::channel['").append(name).append("']");
    if (!channel::verify(completeName, channel))
    {
      return false;
    }
  }
  return true;
}

} // namespace channels

bool verify(const std::string& protocol, const conduit_cpp::Node& n)
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

namespace assembly
{
bool verify(const std::string& protocol, const conduit_cpp::Node& n)
{
  vtkVLogScopeF(PARAVIEW_LOG_CATALYST_VERBOSITY(), "%s: verify", protocol.c_str());
  if (n.dtype().is_list())
  {
    // list can only comprise of string items.
    for (conduit_index_t cc = 0, max = n.number_of_children(); cc < max; ++cc)
    {
      if (!n.child(cc).dtype().is_string())
      {
        vtkLogF(ERROR, "list cannot have non-string items!");
        return false;
      }
    }
    return true;
  }
  else if (n.dtype().is_string())
  {
    return true;
  }
  else if (n.dtype().is_object())
  {
    for (conduit_index_t cc = 0; cc < n.number_of_children(); ++cc)
    {
      if (!assembly::verify(protocol + "::" + n.name(), n.child(cc)))
      {
        return false;
      }
    }
    return true;
  }
  else
  {
    vtkLogF(ERROR, "node must be 'object', 'list', or 'string'.");
    return false;
  }
}

} // namespace assembly

//----------------------------------------------------------------------------
vtkCatalystBlueprint::vtkCatalystBlueprint() = default;

//----------------------------------------------------------------------------
vtkCatalystBlueprint::~vtkCatalystBlueprint() = default;

//----------------------------------------------------------------------------
bool vtkCatalystBlueprint::Verify(const std::string& protocol, const conduit_cpp::Node& n)
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
  else if (protocol == "assembly")
  {
    res = assembly::verify("assembly", n);
  }
  return res;
}

//----------------------------------------------------------------------------
void vtkCatalystBlueprint::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
