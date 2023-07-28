// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkGmshIncludes_h
#define vtkGmshIncludes_h

#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif

// Gmsh headers
#include "Context.h" // for CTX
#include "GModel.h"
#include "GmshGlobal.h" // For GmshInitialize()
#include "MHexahedron.h"
#include "MPrism.h"
#include "MPyramid.h"
#include "MQuadrangle.h"
#include "MTetrahedron.h"
#include "MTriangle.h"
#include "OpenFile.h" // For OpenProject and MergeFile
#include "Options.h"  // for opt_mesh_partition_create_topology()
#include "PView.h"
#include "PViewDataGModel.h"
#include "adaptiveData.h"

#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#pragma GCC diagnostic pop
#endif

#endif
// VTK-HeaderTest-Exclude: vtkGmshIncludes.h
