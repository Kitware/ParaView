/*=========================================================================

  Program:   ParaView
  Module:    vtkDataObjectToConduit.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataObjectToConduit.h"

#include "vtkAOSDataArrayTemplate.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSOADataArrayTemplate.h"
#include "vtkTypeFloat32Array.h"
#include "vtkTypeFloat64Array.h"
#include "vtkTypeInt16Array.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeInt64Array.h"
#include "vtkTypeInt8Array.h"
#include "vtkTypeUInt16Array.h"
#include "vtkTypeUInt32Array.h"
#include "vtkTypeUInt64Array.h"
#include "vtkTypeUInt8Array.h"

#include <catalyst_conduit.hpp>

//----------------------------------------------------------------------------
vtkDataObjectToConduit::vtkDataObjectToConduit() = default;

//----------------------------------------------------------------------------
vtkDataObjectToConduit::~vtkDataObjectToConduit() = default;

//----------------------------------------------------------------------------
bool vtkDataObjectToConduit::FillConduitNode(
  vtkDataObject* data_object, conduit_cpp::Node& conduit_node)
{
  auto data_set = vtkDataSet::SafeDownCast(data_object);
  if (!data_set)
  {
    vtkLogF(ERROR, "Only Data Set objects are supported in vtkDataObjectToConduit.");
    return false;
  }

  return FillConduitNode(data_set, conduit_node);
}

//----------------------------------------------------------------------------
bool vtkDataObjectToConduit::FillConduitNode(vtkDataSet* data_set, conduit_cpp::Node& conduit_node)
{
  bool is_success = FillTopology(data_set, conduit_node);
  if (is_success)
  {
    is_success = FillFields(data_set, conduit_node);
  }

  return is_success;
}

//----------------------------------------------------------------------------
bool vtkDataObjectToConduit::FillTopology(vtkDataSet* data_set, conduit_cpp::Node& conduit_node)
{
  bool is_success = true;

  if (auto imageData = vtkImageData::SafeDownCast(data_set))
  {
    auto coords_node = conduit_node["coordsets/coords"];

    coords_node["type"] = "uniform";

    int* dimensions = imageData->GetDimensions();
    coords_node["dims/i"] = dimensions[0];
    coords_node["dims/j"] = dimensions[1];
    coords_node["dims/k"] = dimensions[2];

    double* origin = imageData->GetOrigin();
    coords_node["origin/x"] = origin[0];
    coords_node["origin/y"] = origin[1];
    coords_node["origin/z"] = origin[2];

    double* spacing = imageData->GetSpacing();
    coords_node["spacing/dx"] = spacing[0];
    coords_node["spacing/dy"] = spacing[1];
    coords_node["spacing/dz"] = spacing[2];

    auto topologies_node = conduit_node["topologies/mesh"];
    topologies_node["type"] = "uniform";
    topologies_node["coordset"] = "coords";
  }
  else if (auto rectilinear_grid = vtkRectilinearGrid::SafeDownCast(data_set))
  {
    auto coords_node = conduit_node["coordsets/coords"];

    coords_node["type"] = "rectilinear";

    auto x_values_node = coords_node["values/x"];
    is_success = ConvertDataArrayToMCArray(rectilinear_grid->GetXCoordinates(), x_values_node);
    if (is_success)
    {
      auto y_values_node = coords_node["values/y"];
      is_success = ConvertDataArrayToMCArray(rectilinear_grid->GetYCoordinates(), y_values_node);
    }
    if (is_success)
    {
      auto z_values_node = coords_node["values/z"];
      is_success = ConvertDataArrayToMCArray(rectilinear_grid->GetZCoordinates(), z_values_node);
    }

    if (is_success)
    {
      auto topologies_node = conduit_node["topologies/mesh"];
      topologies_node["type"] = "rectilinear";
      topologies_node["coordset"] = "coords";
    }
  }
  else
  {
    vtkLogF(ERROR, "Unsupported type.");
    is_success = false;
  }
  return is_success;
}

//----------------------------------------------------------------------------
bool vtkDataObjectToConduit::FillFields(vtkDataSet* data_set, conduit_cpp::Node& conduit_node)
{
  bool is_success = true;

  if (auto cell_data = data_set->GetCellData())
  {
    is_success = FillFields(cell_data, "element", conduit_node);
  }

  if (is_success)
  {
    if (auto point_data = data_set->GetPointData())
    {
      is_success = FillFields(point_data, "vertex", conduit_node);
    }
  }

  if (is_success)
  {
    if (auto field_data = data_set->GetFieldData())
    {
      // Ignore them for now.
    }
  }

  return is_success;
}

//----------------------------------------------------------------------------
bool vtkDataObjectToConduit::FillFields(
  vtkFieldData* field_data, const std::string& association, conduit_cpp::Node& conduit_node)
{
  bool is_success = true;

  int array_count = field_data->GetNumberOfArrays();
  for (int array_index = 0; is_success && array_index < array_count; ++array_index)
  {
    auto array = field_data->GetArray(array_index);
    auto field_node = conduit_node["fields"][array->GetName()];
    field_node["association"] = association;
    field_node["topology"] = "mesh";
    field_node["volume_dependent"] = "false";

    auto values_node = field_node["values"];
    is_success = ConvertDataArrayToMCArray(array, values_node);
  }

  return is_success;
}

//----------------------------------------------------------------------------
bool vtkDataObjectToConduit::ConvertDataArrayToMCArray(
  vtkDataArray* data_array, conduit_cpp::Node& conduit_node)
{
  bool is_success = true;

  if (auto soa_data_array = vtkSOADataArrayTemplate<vtkTypeInt8>::SafeDownCast(data_array))
  {
    conduit_node.set_external_int8_ptr(
      soa_data_array->GetPointer(0), soa_data_array->GetNumberOfValues());
  }
  else if (auto soa_data_array = vtkSOADataArrayTemplate<vtkTypeInt16>::SafeDownCast(data_array))
  {
    conduit_node.set_external_int16_ptr(
      soa_data_array->GetPointer(0), soa_data_array->GetNumberOfValues());
  }
  else if (auto soa_data_array = vtkSOADataArrayTemplate<vtkTypeInt32>::SafeDownCast(data_array))
  {
    conduit_node.set_external_int32_ptr(
      soa_data_array->GetPointer(0), soa_data_array->GetNumberOfValues());
  }
  else if (auto soa_data_array = vtkSOADataArrayTemplate<long>::SafeDownCast(data_array))
  {
    conduit_node.set_external_int64_ptr(
      soa_data_array->GetPointer(0), soa_data_array->GetNumberOfValues());
  }
  else if (auto soa_data_array = vtkSOADataArrayTemplate<vtkTypeUInt8>::SafeDownCast(data_array))
  {
    conduit_node.set_external_uint8_ptr(
      soa_data_array->GetPointer(0), soa_data_array->GetNumberOfValues());
  }
  else if (auto soa_data_array = vtkSOADataArrayTemplate<vtkTypeUInt16>::SafeDownCast(data_array))
  {
    conduit_node.set_external_uint16_ptr(soa_data_array->GetPointer(0),
      soa_data_array->GetNumberOfValues(), 0, soa_data_array->GetNumberOfComponents(), 0, 0);
  }
  else if (auto soa_data_array = vtkSOADataArrayTemplate<vtkTypeUInt32>::SafeDownCast(data_array))
  {
    conduit_node.set_external_uint32_ptr(soa_data_array->GetPointer(0),
      soa_data_array->GetNumberOfValues(), 0, soa_data_array->GetNumberOfComponents(), 0, 0);
  }
  else if (auto soa_data_array = vtkSOADataArrayTemplate<unsigned long>::SafeDownCast(data_array))
  {
    conduit_node.set_external_uint64_ptr(
      soa_data_array->GetPointer(0), soa_data_array->GetNumberOfValues());
  }
  else if (auto soa_data_array = vtkSOADataArrayTemplate<vtkTypeFloat32>::SafeDownCast(data_array))
  {
    conduit_node.set_external_float32_ptr(
      soa_data_array->GetPointer(0), soa_data_array->GetNumberOfValues());
  }
  else if (auto soa_data_array = vtkSOADataArrayTemplate<vtkTypeFloat64>::SafeDownCast(data_array))
  {
    conduit_node.set_external_float64_ptr(
      soa_data_array->GetPointer(0), soa_data_array->GetNumberOfValues());
  }
  else if (auto aos_data_array = vtkAOSDataArrayTemplate<vtkTypeInt8>::SafeDownCast(data_array))
  {
    conduit_node.set_external_int8_ptr(
      aos_data_array->GetPointer(0), aos_data_array->GetNumberOfValues());
  }
  else if (auto aos_data_array = vtkAOSDataArrayTemplate<vtkTypeInt16>::SafeDownCast(data_array))
  {
    conduit_node.set_external_int16_ptr(
      aos_data_array->GetPointer(0), aos_data_array->GetNumberOfValues());
  }
  else if (auto aos_data_array = vtkAOSDataArrayTemplate<vtkTypeInt32>::SafeDownCast(data_array))
  {
    conduit_node.set_external_int32_ptr(
      aos_data_array->GetPointer(0), aos_data_array->GetNumberOfValues());
  }
  else if (auto aos_data_array = vtkAOSDataArrayTemplate<vtkTypeInt64>::SafeDownCast(data_array))
  {
    // conduit_node.set_external_int64_ptr(aos_data_array->GetPointer(0),
    //  aos_data_array->GetNumberOfValues(), 0, aos_data_array->GetNumberOfComponents(), 0, 0);
  }
  else if (auto aos_data_array = vtkAOSDataArrayTemplate<vtkTypeUInt8>::SafeDownCast(data_array))
  {
    conduit_node.set_external_uint8_ptr(
      aos_data_array->GetPointer(0), aos_data_array->GetNumberOfValues());
  }
  else if (auto aos_data_array = vtkAOSDataArrayTemplate<vtkTypeUInt16>::SafeDownCast(data_array))
  {
    conduit_node.set_external_uint16_ptr(
      aos_data_array->GetPointer(0), aos_data_array->GetNumberOfValues());
  }
  else if (auto aos_data_array = vtkAOSDataArrayTemplate<vtkTypeUInt32>::SafeDownCast(data_array))
  {
    conduit_node.set_external_uint32_ptr(
      aos_data_array->GetPointer(0), aos_data_array->GetNumberOfValues());
  }
  else if (auto aos_data_array = vtkAOSDataArrayTemplate<vtkTypeUInt64>::SafeDownCast(data_array))
  {
    // conduit_node.set_external_uint64_ptr(aos_data_array->GetPointer(0),
    //  aos_data_array->GetNumberOfValues(), 0, aos_data_array->GetNumberOfComponents(), 0, 0);
  }
  else if (auto aos_data_array = vtkAOSDataArrayTemplate<vtkTypeFloat32>::SafeDownCast(data_array))
  {
    conduit_node.set_external_float32_ptr(
      aos_data_array->GetPointer(0), aos_data_array->GetNumberOfValues());
  }
  else if (auto aos_data_array = vtkAOSDataArrayTemplate<vtkTypeFloat64>::SafeDownCast(data_array))
  {
    conduit_node.set_external_float64_ptr(
      aos_data_array->GetPointer(0), aos_data_array->GetNumberOfValues());
  }
  else
  {
    vtkLogF(ERROR, "Unsupported data array type.");
    is_success = false;
  }

  return is_success;
}

//----------------------------------------------------------------------------
void vtkDataObjectToConduit::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
