#ifndef CatalystAdaptor_h
#define CatalystAdaptor_h

#include "FEDataStructures.h"
#include <catalyst.hpp>
#include <cstring>
#include <string>

namespace CatalystAdaptor
{

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
    else
    {
      node["catalyst/scripts/script" + std::to_string(cc - 1)].set_string(argv[cc]);
    }
  }
  catalyst_initialize(conduit_cpp::c_node(&node));
}

void Execute(int cycle, double time, Grid& grid, Attributes& attribs)
{
  conduit_cpp::Node exec_params;

  // add time/cycle information
  auto state = exec_params["catalyst/state"];
  state["timestep"].set(cycle);
  state["time"].set(time);

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

  double origin[3];
  grid.GetLocalPoint(0, origin);
  mesh["coordsets/coords/origin/x"].set(origin[0]);
  mesh["coordsets/coords/origin/y"].set(origin[1]);
  mesh["coordsets/coords/origin/z"].set(origin[2]);

  const auto spacing = grid.GetSpacing();
  mesh["coordsets/coords/spacing/x"].set(spacing[0]);
  mesh["coordsets/coords/spacing/y"].set(spacing[1]);
  mesh["coordsets/coords/spacing/z"].set(spacing[2]);

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

  catalyst_execute(conduit_cpp::c_node(&exec_params));
}

void Finalize()
{
  conduit_cpp::Node node;
  catalyst_finalize(conduit_cpp::c_node(&node));
}
}

#endif
