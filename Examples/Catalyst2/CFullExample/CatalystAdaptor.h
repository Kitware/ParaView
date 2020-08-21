#ifndef CatalystAdaptor_h
#define CatalystAdaptor_h

#include <catalyst.h>
#include <stdio.h>

//-----------------------------------------------------------------------------
/**
 * Initialize Catalyst.
 */
//-----------------------------------------------------------------------------
void do_catalyst_initialization(int argc, char* argv[])
{
  conduit_node* catalyst_init_params = conduit_node_create();
  // pass scripts pass on command line.
  for (int cc = 1; cc < argc; ++cc)
  {
    char buf[256];
    snprintf(buf, 256, "catalyst/scripts/script%d", (cc - 1));
    conduit_node_set_path_char8_str(catalyst_init_params, buf, argv[cc]);
  }
  catalyst_initialize(catalyst_init_params);
  conduit_node_destroy(catalyst_init_params);
}

//-----------------------------------------------------------------------------
/**
 * Execute per cycle
 */
//-----------------------------------------------------------------------------
void do_catalyt_execute(int cycle, double time, Grid* grid, Attributes* attribs)
{
  conduit_node* catalyst_exec_params = conduit_node_create();
  conduit_node_set_path_int64(catalyst_exec_params, "catalyst/state/timestep", cycle);
  // one can also use "catalyst/cycle" for the same purpose.
  // conduit_node_set_path_int64(catalyst_exec_params, "catalyst/state/cycle", cycle);
  conduit_node_set_path_float64(catalyst_exec_params, "catalyst/state/time", time);

  // the data must be provided on a named channel. the name is determined by the
  // simulation. for this one, we're calling it "grid".

  // declare the type of the channel; we're using Conduit Mesh Blueprint
  // to describe the mesh and fields.
  conduit_node_set_path_char8_str(catalyst_exec_params, "catalyst/channels/grid/type", "mesh");

  // now, create the mesh.
  conduit_node* mesh = conduit_node_create();

  // add coordsets
  conduit_node_set_path_char8_str(mesh, "coordsets/coords/type", "explicit");
  conduit_node_set_path_char8_str(mesh, "coordsets/coords/type", "explicit");
  conduit_node_set_path_external_float64_ptr_detailed(mesh, "coordsets/coords/values/x",
    /*data=*/grid->Points, /*num_elements=*/grid->NumberOfPoints, /*offset=*/0,
    /*stride=*/3 * sizeof(double), /*element_bytes=*/sizeof(double),
    /*endianness=*/CONDUIT_ENDIANNESS_DEFAULT_ID);
  conduit_node_set_path_external_float64_ptr_detailed(mesh, "coordsets/coords/values/y",
    /*data=*/grid->Points, /*num_elements=*/grid->NumberOfPoints, /*offset=*/1 * sizeof(double),
    /*stride=*/3 * sizeof(double), /*element_bytes=*/sizeof(double),
    /*endianness=*/CONDUIT_ENDIANNESS_DEFAULT_ID);
  conduit_node_set_path_external_float64_ptr_detailed(mesh, "coordsets/coords/values/z",
    /*data=*/grid->Points, /*num_elements=*/grid->NumberOfPoints, /*offset=*/2 * sizeof(double),
    /*stride=*/3 * sizeof(double), /*element_bytes=*/sizeof(double),
    /*endianness=*/CONDUIT_ENDIANNESS_DEFAULT_ID);

  // add topologies
  conduit_node_set_path_char8_str(mesh, "topologies/mesh/type", "unstructured");
  conduit_node_set_path_char8_str(mesh, "topologies/mesh/coordset", "coords");
  conduit_node_set_path_char8_str(mesh, "topologies/mesh/elements/shape", "hex");
  conduit_node_set_path_external_int64_ptr(
    mesh, "topologies/mesh/elements/connectivity", grid->Cells, grid->NumberOfCells * 8);

  // add velocity (point-field)
  conduit_node_set_path_char8_str(mesh, "fields/velocity/association", "vertex");
  conduit_node_set_path_char8_str(mesh, "fields/velocity/topology", "mesh");
  conduit_node_set_path_char8_str(mesh, "fields/velocity/volume_dependent", "false");
  conduit_node_set_path_external_float64_ptr_detailed(mesh, "fields/velocity/values/x",
    /*data=*/attribs->Velocity, /*num_elements=*/grid->NumberOfPoints, /*offset=*/0,
    /*stride=*/sizeof(double), /*element_bytes=*/sizeof(double),
    /*endianness*/ CONDUIT_ENDIANNESS_DEFAULT_ID);
  conduit_node_set_path_external_float64_ptr_detailed(mesh, "fields/velocity/values/y",
    /*data=*/attribs->Velocity, /*num_elements=*/grid->NumberOfPoints,
    /*offset=*/grid->NumberOfPoints * sizeof(double),
    /*stride=*/sizeof(double), /*element_bytes=*/sizeof(double),
    /*endianness*/ CONDUIT_ENDIANNESS_DEFAULT_ID);
  conduit_node_set_path_external_float64_ptr_detailed(mesh, "fields/velocity/values/z",
    /*data=*/attribs->Velocity, /*num_elements=*/grid->NumberOfPoints,
    /*offset=*/2 * grid->NumberOfPoints * sizeof(double),
    /*stride=*/sizeof(double), /*element_bytes=*/sizeof(double),
    /*endianness*/ CONDUIT_ENDIANNESS_DEFAULT_ID);

  // add pressure (cell-field)
  conduit_node_set_path_char8_str(mesh, "fields/pressure/association", "element");
  conduit_node_set_path_char8_str(mesh, "fields/pressure/topology", "mesh");
  conduit_node_set_path_char8_str(mesh, "fields/pressure/volume_dependent", "false");
  conduit_node_set_path_external_float32_ptr(
    mesh, "fields/pressure/values", attribs->Pressure, grid->NumberOfCells);
  conduit_node_set_path_external_node(catalyst_exec_params, "catalyst/channels/grid/data", mesh);

#if 0
  // print for debugging purposes, if needed
  conduit_node_print(catalyst_exec_params);

  // print information with details about memory allocation
  conduit_node* info = conduit_node_create();
  conduit_node_info(catalyst_exec_params, info);
  conduit_node_print(info);
  conduit_node_destroy(info);
#endif

  catalyst_execute(catalyst_exec_params);
  conduit_node_destroy(catalyst_exec_params);
  conduit_node_destroy(mesh);
}

//-----------------------------------------------------------------------------
/**
 * Finalize Catalyst.
 */
//-----------------------------------------------------------------------------
void do_catalyt_finalization()
{
  conduit_node* catalyst_fini_params = conduit_node_create();
  catalyst_finalize(catalyst_fini_params);
  conduit_node_destroy(catalyst_fini_params);
}

#endif
