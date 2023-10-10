// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef CatalystAdaptor_h
#define CatalystAdaptor_h

#include <catalyst.hpp>

#include "FEDataStructures.h"

#include <iostream>
#include <math.h>
#include <mpi.h>
#include <string>
#include <vector>

namespace CatalystAdaptor
{

/**
 * In this example, we show how to pass overlapping AMR data
 * into Conduit for ParaView Catalyst processing.
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

void Execute(unsigned int cycle, double time, AMR& amr)
{
  int numRanks(1), myRank(0);
  MPI_Comm_size(MPI_COMM_WORLD, &numRanks);
  MPI_Comm_rank(MPI_COMM_WORLD, &myRank);

  conduit_cpp::Node exec_params;

  // add time/cycle information
  auto state = exec_params["catalyst/state"];
  state["timestep"].set(cycle);
  state["time"].set(time);

  // Add channels.
  // We only have 1 channel here. Let's name it 'grid'.
  auto channel = exec_params["catalyst/channels/grid"];

  // Since this example is using Conduit Mesh Blueprint to define the mesh,
  // we set the channel's type to "amrmesh".
  channel["type"].set("amrmesh");

  // now create the mesh.
  conduit_cpp::Node mesh = channel["data"];

  for (unsigned int level = 0; level < amr.NumberOfAMRLevels; level++)
  {
    std::string patch_name = "domain_" + std::to_string(level + amr.NumberOfAMRLevels * myRank);
    conduit_cpp::Node patch = mesh[patch_name];
    // add basic state info
    patch["state/domain_id"] = level + amr.NumberOfAMRLevels * myRank;
    patch["state/cycle"] = cycle;
    patch["state/time"] = time;
    patch["state/level"] = level;

    patch["coordsets/coords/type"] = "uniform";
    std::array<int, 6> levelIndices = amr.GetLevelIndices(level);
    patch["coordsets/coords/dims/i"] = levelIndices[1] - levelIndices[0] + 1;
    patch["coordsets/coords/dims/j"] = levelIndices[3] - levelIndices[2] + 1;
    patch["coordsets/coords/dims/k"] = levelIndices[5] - levelIndices[4] + 1;

    patch["coordsets/coords/spacing/dx"] = 1. / std::pow(2, level);
    patch["coordsets/coords/spacing/dy"] = 1. / std::pow(2, level);
    patch["coordsets/coords/spacing/dz"] = 1. / std::pow(2, level);

    std::array<double, 3> levelOrigin = amr.GetLevelOrigin(level);
    patch["coordsets/coords/origin/x"] = levelOrigin[0];
    patch["coordsets/coords/origin/y"] = levelOrigin[1];
    patch["coordsets/coords/origin/z"] = levelOrigin[2];

    // create a rectilinear topology that refs our coordset
    patch["topologies/topo/type"] = "uniform";
    patch["topologies/topo/coordset"] = "coords";

    // add logical elements origin
    patch["topologies/topo/elements/origin/i0"] = levelIndices[0];
    patch["topologies/topo/elements/origin/j0"] = levelIndices[2];
    patch["topologies/topo/elements/origin/k0"] = levelIndices[4];

    conduit_cpp::Node nest_set;
    nest_set["association"] = "element";
    nest_set["topology"] = "topo";
    if (level > 0)
    {
      int parent_id = amr.BlockId[level - 1];
      std::string parent_name = "windows/window_" + std::to_string(parent_id);
      conduit_cpp::Node parent = nest_set[parent_name];
      parent["domain_id"] = parent_id;
      parent["domain_type"] = "parent";
      std::array<int, 6> parentLevelIndices = amr.GetLevelIndices(level - 1);
      parent["origin/i"] = levelIndices[0] / 2;
      parent["origin/j"] = parentLevelIndices[2];
      parent["origin/k"] = parentLevelIndices[4];
      parent["dims/i"] = parentLevelIndices[1] - levelIndices[0] / 2 + 1;
      parent["dims/j"] = parentLevelIndices[3] - parentLevelIndices[2] + 1;
      ;
      parent["dims/k"] = parentLevelIndices[5] - parentLevelIndices[4] + 1;
      ;
      parent["ratio/i"] = 2;
      parent["ratio/j"] = 2;
      parent["ratio/k"] = 2;
    }
    if (level < amr.NumberOfAMRLevels - 1)
    {
      int child_id = amr.BlockId[level];
      std::string child_name = "windows/window_" + std::to_string(child_id);
      conduit_cpp::Node child = nest_set[child_name];
      child["domain_id"] = child_id;
      child["domain_type"] = "child";

      child["origin/i"] = levelIndices[0];
      child["origin/j"] = levelIndices[2];
      child["origin/k"] = levelIndices[4];

      child["dims/i"] = levelIndices[1] - levelIndices[0] + 1;
      child["dims/j"] = levelIndices[3] - levelIndices[2] + 1;
      child["dims/k"] = levelIndices[5] - levelIndices[4] + 1;

      child["ratio/i"] = 2;
      child["ratio/j"] = 2;
      child["ratio/k"] = 2;
    }
    patch["nestsets/nest"].set(nest_set);
    // add fields
    conduit_cpp::Node fields = patch["fields"];

    // cell data corresponding to MPI process id
    conduit_cpp::Node proc_id_field = fields["procid"];
    proc_id_field["association"] = "element";
    proc_id_field["topology"] = "topo";
    int num_cells = (levelIndices[1] - levelIndices[0]) * (levelIndices[3] - levelIndices[2]) *
      (levelIndices[5] - levelIndices[4]);
    std::vector<int> cellValues(num_cells, myRank);
    // we copy the data since cellValues will get deallocated
    proc_id_field["values"] = cellValues;

    // point data that varies in time and X location.
    conduit_cpp::Node other_field = fields["otherfield"];
    other_field["association"] = "vertex";
    other_field["topology"] = "topo";
    int num_points = (levelIndices[1] - levelIndices[0] + 1) *
      (levelIndices[3] - levelIndices[2] + 1) * (levelIndices[5] - levelIndices[4] + 1);
    std::vector<double> point_values(num_points, 0);
    for (size_t k = 0; k < levelIndices[5] - levelIndices[4] + 1; k++)
    {
      for (size_t j = 0; j < levelIndices[3] - levelIndices[2] + 1; j++)
      {
        for (size_t i = 0; i < levelIndices[1] - levelIndices[0] + 1; i++)
        {
          size_t l = i + j * (levelIndices[1] - levelIndices[0] + 1) +
            k * (levelIndices[1] - levelIndices[0] + 1) * (levelIndices[3] - levelIndices[2] + 1);
          double spacing = 1. / std::pow(2, level);
          double xOrigin = levelOrigin[0];
          point_values[l] = xOrigin + i * spacing * std::cos(time);
        }
      }
    }
    // we copy the data since point_values will get deallocated
    other_field["values"] = point_values;
  }

  // exec_params.print(); // for viewing the Conduit node information

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
