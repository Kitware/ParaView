/*=========================================================================

  Program:   ParaView
  Module:    vtkConduitSource.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkConduitSource.h"

#include "vtkCellArray.h"
#include "vtkConduitArrayUtilities.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPoints.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"

#include <conduit.hpp>
#include <conduit_blueprint.hpp>
#include <conduit_cpp_to_c.hpp>

#include <algorithm>
namespace internals
{

//----------------------------------------------------------------------------
int GetAssociation(const std::string& assoc)
{
  if (assoc == "element")
  {
    return vtkDataObject::CELL;
  }
  else if (assoc == "vertex")
  {
    return vtkDataObject::POINT;
  }

  throw std::runtime_error("unsupported association " + assoc);
}

//----------------------------------------------------------------------------
int GetCellType(const std::string& shape)
{
  if (shape == "point")
  {
    return VTK_VERTEX;
  }
  else if (shape == "line")
  {
    return VTK_LINE;
  }
  else if (shape == "tri")
  {
    return VTK_TRIANGLE;
  }
  else if (shape == "quad")
  {
    return VTK_QUAD;
  }
  else if (shape == "tet")
  {
    return VTK_TETRA;
  }
  else if (shape == "hex")
  {
    return VTK_HEXAHEDRON;
  }
  else
  {
    throw std::runtime_error("unsupported shape " + shape);
  }
}

//----------------------------------------------------------------------------
// internal: get number of points in VTK cell type.
static vtkIdType GetNumberOfPointsInCellType(int vtk_cell_type)
{
  switch (vtk_cell_type)
  {
    case VTK_VERTEX:
      return 1;
    case VTK_LINE:
      return 2;
    case VTK_TRIANGLE:
      return 3;
    case VTK_QUAD:
    case VTK_TETRA:
      return 4;
    case VTK_HEXAHEDRON:
      return 8;
    default:
      throw std::runtime_error("unsupported cell type " + std::to_string(vtk_cell_type));
  }
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkPoints> CreatePoints(const conduit::Node& coords)
{
  if (coords["type"].as_string() != "explicit")
  {
    throw std::runtime_error("invalid node!");
  }

  auto array = vtkConduitArrayUtilities::MCArrayToVTKArray(&coords["values"], "coords");
  if (array == nullptr)
  {
    throw std::runtime_error("failed to convert to VTK array!");
  }
  if (array->GetNumberOfComponents() < 3)
  {
    array = vtkConduitArrayUtilities::SetNumberOfComponents(array, 3);
  }
  else if (array->GetNumberOfComponents() > 3)
  {
    throw std::runtime_error("points cannot have more than 3 components!");
  }

  vtkNew<vtkPoints> pts;
  pts->SetData(array);
  return pts;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataSet> GetMesh(
  const conduit::Node& topologyNode, const conduit::Node& coordsets)
{
  // get the coordset for this topology element.
  auto& coords = coordsets[topologyNode["coordset"].as_string()];
  if (topologyNode["type"].as_string() == "uniform" && coords["type"].as_string() == "uniform")
  {
    vtkNew<vtkImageData> img;
    int dims[3] = { 1, 1, 1 };
    const char* dims_paths[] = { "dims/i", "dims/j", "dims/k" };
    double origin[3] = { 0, 0, 0 };
    const char* origin_paths[] = { "origin/x", "origin/y", "origin/z" };
    double spacing[3] = { 1, 1, 1 };
    const char* spacing_paths[] = { "spacing/dx", "spacing/dy", "spacing/dz" };
    for (int cc = 0; cc < 3; ++cc)
    {
      if (coords.has_path(dims_paths[cc]))
      {
        dims[cc] = coords[dims_paths[cc]].to_int32();
      }
      if (coords.has_path(origin_paths[cc]))
      {
        origin[cc] = coords[origin_paths[cc]].to_double();
      }
      if (coords.has_path(spacing_paths[cc]))
      {
        spacing[cc] = coords[spacing_paths[cc]].to_double();
      }
    }
    img->SetOrigin(origin);
    img->SetSpacing(spacing);
    img->SetDimensions(dims);
    return img;
  }
  else if (topologyNode["type"].as_string() == "rectilinear" &&
    coords["type"].as_string() == "rectilinear")
  {
    vtkNew<vtkRectilinearGrid> rg;
    auto xArray = vtkConduitArrayUtilities::MCArrayToVTKArray(&coords["values/x"], "xcoords");
    auto yArray = vtkConduitArrayUtilities::MCArrayToVTKArray(&coords["values/y"], "ycoords");
    auto zArray = vtkConduitArrayUtilities::MCArrayToVTKArray(&coords["values/z"], "zcoords");
    rg->SetDimensions(
      xArray->GetNumberOfTuples(), yArray->GetNumberOfTuples(), zArray->GetNumberOfTuples());
    rg->SetXCoordinates(xArray);
    rg->SetYCoordinates(xArray);
    rg->SetZCoordinates(xArray);
    return rg;
  }
  else if (topologyNode["type"].as_string() == "structured" &&
    coords["type"].as_string() == "explicit")
  {
    vtkNew<vtkStructuredGrid> sg;
    sg->SetPoints(CreatePoints(coords));
    sg->SetDimensions(
      topologyNode.has_path("elements/dims/i") ? topologyNode["elements/dims/i"].to_int32() + 1 : 1,
      topologyNode.has_path("elements/dims/j") ? topologyNode["elements/dims/j"].to_int32() + 1 : 1,
      topologyNode.has_path("elements/dims/k") ? topologyNode["elements/dims/k"].to_int32() + 1
                                               : 1);
    return sg;
  }
  else if (topologyNode["type"].as_string() == "unstructured" &&
    coords["type"].as_string() == "explicit")
  {
    vtkNew<vtkUnstructuredGrid> ug;
    ug->SetPoints(CreatePoints(coords));
    const auto vtk_cell_type = GetCellType(topologyNode["elements/shape"].as_string());
    const auto cell_size = GetNumberOfPointsInCellType(vtk_cell_type);
    auto cellArray = vtkConduitArrayUtilities::MCArrayToVTKCellArray(
      cell_size, &topologyNode["elements/connectivity"]);
    ug->SetCells(vtk_cell_type, cellArray);
    return ug;
  }
  else
  {
    throw std::runtime_error("unsupported topology or coordset");
  }
}

} // namespace internals

class vtkConduitSource::vtkInternals
{
public:
  const conduit::Node* Node = nullptr;
  const conduit::Node* GlobalFieldsNode = nullptr;
};

vtkStandardNewMacro(vtkConduitSource);
//----------------------------------------------------------------------------
vtkConduitSource::vtkConduitSource()
  : Internals(new vtkConduitSource::vtkInternals())
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkConduitSource::~vtkConduitSource() = default;

//----------------------------------------------------------------------------
void vtkConduitSource::SetNode(const conduit_node* node)
{
  const conduit::Node* cpp_node = conduit::cpp_node(node);
  auto& internals = (*this->Internals);
  if (internals.Node != cpp_node)
  {
    internals.Node = cpp_node;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkConduitSource::SetGlobalFieldsNode(const conduit_node* node)
{
  const conduit::Node* cpp_node = conduit::cpp_node(node);
  auto& internals = (*this->Internals);
  if (internals.GlobalFieldsNode != cpp_node)
  {
    internals.GlobalFieldsNode = cpp_node;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkConduitSource::FillOutputPortInformation(int, vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPartitionedDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkConduitSource::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  auto output = vtkPartitionedDataSet::GetData(outputVector, 0);
  auto& internals = (*this->Internals);
  if (internals.Node == nullptr)
  {
    vtkWarningMacro("Missing Conduit Node!");
    return 1;
  }

  auto& node = (*internals.Node);
  conduit::Node info;
  if (!conduit::blueprint::mesh::verify(node, info))
  {
    vtkLogF(ERROR, "Mesh blueprint verification failed!\nDetails:\n%s", info.to_json().c_str());
    return 0;
  }

  std::map<std::string, vtkSmartPointer<vtkDataSet> > datasets;

  // process "topologies".
  auto& topologies = node["topologies"];
  auto iter = topologies.children();
  while (iter.has_next())
  {
    iter.next();
    try
    {
      if (auto ds = internals::GetMesh(iter.node(), node["coordsets"]))
      {
        auto idx = output->GetNumberOfPartitions();
        output->SetPartition(idx, ds);
        output->GetMetaData(idx)->Set(vtkCompositeDataSet::NAME(), iter.name().c_str());
        datasets[iter.name()] = ds;
      }
    }
    catch (std::exception& e)
    {
      vtkLogF(ERROR, "failed to process '../topologies/%s'.", iter.name().c_str());
      vtkLogF(ERROR, "ERROR: \n%s\n", e.what());
      return 0;
    }
  }

  // process "fields"
  if (!node.has_path("fields"))
  {
    return 1;
  }

  auto& fields = node["fields"];
  iter = fields.children();
  while (iter.has_next())
  {
    auto& fieldNode = iter.next();
    const auto fieldname = iter.name();
    try
    {
      auto dataset = datasets.at(fieldNode["topology"].as_string());
      const auto vtk_association = internals::GetAssociation(fieldNode["association"].as_string());
      auto dsa = dataset->GetAttributes(vtk_association);
      auto array = vtkConduitArrayUtilities::MCArrayToVTKArray(&fieldNode["values"], fieldname);
      if (array->GetNumberOfTuples() != dataset->GetNumberOfElements(vtk_association))
      {
        throw std::runtime_error("mismatched tuple count!");
      }
      dsa->AddArray(array);
    }
    catch (std::exception& e)
    {
      vtkLogF(ERROR, "failed to process '../fields/%s'.", fieldname.c_str());
      vtkLogF(ERROR, "ERROR: \n%s\n", e.what());
      return 0;
    }
  }

  if (internals.GlobalFieldsNode)
  {
    auto fd = output->GetFieldData();
    auto& globalFields = (*internals.GlobalFieldsNode);
    // this can be made very generic. For now, I am only processing known
    // fields.
    if (globalFields.has_path("time"))
    {
      // for compatibility with older Catalyst scripts.
      vtkNew<vtkDoubleArray> timeValue;
      timeValue->SetName("TimeValue");
      timeValue->SetNumberOfTuples(1);
      timeValue->SetTypedComponent(0, 0, globalFields["time"].to_float64());
      fd->AddArray(timeValue);

      // "time" is a better name than "TimeValue"
      vtkNew<vtkDoubleArray> time;
      time->SetName("time");
      time->SetNumberOfTuples(1);
      time->SetTypedComponent(0, 0, globalFields["time"].to_float64());
      fd->AddArray(time);

      // let's also set DATA_TIME_STEP.
      output->GetInformation()->Set(
        vtkDataObject::DATA_TIME_STEP(), globalFields["time"].to_float64());
    }
    if (globalFields.has_path("cycle"))
    {
      vtkNew<vtkIntArray> cycle;
      cycle->SetName("cycle");
      cycle->SetNumberOfTuples(1);
      cycle->SetTypedComponent(0, 0, globalFields["cycle"].to_int64());
      fd->AddArray(cycle);
    }
    if (globalFields.has_path("timestep"))
    {
      vtkNew<vtkIntArray> timestep;
      timestep->SetName("timestep");
      timestep->SetNumberOfTuples(1);
      timestep->SetTypedComponent(0, 0, globalFields["timestep"].to_int64());
      fd->AddArray(timestep);
    }
    if (globalFields.has_path("channel"))
    {
      vtkNew<vtkStringArray> channel;
      channel->SetName("__CatalystChannel__");
      channel->InsertNextValue(globalFields["channel"].as_string().c_str());
      fd->AddArray(channel);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkConduitSource::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestInformation(request, inputVector, outputVector))
  {
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  auto& internals = (*this->Internals);
  if (internals.GlobalFieldsNode == nullptr)
  {
    return 1;
  }

  auto& node = (*internals.GlobalFieldsNode);
  if (node.has_path("time"))
  {
    double time = node["time"].to_float64();
    double timesteps[2] = { time, time };
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &time, 1);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timesteps, 2);
  }
  else
  {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkConduitSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
