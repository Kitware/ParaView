// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef CatalystAdaptor_h
#define CatalystAdaptor_h

#include "FEDataStructures.h"
#include <catalyst.hpp>

#include <iostream>
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
    std::string file_path = argv[cc];
    if (file_path.size() > 4 && file_path.substr(file_path.size() - 4, 4) == ".xml")
    {
      node["catalyst/proxies/proxy" + std::to_string(cc - 1)].set_string(argv[cc]);
    }
    else
    {
      node["catalyst/scripts/script" + std::to_string(cc - 1)].set_string(argv[cc]);
    }
  }
  node["catalyst_load/implementation"] = "paraview";
  node["catalyst_load/search_paths/paraview"] = PARAVIEW_IMPL_DIR;
  catalyst_status err = catalyst_initialize(conduit_cpp::c_node(&node));
  if (err != catalyst_status_ok)
  {
    std::cerr << "Failed to initialize Catalyst: " << err << std::endl;
  }
}

void AddStateInformation(conduit_cpp::Node& exec_params, int cycle, double time)
{
  // add time/cycle information
  auto state = exec_params["catalyst/state"];
  state["timestep"].set(cycle);
  state["time"].set(time);
}

void AddGridChannel(conduit_cpp::Node& exec_params, Grid& grid, Attributes& attribs)
{

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
}

void AddSteerableChannel(conduit_cpp::Node& exec_params)
{
  auto steerable = exec_params["catalyst/channels/steerable"];
  steerable["type"].set("mesh");

  auto steerable_mesh = steerable["data"];
  steerable_mesh["coordsets/coords/type"].set_string("explicit");
  steerable_mesh["coordsets/coords/values/x"].set_float64_vector({ 1 });
  steerable_mesh["coordsets/coords/values/y"].set_float64_vector({ 2 });
  steerable_mesh["coordsets/coords/values/z"].set_float64_vector({ 3 });
  steerable_mesh["topologies/mesh/type"].set("unstructured");
  steerable_mesh["topologies/mesh/coordset"].set("coords");
  steerable_mesh["topologies/mesh/elements/shape"].set("point");
  steerable_mesh["topologies/mesh/elements/connectivity"].set_int32_vector({ 0 });
  steerable_mesh["fields/steerable/association"].set("vertex");
  steerable_mesh["fields/steerable/topology"].set("mesh");
  steerable_mesh["fields/steerable/volume_dependent"].set("false");
  steerable_mesh["fields/steerable/values"].set_int32_vector({ 2 });
}

void Execute(int cycle, double time, Grid& grid, Attributes& attribs)
{
  conduit_cpp::Node exec_params;

  AddStateInformation(exec_params, cycle, time);
  AddGridChannel(exec_params, grid, attribs);
  AddSteerableChannel(exec_params);

  catalyst_status err = catalyst_execute(conduit_cpp::c_node(&exec_params));
  if (err != catalyst_status_ok)
  {
    std::cerr << "Failed to execute Catalyst: " << err << std::endl;
  }
}

void Results(unsigned int timeStep)
{
  conduit_cpp::Node results;
  catalyst_status err = catalyst_results(conduit_cpp::c_node(&results));
  if (err != catalyst_status_ok)
  {
    std::cerr << "Failed to execute Catalyst: " << err << std::endl;
  }
  else
  {
    std::cout << "Result Node dump:" << std::endl;
    results.print();

    const std::string x_value_path = "catalyst/steerable/coordsets/coords/values/x";
    if (results.has_path(x_value_path))
    {
      double expected_value = timeStep * 0.1;
      auto node = results[x_value_path].as_float64_ptr();
      if (node[0] != expected_value)
      {
        std::cerr << "Wrong value: " << node[0] << " expected: " << expected_value << std::endl;
      }
    }
    else
    {
      std::cerr << "key: [" << x_value_path << "] not found!" << std::endl;
    }

    const std::string field_values_path = "catalyst/steerable/fields/type/values";
    if (results.has_path(field_values_path))
    {
      int expected_value = timeStep % 3;
      auto node = results[field_values_path].as_int_ptr();
      if (node[0] != expected_value)
      {
        std::cerr << "Wrong value: " << node[0] << " expected: " << expected_value << std::endl;
      }
    }
    else
    {
      std::cerr << "key: [" << field_values_path << "] not found!" << std::endl;
    }
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
