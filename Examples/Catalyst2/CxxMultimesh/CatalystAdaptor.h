#ifndef CatalystAdaptor_h
#define CatalystAdaptor_h

#include "FEDataStructures.h"
#include <catalyst.hpp>

#include <iostream>
#include <numeric>
#include <string>
#include <vector>

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
  node["catalyst_load/implementation"] = "paraview";
  node["catalyst_load/search_paths/paraview"] = PARAVIEW_IMPL_DIR;
  catalyst_status err = catalyst_initialize(conduit_cpp::c_node(&node));
  if (err != catalyst_status_ok)
  {
    std::cerr << "Failed to initialize Catalyst: " << err << std::endl;
  }
}

void Execute(int cycle, double time, Grid& grid, Attributes& attribs, Particles& particles)
{
  conduit_cpp::Node exec_params;

  // add time/cycle information
  auto state = exec_params["catalyst/state"];
  state["timestep"].set(cycle);
  state["time"].set(time);

  // Add channels.
  auto channel = exec_params["catalyst/channels/input"];

  // Since this example is using "multimesh" protocol
  // we set the channel's type to "multimesh".
  channel["type"].set_string("multimesh");

  // now create 1st mesh named "grid"
  auto mesh_grid = channel["data/grid"];

  // start with coordsets (of course, the sequence is not important, just make
  // it easier to think in this order).
  mesh_grid["coordsets/coords/type"].set_string("explicit");

  // We don't use the conduit_cpp::Node::set(std::vector<..>) API since that deep
  // copies. For zero-copy, we use the set_.._ptr(..) API.
  mesh_grid["coordsets/coords/values/x"].set_external(
    grid.GetPointsArray(), grid.GetNumberOfPoints(), /*offset=*/0, /*stride=*/3 * sizeof(double));
  mesh_grid["coordsets/coords/values/y"].set_external(grid.GetPointsArray(),
    grid.GetNumberOfPoints(),
    /*offset=*/sizeof(double), /*stride=*/3 * sizeof(double));
  mesh_grid["coordsets/coords/values/z"].set_external(grid.GetPointsArray(),
    grid.GetNumberOfPoints(),
    /*offset=*/2 * sizeof(double), /*stride=*/3 * sizeof(double));

  // Next, add topology
  mesh_grid["topologies/mesh/type"].set_string("unstructured");
  mesh_grid["topologies/mesh/coordset"].set_string("coords");
  mesh_grid["topologies/mesh/elements/shape"].set_string("hex");
  mesh_grid["topologies/mesh/elements/connectivity"].set(
    grid.GetCellPoints(0), grid.GetNumberOfCells() * 8);

  // Finally, add fields.
  auto fields_grid = mesh_grid["fields"];
  fields_grid["velocity/association"].set_string("vertex");
  fields_grid["velocity/topology"].set_string("mesh");
  fields_grid["velocity/volume_dependent"].set_string("false");

  // Field data (aka meta-data) are declared in the mesh "state" node.
  auto mesh_grid_state_fields = mesh_grid["state/fields"];
  mesh_grid_state_fields["author"] = "Kitware";
  mesh_grid_state_fields["mesh time"] = time;
  mesh_grid_state_fields["mesh timestep"] = cycle;
  mesh_grid_state_fields["mesh external data"].set_external(
    grid.GetPointsArray(), grid.GetNumberOfPoints(), 0, 3 * sizeof(double));

  // velocity is stored in non-interlaced form (unlike points).
  fields_grid["velocity/values/x"].set_external(
    attribs.GetVelocityArray(), grid.GetNumberOfPoints(), /*offset=*/0);
  fields_grid["velocity/values/y"].set_external(attribs.GetVelocityArray(),
    grid.GetNumberOfPoints(),
    /*offset=*/grid.GetNumberOfPoints() * sizeof(double));
  fields_grid["velocity/values/z"].set_external(attribs.GetVelocityArray(),
    grid.GetNumberOfPoints(),
    /*offset=*/grid.GetNumberOfPoints() * sizeof(double) * 2);

  // pressure is cell-data.
  fields_grid["pressure/association"].set_string("element");
  fields_grid["pressure/topology"].set_string("mesh");
  fields_grid["pressure/volume_dependent"].set_string("false");
  fields_grid["pressure/values"].set_external(attribs.GetPressureArray(), grid.GetNumberOfCells());

  // now create 2st mesh named called "particles"
  auto mesh_particles = channel["data/particles"];
  mesh_particles["coordsets/coords/type"].set_string("explicit");
  mesh_particles["coordsets/coords/values/x"].set_external(particles.GetPointsArray(),
    particles.GetNumberOfPoints(), /*offset=*/0, /*stride=*/3 * sizeof(double));
  mesh_particles["coordsets/coords/values/y"].set_external(particles.GetPointsArray(),
    particles.GetNumberOfPoints(), /*offset=*/sizeof(double), /*stride=*/3 * sizeof(double));
  mesh_particles["coordsets/coords/values/z"].set_external(particles.GetPointsArray(),
    particles.GetNumberOfPoints(), /*offset=*/2 * sizeof(double), /*stride=*/3 * sizeof(double));

  // now, the topology.
  mesh_particles["topologies/mesh/type"].set_string("unstructured");
  mesh_particles["topologies/mesh/coordset"].set_string("coords");
  mesh_particles["topologies/mesh/elements/shape"].set_string("point");
  std::vector<conduit_int64> connectivity(particles.GetNumberOfPoints());
  std::iota(connectivity.begin(), connectivity.end(), 0);
  mesh_particles["topologies/mesh/elements/connectivity"].set_external(
    &connectivity[0], particles.GetNumberOfPoints());

  // now, add assembly
  // Assembly:
  //   > Mesh
  //      (grid)
  //   > Particles
  //      (particles)
  //   > Collection
  //      > Sub Collection
  //        [(grid), (particles)]
  auto assembly = channel["assembly"];
  assembly["Grid"].set_string("grid");
  assembly["Particles"].set_string("particles");
  auto subCollection = assembly["Collection/Sub Collection"];
  subCollection.append().set_string("grid");
  subCollection.append().set_string("particles");

  catalyst_status err = catalyst_execute(conduit_cpp::c_node(&exec_params));
  if (err != catalyst_status_ok)
  {
    std::cerr << "Failed to execute Catalyst: " << err << std::endl;
  }
}

void Finalize()
{
  conduit_cpp::Node node;
  catalyst_status err = catalyst_finalize(conduit_cpp::c_node(&node));
  if (err != catalyst_status_ok)
  {
    std::cerr << "Failed to finalize Catalyst: " << err << std::endl;
  }
}
}

#endif
