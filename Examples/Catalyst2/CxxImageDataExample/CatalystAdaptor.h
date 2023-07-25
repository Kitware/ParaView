#ifndef CatalystAdaptor_h
#define CatalystAdaptor_h

#include "FEDataStructures.h"
#include <catalyst.hpp>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

namespace CatalystAdaptor
{
static std::vector<std::string> filesToValidate;

/**
 * In this example, we show how we can use Catalysts's C++
 * wrapper around conduit's C API to create Conduit nodes.
 * This is not required. A C++ adaptor can just as
 * conveniently use the Conduit C API to setup the
 * `conduit_node`. However, this example shows that one can
 * indeed use Catalyst's C++ API, if the developer so chooses.
 */
void Initialize(int argc, char* argv[])
{
  conduit_cpp::Node node;
  for (int cc = 1; cc < argc; ++cc)
  {
    if (strcmp(argv[cc], "--output") == 0 && (cc + 1) < argc)
    {
      node["catalyst/pipelines/0/type"].set("io");
      node["catalyst/pipelines/0/filename"].set(argv[cc + 1]);
      node["catalyst/pipelines/0/channel"].set("grid");
      ++cc;
    }
    else if (strcmp(argv[cc], "--exists") == 0 && (cc + 1) < argc)
    {
      filesToValidate.push_back(argv[cc + 1]);
      ++cc;
    }
    else
    {
      const auto path = std::string(argv[cc]);
      // note: one can simply add the script file as follows:
      // node["catalyst/scripts/script" + std::to_string(cc - 1)].set_string(path);

      // alternatively, use this form to pass optional parameters to the script.
      const auto name = "catalyst/scripts/script" + std::to_string(cc - 1);
      node[name + "/filename"].set_string(path);
      node[name + "/args"].append().set_string("argument0");
      node[name + "/args"].append().set_string("argument1=12");
      node[name + "/args"].append().set_string("--argument3");
      node[name + "/args"].append().set_string("--channel-name=grid");
    }
  }

  // indicate that we want to load ParaView-Catalyst
  node["catalyst_load/implementation"].set_string("paraview");
  node["catalyst_load/search_paths/paraview"] = PARAVIEW_IMPL_DIR;

  catalyst_status err = catalyst_initialize(conduit_cpp::c_node(&node));
  if (err != catalyst_status_ok)
  {
    std::cerr << "ERROR: Failed to initialize Catalyst: " << err << std::endl;
  }
}

void Execute(int cycle, double time, Grid& grid, Attributes& attribs)
{
  conduit_cpp::Node exec_params;

  // add time/cycle information
  auto state = exec_params["catalyst/state"];
  state["timestep"].set(cycle);
  state["time"].set(time);

  // add optional execution parameters
  state["parameters"].append().set_string("parameter0");
  state["parameters"].append().set_string("parameter1=42");
  state["parameters"].append().set_string("parameter2=doThing");
  state["parameters"].append().set_string("timeParam=" + std::to_string(time));

  // Add channels.
  // We only have 1 channel here. Let's name it 'grid'.
  auto channel = exec_params["catalyst/channels/grid"];

  // Since this example is using Conduit Mesh Blueprint to define the mesh,
  // we set the channel's type to "mesh".
  channel["type"].set("mesh");

  // now create the mesh.
  auto mesh = channel["data"];

  // start with coordsets (of course, the sequence is not important, just make
  // it easier to think in this order).
  mesh["coordsets/coords/type"].set("uniform");

  const auto* ext = grid.GetExtent();
  mesh["coordsets/coords/dims/i"].set(ext[1] - ext[0] + 1);
  mesh["coordsets/coords/dims/j"].set(ext[3] - ext[2] + 1);
  mesh["coordsets/coords/dims/k"].set(ext[5] - ext[4] + 1);

  double localOrigin[3];
  grid.GetLocalPoint(0, localOrigin);
  mesh["coordsets/coords/origin/x"].set(localOrigin[0]);
  mesh["coordsets/coords/origin/y"].set(localOrigin[1]);
  mesh["coordsets/coords/origin/z"].set(localOrigin[2]);

  const auto spacing = grid.GetSpacing();
  mesh["coordsets/coords/spacing/dx"].set(spacing[0]);
  mesh["coordsets/coords/spacing/dy"].set(spacing[1]);
  mesh["coordsets/coords/spacing/dz"].set(spacing[2]);

  // Next, add topology
  mesh["topologies/mesh/type"].set("uniform");
  mesh["topologies/mesh/coordset"].set("coords");

  // Finally, add fields.
  auto fields = mesh["fields"];
  fields["velocity/association"].set("vertex");
  fields["velocity/topology"].set("mesh");
  fields["velocity/volume_dependent"].set("false");

  // velocity is stored in non-interlaced form (unlike points).
  fields["velocity/values/x"].set_external(
    attribs.GetVelocityArray(), grid.GetNumberOfLocalPoints(), /*offset=*/0);
  fields["velocity/values/y"].set_external(attribs.GetVelocityArray(),
    grid.GetNumberOfLocalPoints(),
    /*offset=*/grid.GetNumberOfLocalPoints() * sizeof(double));
  fields["velocity/values/z"].set_external(attribs.GetVelocityArray(),
    grid.GetNumberOfLocalPoints(),
    /*offset=*/grid.GetNumberOfLocalPoints() * sizeof(double) * 2);

  // pressure is cell-data.
  fields["pressure/association"].set("element");
  fields["pressure/topology"].set("mesh");
  fields["pressure/volume_dependent"].set("false");
  fields["pressure/values"].set_external(attribs.GetPressureArray(), grid.GetNumberOfLocalCells());

  catalyst_status err = catalyst_execute(conduit_cpp::c_node(&exec_params));
  if (err != catalyst_status_ok)
  {
    std::cerr << "ERROR: Failed to execute Catalyst: " << err << std::endl;
  }
}

void Finalize()
{
  conduit_cpp::Node node;
  catalyst_status err = catalyst_finalize(conduit_cpp::c_node(&node));
  if (err != catalyst_status_ok)
  {
    std::cerr << "ERROR: Failed to finalize Catalyst: " << err << std::endl;
  }

  for (const auto& fname : filesToValidate)
  {
    std::ifstream istrm(fname.c_str(), std::ios::binary);
    if (!istrm.is_open())
    {
      std::cerr << "ERROR: Failed to open file '" << fname.c_str() << "'." << std::endl;
    }
  }
}
}

#endif
