#ifndef CatalystAdaptor_h
#define CatalystAdaptor_h

#include "FEDataStructures.h"
#include <catalyst.hpp>

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
    node["catalyst/scripts/script" + std::to_string(cc - 1)].set_string(argv[cc]);
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
  mesh["coordsets/coords/type"].set("explicit");

  mesh["coordsets/coords/values/x"].set_external(
    grid.GetPointsArray(), grid.GetNumberOfPoints(), /*offset=*/0, /*stride=*/3 * sizeof(double));
  mesh["coordsets/coords/values/y"].set_external(grid.GetPointsArray(), grid.GetNumberOfPoints(),
    /*offset=*/sizeof(double), /*stride=*/3 * sizeof(double));
  mesh["coordsets/coords/values/z"].set_external(grid.GetPointsArray(), grid.GetNumberOfPoints(),
    /*offset=*/2 * sizeof(double), /*stride=*/3 * sizeof(double));

  // Next, add topology
  mesh["topologies/mesh/type"].set("unstructured");
  mesh["topologies/mesh/coordset"].set("coords");
  mesh["topologies/mesh/elements/shape"].set("hex");
  mesh["topologies/mesh/elements/connectivity"].set_external(
    grid.GetCellPoints(0), grid.GetNumberOfCells() * 8);

  // Finally, add fields.
  auto fields = mesh["fields"];
  fields["velocity/association"].set("vertex");
  fields["velocity/topology"].set("mesh");
  fields["velocity/volume_dependent"].set("false");

  // velocity is stored in non-interlaced form (unlike points).
  fields["velocity/values/x"].set_external(
    attribs.GetVelocityArray(), grid.GetNumberOfPoints(), /*offset=*/0);
  fields["velocity/values/y"].set_external(attribs.GetVelocityArray(), grid.GetNumberOfPoints(),
    /*offset=*/grid.GetNumberOfPoints() * sizeof(double));
  fields["velocity/values/z"].set_external(attribs.GetVelocityArray(), grid.GetNumberOfPoints(),
    /*offset=*/grid.GetNumberOfPoints() * sizeof(double) * 2);

  // pressure is cell-data.
  fields["pressure/association"].set("element");
  fields["pressure/topology"].set("mesh");
  fields["pressure/volume_dependent"].set("false");
  fields["pressure/values"].set_external(attribs.GetPressureArray(), grid.GetNumberOfCells());

  catalyst_execute(conduit_cpp::c_node(&exec_params));
}

void Finalize()
{
  conduit_cpp::Node node;
  catalyst_finalize(conduit_cpp::c_node(&node));
}
}

#endif
