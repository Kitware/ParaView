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

#include <catalyst_conduit_blueprint.hpp>

#define VERIFY(x, ...)                                                                             \
  if ((x) == false)                                                                                \
  {                                                                                                \
    vtkLogF(ERROR, __VA_ARGS__);                                                                   \
    return false;                                                                                  \
  }

namespace
{

vtkSmartPointer<vtkDataObject> Convert(const conduit_cpp::Node& node)
{
  vtkNew<vtkConduitSource> source;
  source->SetNode(conduit_cpp::c_node(&node));
  source->Update();
  return source->GetOutputDataObject(0);
}

bool ValidateMeshTypeUniform()
{
  conduit_cpp::Node mesh;
  conduit_cpp::BlueprintMesh::Example::basic("uniform", 3, 3, 3, mesh);
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
  conduit_cpp::Node mesh;
  conduit_cpp::BlueprintMesh::Example::basic("rectilinear", 3, 3, 3, mesh);
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
  conduit_cpp::Node mesh;
  conduit_cpp::BlueprintMesh::Example::basic("structured", 3, 3, 3, mesh);
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
  conduit_cpp::Node mesh;
  // generate simple explicit tri-based 2d 'basic' mesh
  conduit_cpp::BlueprintMesh::Example::basic("tris", 3, 3, 0, mesh);

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

bool ValidateFieldData()
{
  conduit_cpp::Node mesh;
  conduit_cpp::BlueprintMesh::Example::basic("uniform", 3, 3, 3, mesh);

  auto field_data_node = mesh["state/fields"];

  auto empty_field_data = field_data_node["empty_field_data"];

  auto integer_field_data = field_data_node["integer_field_data"];
  integer_field_data.set_int64(42);

  auto float_field_data = field_data_node["float_field_data"];
  float_field_data.set_float64(5.0);

  auto string_field_data = field_data_node["string_field_data"];
  string_field_data.set_string("test");

  auto integer_vector_field_data = field_data_node["integer_vector_field_data"];
  integer_vector_field_data.set_int64_vector({ 1, 2, 3 });

  auto float_vector_field_data = field_data_node["float_vector_field_data"];
  float_vector_field_data.set_float64_vector({ 4.0, 5.0, 6.0 });

  std::vector<int> integer_buffer = { 123, 456, 789 };
  auto external_integer_vector_field_data = field_data_node["external_integer_vector"];
  external_integer_vector_field_data.set_external_int32_ptr(
    integer_buffer.data(), integer_buffer.size());

  auto data = Convert(mesh);
  auto field_data = data->GetFieldData();

  VERIFY(field_data->GetNumberOfArrays() == 6,
    "incorrect number of arrays in field data, expected 6, got %d",
    field_data->GetNumberOfArrays());

  auto integer_field_array = field_data->GetAbstractArray(0);
  VERIFY(std::string(integer_field_array->GetName()) == "integer_field_data",
    "wrong array name, expected \"integer_field_data\", got %s", integer_field_array->GetName());
  VERIFY(integer_field_array->GetNumberOfComponents() == 1, "wrong number of component");
  VERIFY(integer_field_array->GetNumberOfTuples() == 1, "wrong number of tuples");
  VERIFY(integer_field_array->GetVariantValue(0).ToInt() == 42, "wrong value");

  auto float_field_array = field_data->GetAbstractArray(1);
  VERIFY(std::string(float_field_array->GetName()) == "float_field_data",
    "wrong array name, expected \"float_field_data\", got %s", float_field_array->GetName());
  VERIFY(float_field_array->GetNumberOfComponents() == 1, "wrong number of component");
  VERIFY(float_field_array->GetNumberOfTuples() == 1, "wrong number of tuples");
  VERIFY(float_field_array->GetVariantValue(0).ToFloat() == 5.0, "wrong value");

  auto string_field_array = field_data->GetAbstractArray(2);
  VERIFY(std::string(string_field_array->GetName()) == "string_field_data",
    "wrong array name, expected \"string_field_data\", got %s", string_field_array->GetName());
  VERIFY(string_field_array->GetNumberOfComponents() == 1, "wrong number of component");
  VERIFY(string_field_array->GetNumberOfTuples() == 1, "wrong number of tuples");
  VERIFY(string_field_array->GetVariantValue(0).ToString() == "test", "wrong value");

  auto integer_vector_field_array = field_data->GetAbstractArray(3);
  VERIFY(std::string(integer_vector_field_array->GetName()) == "integer_vector_field_data",
    "wrong array name, expected \"integer_vector_field_data\", got %s",
    integer_vector_field_array->GetName());
  VERIFY(integer_vector_field_array->GetNumberOfComponents() == 1, "wrong number of component");
  VERIFY(integer_vector_field_array->GetNumberOfTuples() == 3, "wrong number of tuples");
  VERIFY(integer_vector_field_array->GetVariantValue(0).ToInt() == 1, "wrong value");
  VERIFY(integer_vector_field_array->GetVariantValue(1).ToInt() == 2, "wrong value");
  VERIFY(integer_vector_field_array->GetVariantValue(2).ToInt() == 3, "wrong value");

  auto float_vector_field_array = field_data->GetAbstractArray(4);
  VERIFY(std::string(float_vector_field_array->GetName()) == "float_vector_field_data",
    "wrong array name, expected \"float_vector_field_data\", got %s",
    float_vector_field_array->GetName());
  VERIFY(float_vector_field_array->GetNumberOfComponents() == 1, "wrong number of component");
  VERIFY(float_vector_field_array->GetNumberOfTuples() == 3, "wrong number of tuples");
  VERIFY(float_vector_field_array->GetVariantValue(0).ToInt() == 4.0, "wrong value");
  VERIFY(float_vector_field_array->GetVariantValue(1).ToInt() == 5.0, "wrong value");
  VERIFY(float_vector_field_array->GetVariantValue(2).ToInt() == 6.0, "wrong value");

  auto external_integer_vector_field_array = field_data->GetAbstractArray(5);
  VERIFY(std::string(external_integer_vector_field_array->GetName()) == "external_integer_vector",
    "wrong array name, expected \"external_integer_vector\", got %s",
    external_integer_vector_field_array->GetName());
  VERIFY(
    external_integer_vector_field_array->GetNumberOfComponents() == 1, "wrong number of component");
  VERIFY(external_integer_vector_field_array->GetNumberOfTuples() == 3, "wrong number of tuples");
  VERIFY(external_integer_vector_field_array->GetVariantValue(0).ToInt() == 123, "wrong value");
  VERIFY(external_integer_vector_field_array->GetVariantValue(1).ToInt() == 456, "wrong value");
  VERIFY(external_integer_vector_field_array->GetVariantValue(2).ToInt() == 789, "wrong value");

  return true;
}
}

int TestConduitSource(int, char*[])
{
  return ValidateMeshTypeUniform() && ValidateMeshTypeRectilinear() &&
      ValidateMeshTypeStructured() && ValidateMeshTypeUnstructured() && ValidateFieldData()
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
}
