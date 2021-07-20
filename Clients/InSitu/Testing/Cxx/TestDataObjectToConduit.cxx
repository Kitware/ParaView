/*=========================================================================

  Program:   ParaView
  Module:    TestDataObjectToConduit.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDataObjectToConduit.h"

#include <catalyst_conduit.hpp>

#include <vtkDoubleArray.h>
#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkRectilinearGrid.h>
#include <vtkTable.h>

bool TestNonDataSetObject()
{
  conduit_cpp::Node node;
  vtkNew<vtkTable> table;

  bool is_table_supported = vtkDataObjectToConduit::FillConduitNode(table, node);

  return !is_table_supported;
}

bool TestImageData()
{
  conduit_cpp::Node node;
  vtkNew<vtkImageData> image;

  image->SetDimensions(2, 3, 1);
  image->SetSpacing(10, 20, 30);
  image->SetOrigin(-1, -2, -3);
  image->AllocateScalars(VTK_INT, 1);
  int* dims = image->GetDimensions();

  for (int z = 0; z < dims[2]; z++)
  {
    for (int y = 0; y < dims[1]; y++)
    {
      for (int x = 0; x < dims[0]; x++)
      {
        int* pixel = static_cast<int*>(image->GetScalarPointer(x, y, z));
        pixel[0] = 2;
      }
    }
  }

  bool is_success =
    vtkDataObjectToConduit::FillConduitNode(vtkDataObject::SafeDownCast(image), node);

  if (is_success)
  {
    conduit_cpp::Node expected_node;
    auto coords_node = expected_node["coordsets/coords"];
    coords_node["type"] = "uniform";
    coords_node["dims/i"] = 2;
    coords_node["dims/j"] = 3;
    coords_node["dims/k"] = 1;
    coords_node["origin/x"] = -1.0;
    coords_node["origin/y"] = -2.0;
    coords_node["origin/z"] = -3.0;
    coords_node["spacing/dx"] = 10.0;
    coords_node["spacing/dy"] = 20.0;
    coords_node["spacing/dz"] = 30.0;

    auto topologies_node = expected_node["topologies/mesh"];
    topologies_node["type"] = "uniform";
    topologies_node["coordset"] = "coords";

    auto field_node = expected_node["fields/ImageScalars"];
    field_node["association"] = "vertex";
    field_node["topology"] = "mesh";
    field_node["volume_dependent"] = "false";
    field_node["values"] = std::vector<int>{ 2, 2, 2, 2, 2, 2 };

    conduit_cpp::Node diff_info;
    bool are_nodes_different = node.diff(expected_node, diff_info, 1e-6);
    if (are_nodes_different)
    {
      diff_info.print();
    }

    is_success = !are_nodes_different;
  }

  return is_success;
}

bool TestRectilinearGrid()
{
  conduit_cpp::Node node;
  vtkNew<vtkRectilinearGrid> rectilinear_grid;

  rectilinear_grid->SetDimensions(2, 3, 1);

  vtkNew<vtkDoubleArray> xArray;
  xArray->InsertNextValue(0.0);
  xArray->InsertNextValue(2.0);
  rectilinear_grid->SetXCoordinates(xArray);

  vtkNew<vtkDoubleArray> yArray;
  yArray->InsertNextValue(0.0);
  yArray->InsertNextValue(1.0);
  yArray->InsertNextValue(2.0);
  rectilinear_grid->SetYCoordinates(yArray);

  vtkNew<vtkDoubleArray> zArray;
  zArray->InsertNextValue(0.0);
  rectilinear_grid->SetZCoordinates(zArray);

  vtkNew<vtkDoubleArray> fieldArray;
  fieldArray->SetName("rectilinear_field");
  fieldArray->SetNumberOfComponents(2);
  fieldArray->SetNumberOfTuples(6);
  for (int tuple_index = 0; tuple_index < 6; ++tuple_index)
  {
    fieldArray->SetTuple2(tuple_index, tuple_index, tuple_index * 2);
  }

  rectilinear_grid->GetPointData()->AddArray(fieldArray);

  bool is_success =
    vtkDataObjectToConduit::FillConduitNode(vtkDataObject::SafeDownCast(rectilinear_grid), node);

  if (is_success)
  {
    conduit_cpp::Node expected_node;
    auto coords_node = expected_node["coordsets/coords"];
    coords_node["type"] = "rectilinear";
    coords_node["values/x"] = std::vector<double>{ 0, 2 };
    coords_node["values/y"] = std::vector<double>{ 0, 1, 2 };
    coords_node["values/z"] = std::vector<double>{ 0 };

    auto topologies_node = expected_node["topologies/mesh"];
    topologies_node["type"] = "rectilinear";
    topologies_node["coordset"] = "coords";

    auto field_node = expected_node["fields/rectilinear_field"];
    field_node["association"] = "vertex";
    field_node["topology"] = "mesh";
    field_node["volume_dependent"] = "false";
    field_node["values"] = std::vector<double>{ 0, 0, 1, 2, 2, 4, 3, 6, 4, 8, 5, 10 };

    conduit_cpp::Node diff_info;
    bool are_nodes_different = node.diff(expected_node, diff_info, 1e-6);
    if (are_nodes_different)
    {
      diff_info.print();
    }

    is_success = !are_nodes_different;
  }

  return is_success;
}

bool TestStructuredGrid()
{
  return true;
}

bool TestUnstructuredGrid()
{
  return true;
}

bool TestPolydata()
{
  return true;
}

bool TestExplicitStructuredGrid()
{
  return true;
}

int TestDataObjectToConduit(int, char* [])
{
  bool is_success = true;

  is_success &= TestNonDataSetObject();
  is_success &= TestImageData();
  is_success &= TestRectilinearGrid();
  is_success &= TestStructuredGrid();
  is_success &= TestUnstructuredGrid();
  is_success &= TestPolydata();
  is_success &= TestExplicitStructuredGrid();

  return is_success ? 0 : 1;
}
