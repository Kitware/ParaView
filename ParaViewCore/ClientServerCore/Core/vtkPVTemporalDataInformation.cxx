/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTemporalDataInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTemporalDataInformation.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkClientServerStream.h"
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vector>

vtkStandardNewMacro(vtkPVTemporalDataInformation);
//----------------------------------------------------------------------------
vtkPVTemporalDataInformation::vtkPVTemporalDataInformation()
{
  this->NumberOfTimeSteps = 0;
  this->TimeRange[0] = VTK_DOUBLE_MAX;
  this->TimeRange[1] = -VTK_DOUBLE_MAX;
  this->PortNumber = 0;

  this->PointDataInformation = vtkPVDataSetAttributesInformation::New();
  this->CellDataInformation = vtkPVDataSetAttributesInformation::New();
  this->FieldDataInformation = vtkPVDataSetAttributesInformation::New();
  this->VertexDataInformation = vtkPVDataSetAttributesInformation::New();
  this->EdgeDataInformation = vtkPVDataSetAttributesInformation::New();
  this->RowDataInformation = vtkPVDataSetAttributesInformation::New();

  // Update field association information on the all the
  // vtkPVDataSetAttributesInformation instances.
  for (int cc = 0; cc < vtkDataObject::NUMBER_OF_ASSOCIATIONS; cc++)
  {
    if (vtkPVDataSetAttributesInformation* dsa = this->GetAttributeInformation(cc))
    {
      dsa->SetFieldAssociation(cc);
    }
  }
}

//----------------------------------------------------------------------------
vtkPVTemporalDataInformation::~vtkPVTemporalDataInformation()
{
  this->PointDataInformation->Delete();
  this->PointDataInformation = NULL;
  this->CellDataInformation->Delete();
  this->CellDataInformation = NULL;
  this->FieldDataInformation->Delete();
  this->FieldDataInformation = NULL;
  this->VertexDataInformation->Delete();
  this->VertexDataInformation = NULL;
  this->EdgeDataInformation->Delete();
  this->EdgeDataInformation = NULL;
  this->RowDataInformation->Delete();
  this->RowDataInformation = NULL;
}

//----------------------------------------------------------------------------
vtkPVDataSetAttributesInformation* vtkPVTemporalDataInformation::GetAttributeInformation(int attr)
{
  switch (attr)
  {
    case vtkDataObject::POINT:
      return this->PointDataInformation;

    case vtkDataObject::CELL:
      return this->CellDataInformation;

    case vtkDataObject::FIELD:
      return this->FieldDataInformation;

    case vtkDataObject::VERTEX:
      return this->VertexDataInformation;

    case vtkDataObject::EDGE:
      return this->EdgeDataInformation;

    case vtkDataObject::ROW:
      return this->RowDataInformation;
  }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkPVTemporalDataInformation::Initialize()
{
  this->NumberOfTimeSteps = 0;
  this->TimeRange[0] = VTK_DOUBLE_MAX;
  this->TimeRange[1] = -VTK_DOUBLE_MAX;
  this->PointDataInformation->Initialize();
  this->CellDataInformation->Initialize();
  this->FieldDataInformation->Initialize();
  this->VertexDataInformation->Initialize();
  this->EdgeDataInformation->Initialize();
  this->RowDataInformation->Initialize();
}

//----------------------------------------------------------------------------
void vtkPVTemporalDataInformation::CopyParametersToStream(vtkMultiProcessStream& str)
{
  str << 829993 << this->PortNumber;
}

//----------------------------------------------------------------------------
void vtkPVTemporalDataInformation::CopyParametersFromStream(vtkMultiProcessStream& str)
{
  int magic_number;
  str >> magic_number >> this->PortNumber;
  if (magic_number != 829993)
  {
    vtkErrorMacro("Magic number mismatch.");
  }
}

//----------------------------------------------------------------------------
void vtkPVTemporalDataInformation::CopyFromObject(vtkObject* object)
{
  vtkAlgorithm* algo = vtkAlgorithm::SafeDownCast(object);
  vtkAlgorithmOutput* port = vtkAlgorithmOutput::SafeDownCast(object);
  if (algo)
  {
    port = algo->GetOutputPort(this->PortNumber);
  }

  if (!port)
  {
    vtkErrorMacro("vtkPVTemporalDataInformation needs a vtkAlgorithm or "
                  " a vtkAlgorithmOutput.");
    return;
  }

  port->GetProducer()->Update();
  vtkDataObject* dobj = port->GetProducer()->GetOutputDataObject(port->GetIndex());

  // Collect current information.
  vtkSmartPointer<vtkPVDataInformation> dinfo = vtkSmartPointer<vtkPVDataInformation>::New();
  dinfo->CopyFromObject(dobj);
  this->AddInformation(dinfo);

  if (!dinfo->GetHasTime() || dinfo->GetTimeSpan()[0] == dinfo->GetTimeSpan()[1])
  {
    // nothing temporal about this data! Nothing to do.
    return;
  }

  // We are not assured that this data has time. We currently only handle
  // timesteps properly, for contiguous time-range, we simply use the first and
  // last time value as the 2 timesteps.

  vtkInformation* pipelineInfo = port->GetProducer()->GetOutputInformation(port->GetIndex());
  std::vector<double> timesteps;
  if (pipelineInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    double* ptimesteps = pipelineInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    int length = pipelineInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    timesteps.resize(length);
    for (int cc = 0; cc < length; cc++)
    {
      timesteps[cc] = ptimesteps[cc];
    }
    this->NumberOfTimeSteps = length;
  }
  else if (pipelineInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
  {
    double* ptimesteps = pipelineInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    timesteps.push_back(ptimesteps[0]);
    timesteps.push_back(ptimesteps[1]);
    this->NumberOfTimeSteps = 0;
  }

  std::vector<double>::iterator iter;
  vtkStreamingDemandDrivenPipeline* sddp =
    vtkStreamingDemandDrivenPipeline::SafeDownCast(port->GetProducer()->GetExecutive());
  if (!sddp)
  {
    vtkErrorMacro("This class expects vtkStreamingDemandDrivenPipeline.");
    return;
  }

  double current_time = dinfo->GetTime();
  for (iter = timesteps.begin(); iter != timesteps.end(); ++iter)
  {
    if (*iter == current_time)
    {
      // skip the timestep already seen.
      continue;
    }
    pipelineInfo->Set(sddp->UPDATE_TIME_STEP(), *iter);
    sddp->Update(port->GetIndex());

    dobj = port->GetProducer()->GetOutputDataObject(port->GetIndex());
    dinfo->Initialize();
    dinfo->CopyFromObject(dobj);
    this->AddInformation(dinfo);
  }
}

//----------------------------------------------------------------------------
void vtkPVTemporalDataInformation::AddInformation(vtkPVInformation* info)
{
  vtkPVDataInformation* dinfo = vtkPVDataInformation::SafeDownCast(info);
  vtkPVTemporalDataInformation* tinfo = vtkPVTemporalDataInformation::SafeDownCast(info);

  if (dinfo)
  {
    this->PointDataInformation->AddInformation(dinfo->GetPointDataInformation());
    this->CellDataInformation->AddInformation(dinfo->GetCellDataInformation());
    this->VertexDataInformation->AddInformation(dinfo->GetVertexDataInformation());
    this->EdgeDataInformation->AddInformation(dinfo->GetEdgeDataInformation());
    this->RowDataInformation->AddInformation(dinfo->GetRowDataInformation());
    this->FieldDataInformation->AddInformation(dinfo->GetFieldDataInformation());
  }
  else if (tinfo)
  {
    this->PointDataInformation->AddInformation(tinfo->GetPointDataInformation());
    this->CellDataInformation->AddInformation(tinfo->GetCellDataInformation());
    this->VertexDataInformation->AddInformation(tinfo->GetVertexDataInformation());
    this->EdgeDataInformation->AddInformation(tinfo->GetEdgeDataInformation());
    this->RowDataInformation->AddInformation(tinfo->GetRowDataInformation());
    this->FieldDataInformation->AddInformation(tinfo->GetFieldDataInformation());
    this->NumberOfTimeSteps = tinfo->NumberOfTimeSteps > this->NumberOfTimeSteps
      ? tinfo->NumberOfTimeSteps
      : this->NumberOfTimeSteps;
    this->TimeRange[0] =
      tinfo->TimeRange[0] < this->TimeRange[0] ? tinfo->TimeRange[0] : this->TimeRange[0];
    this->TimeRange[1] =
      tinfo->TimeRange[1] > this->TimeRange[1] ? tinfo->TimeRange[1] : this->TimeRange[1];
  }
}

//----------------------------------------------------------------------------
void vtkPVTemporalDataInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply << this->NumberOfTimeSteps << this->TimeRange[0]
       << this->TimeRange[1];

  size_t length;
  const unsigned char* data;
  vtkClientServerStream dcss;
  this->PointDataInformation->CopyToStream(&dcss);
  dcss.GetData(&data, &length);
  *css << vtkClientServerStream::InsertArray(data, static_cast<int>(length));

  dcss.Reset();
  this->CellDataInformation->CopyToStream(&dcss);
  dcss.GetData(&data, &length);
  *css << vtkClientServerStream::InsertArray(data, static_cast<int>(length));

  dcss.Reset();
  this->VertexDataInformation->CopyToStream(&dcss);
  dcss.GetData(&data, &length);
  *css << vtkClientServerStream::InsertArray(data, static_cast<int>(length));

  dcss.Reset();
  this->EdgeDataInformation->CopyToStream(&dcss);
  dcss.GetData(&data, &length);
  *css << vtkClientServerStream::InsertArray(data, static_cast<int>(length));

  dcss.Reset();
  this->RowDataInformation->CopyToStream(&dcss);
  dcss.GetData(&data, &length);
  *css << vtkClientServerStream::InsertArray(data, static_cast<int>(length));

  dcss.Reset();
  this->FieldDataInformation->CopyToStream(&dcss);
  dcss.GetData(&data, &length);
  *css << vtkClientServerStream::InsertArray(data, static_cast<int>(length));

  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVTemporalDataInformation::CopyFromStream(const vtkClientServerStream* css)
{
  int index = 0;
  if (!css->GetArgument(0, index++, &this->NumberOfTimeSteps))
  {
    vtkErrorMacro("Error parsing NumberOfTimeSteps.");
    return;
  }
  if (!css->GetArgument(0, index++, &this->TimeRange[0]))
  {
    vtkErrorMacro("Error parsing TimeRange[0].");
    return;
  }
  if (!css->GetArgument(0, index++, &this->TimeRange[1]))
  {
    vtkErrorMacro("Error parsing TimeRange[1].");
    return;
  }

  vtkTypeUInt32 length;
  std::vector<unsigned char> data;
  vtkClientServerStream dcss;

  // Point array information.
  if (!css->GetArgumentLength(0, index, &length))
  {
    vtkErrorMacro("Error parsing length of data information.");
    return;
  }
  data.resize(length);
  if (!css->GetArgument(0, index++, &*data.begin(), length))
  {
    vtkErrorMacro("Error parsing data information.");
    return;
  }
  dcss.SetData(&*data.begin(), length);
  this->PointDataInformation->CopyFromStream(&dcss);

  // Cell array information.
  if (!css->GetArgumentLength(0, index, &length))
  {
    vtkErrorMacro("Error parsing length of data information.");
    return;
  }
  data.resize(length);
  if (!css->GetArgument(0, index++, &*data.begin(), length))
  {
    vtkErrorMacro("Error parsing data information.");
    return;
  }
  dcss.SetData(&*data.begin(), length);
  this->CellDataInformation->CopyFromStream(&dcss);

  // Vertex array information.
  if (!css->GetArgumentLength(0, index, &length))
  {
    vtkErrorMacro("Error parsing length of data information.");
    return;
  }
  data.resize(length);
  if (!css->GetArgument(0, index++, &*data.begin(), length))
  {
    vtkErrorMacro("Error parsing data information.");
    return;
  }
  dcss.SetData(&*data.begin(), length);
  this->VertexDataInformation->CopyFromStream(&dcss);

  // Edge array information.
  if (!css->GetArgumentLength(0, index, &length))
  {
    vtkErrorMacro("Error parsing length of data information.");
    return;
  }
  data.resize(length);
  if (!css->GetArgument(0, index++, &*data.begin(), length))
  {
    vtkErrorMacro("Error parsing data information.");
    return;
  }
  dcss.SetData(&*data.begin(), length);
  this->EdgeDataInformation->CopyFromStream(&dcss);

  // Row array information.
  if (!css->GetArgumentLength(0, index, &length))
  {
    vtkErrorMacro("Error parsing length of data information.");
    return;
  }
  data.resize(length);
  if (!css->GetArgument(0, index++, &*data.begin(), length))
  {
    vtkErrorMacro("Error parsing data information.");
    return;
  }
  dcss.SetData(&*data.begin(), length);
  this->RowDataInformation->CopyFromStream(&dcss);

  // Field array information.
  if (!css->GetArgumentLength(0, index, &length))
  {
    vtkErrorMacro("Error parsing length of data information.");
    return;
  }
  data.resize(length);
  if (!css->GetArgument(0, index++, &*data.begin(), length))
  {
    vtkErrorMacro("Error parsing data information.");
    return;
  }
  dcss.SetData(&*data.begin(), length);
  this->FieldDataInformation->CopyFromStream(&dcss);

  return;
}

//----------------------------------------------------------------------------
vtkPVArrayInformation* vtkPVTemporalDataInformation::GetArrayInformation(
  const char* arrayname, int attribute_type)
{
  vtkPVDataSetAttributesInformation* attrInfo = this->GetAttributeInformation(attribute_type);
  return attrInfo ? attrInfo->GetArrayInformation(arrayname) : NULL;
}

//----------------------------------------------------------------------------
void vtkPVTemporalDataInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfTimeSteps: " << this->NumberOfTimeSteps << endl;
  os << indent << "TimeRange: " << this->TimeRange[0] << ", " << this->TimeRange[1] << endl;

  vtkIndent i2 = indent.GetNextIndent();
  os << indent << "PointDataInformation " << endl;
  this->PointDataInformation->PrintSelf(os, i2);
  os << indent << "CellDataInformation " << endl;
  this->CellDataInformation->PrintSelf(os, i2);
  os << indent << "VertexDataInformation" << endl;
  this->VertexDataInformation->PrintSelf(os, i2);
  os << indent << "EdgeDataInformation" << endl;
  this->EdgeDataInformation->PrintSelf(os, i2);
  os << indent << "RowDataInformation" << endl;
  this->RowDataInformation->PrintSelf(os, i2);
  os << indent << "FieldDataInformation " << endl;
  this->FieldDataInformation->PrintSelf(os, i2);
}
