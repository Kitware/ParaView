/*=========================================================================

  Program:   ParaView
  Module:    vtkSMFetchDataProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMFetchDataProxy.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkSMPropertyHelper.h"

vtkStandardNewMacro(vtkSMFetchDataProxy);
//----------------------------------------------------------------------------
vtkSMFetchDataProxy::vtkSMFetchDataProxy()
{
}

//----------------------------------------------------------------------------
vtkSMFetchDataProxy::~vtkSMFetchDataProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMFetchDataProxy::UpdatePipeline()
{
  this->PassMetaData(0.0, false);
  this->Superclass::UpdatePipeline();
}

//----------------------------------------------------------------------------
void vtkSMFetchDataProxy::UpdatePipeline(double time)
{
  this->PassMetaData(time, true);
  this->Superclass::UpdatePipeline(time);
}

//----------------------------------------------------------------------------
void vtkSMFetchDataProxy::PassMetaData(double time, bool use_time)
{
  vtkSMPropertyHelper helper(this, "Input");

  if (helper.GetNumberOfElements() == 0)
    {
    return;
    }
  vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(helper.GetAsProxy(0));
  if (!input)
    {
    return;
    }
  unsigned int port = helper.GetOutputPort(0);

  if (use_time)
    {
    input->UpdatePipeline(time);
    }
  else
    {
    input->UpdatePipeline();
    }

  vtkPVDataInformation* inputInfo = input->GetDataInformation(port);
  int dataType = inputInfo->GetDataSetType();
  int cDataType = inputInfo->GetCompositeDataSetType();
  if (cDataType > 0)
    {
    dataType = cDataType;
    }

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->GetID() << "SetOutputDataType" << dataType
         << vtkClientServerStream::End;
  if (dataType == VTK_STRUCTURED_POINTS ||
      dataType == VTK_STRUCTURED_GRID   ||
      dataType == VTK_RECTILINEAR_GRID  ||
      dataType == VTK_IMAGE_DATA)
    {
    const int* extent = inputInfo->GetExtent();
    stream << vtkClientServerStream::Invoke
           << this->GetID()
           << "SetWholeExtent"
           << vtkClientServerStream::InsertArray(extent, 6)
           << vtkClientServerStream::End;
    }

  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, this->GetServers(), stream);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkSMFetchDataProxy::GetData()
{
  return vtkAlgorithm::SafeDownCast(
    this->GetClientSideObject())->GetOutputDataObject(0);
}

//----------------------------------------------------------------------------
void vtkSMFetchDataProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
