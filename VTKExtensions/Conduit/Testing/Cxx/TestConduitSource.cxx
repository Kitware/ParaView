/*=========================================================================

  Program:   ParaView
  Module:    TestConduitSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCellData.h"
#include "vtkConduitSource.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include <conduit_blueprint.hpp>

#define VERIFY(x, ...)                                                                             \
  if ((x) == false)                                                                                \
  {                                                                                                \
    vtkLogF(ERROR, __VA_ARGS__);                                                                   \
    return false;                                                                                  \
  }

namespace
{

vtkSmartPointer<vtkDataObject> Convert(const conduit::Node& node)
{
  vtkNew<vtkConduitSource> source;
  source->SetNode(&node);
  source->Update();
  return source->GetOutputDataObject(0);
}

bool ValidateMeshTypeUniform()
{
  conduit::Node mesh;
  conduit::blueprint::mesh::examples::basic("uniform", 3, 3, 3, mesh);
  auto data = Convert(mesh);
  VERIFY(vtkPartitionedDataSet::SafeDownCast(data) != nullptr,
    "incorrect data type, expected vtkPartitionedDataSet, got %s", vtkLogIdentifier(data));
  auto pds = vtkPartitionedDataSet::SafeDownCast(data);
  VERIFY(pds->GetNumberOfPartitions() == 1, "incorrect number of partitions, expected 1, got %d",
    pds->GetNumberOfPartitions());
  auto img = vtkImageData::SafeDownCast(pds->GetPartition(0));
  VERIFY(img != nullptr, "missing partition 0");
  VERIFY(vtkVector3i(img->GetDimensions()) == vtkVector3i(3, 3, 3),
    "incorrect dimensions, expected=3x3x3, got=%dx%dx%d", img->GetDimensions()[0],
    img->GetDimensions()[1], img->GetDimensions()[2]);

  return true;
}

bool ValidateMeshTypeRectilinear()
{
  conduit::Node mesh;
  conduit::blueprint::mesh::examples::basic("rectilinear", 3, 3, 3, mesh);
  auto data = Convert(mesh);
  VERIFY(vtkPartitionedDataSet::SafeDownCast(data) != nullptr,
    "incorrect data type, expected vtkPartitionedDataSet, got %s", vtkLogIdentifier(data));
  auto pds = vtkPartitionedDataSet::SafeDownCast(data);
  VERIFY(pds->GetNumberOfPartitions() == 1, "incorrect number of partitions, expected 1, got %d",
    pds->GetNumberOfPartitions());
  auto rg = vtkRectilinearGrid::SafeDownCast(pds->GetPartition(0));
  VERIFY(rg != nullptr, "missing partition 0");
  VERIFY(vtkVector3i(rg->GetDimensions()) == vtkVector3i(3, 3, 3),
    "incorrect dimensions, expected=3x3x3, got=%dx%dx%d", rg->GetDimensions()[0],
    rg->GetDimensions()[1], rg->GetDimensions()[2]);

  return true;
}

bool ValidateMeshTypeStructured()
{
  conduit::Node mesh;
  conduit::blueprint::mesh::examples::basic("structured", 3, 3, 3, mesh);
  auto data = Convert(mesh);
  VERIFY(vtkPartitionedDataSet::SafeDownCast(data) != nullptr,
    "incorrect data type, expected vtkPartitionedDataSet, got %s", vtkLogIdentifier(data));
  auto pds = vtkPartitionedDataSet::SafeDownCast(data);
  VERIFY(pds->GetNumberOfPartitions() == 1, "incorrect number of partitions, expected 1, got %d",
    pds->GetNumberOfPartitions());
  auto sg = vtkStructuredGrid::SafeDownCast(pds->GetPartition(0));
  VERIFY(sg != nullptr, "missing partition 0");
  VERIFY(vtkVector3i(sg->GetDimensions()) == vtkVector3i(3, 3, 3),
    "incorrect dimensions, expected=3x3x3, got=%dx%dx%d", sg->GetDimensions()[0],
    sg->GetDimensions()[1], sg->GetDimensions()[2]);

  return true;
}

bool ValidateMeshTypeUnstructured()
{
  conduit::Node mesh;
  // generate simple explicit tri-based 2d 'basic' mesh
  conduit::blueprint::mesh::examples::basic("tris", 3, 3, 0, mesh);

  auto data = Convert(mesh);
  VERIFY(vtkPartitionedDataSet::SafeDownCast(data) != nullptr,
    "incorrect data type, expected vtkPartitionedDataSet, got %s", vtkLogIdentifier(data));
  auto pds = vtkPartitionedDataSet::SafeDownCast(data);
  VERIFY(pds->GetNumberOfPartitions() == 1, "incorrect number of partitions, expected 1, got %d",
    pds->GetNumberOfPartitions());
  auto ug = vtkUnstructuredGrid::SafeDownCast(pds->GetPartition(0));
  VERIFY(ug != nullptr, "missing partition 0");

  VERIFY(ug->GetNumberOfPoints() == 9, "incorrect number of points, expected 9, got %lld",
    ug->GetNumberOfPoints());
  VERIFY(ug->GetNumberOfCells() == 8, "incorrect number of cells, expected 8, got %lld",
    ug->GetNumberOfCells());
  VERIFY(ug->GetCellData()->GetArray("field") != nullptr, "missing 'field' cell-data array");
  return true;
}
}

int TestConduitSource(int, char* [])
{
  return ValidateMeshTypeUniform() && ValidateMeshTypeRectilinear() &&
      ValidateMeshTypeStructured() && ValidateMeshTypeUnstructured()
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
}
