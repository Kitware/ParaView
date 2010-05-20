/*=========================================================================

  Program:   ParaView
  Module:    vtkSMImageTextureProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMImageTextureProxy.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMSourceProxy.h"
#include "vtkImageData.h"

vtkStandardNewMacro(vtkSMImageTextureProxy);
//----------------------------------------------------------------------------
vtkSMImageTextureProxy::vtkSMImageTextureProxy()
{
  this->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
}

//----------------------------------------------------------------------------
vtkSMImageTextureProxy::~vtkSMImageTextureProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMImageTextureProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects();

  vtkSMProxy* reader = this->GetSubProxy("Source");
  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << reader->GetID()
          << "GetOutputPort"
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << this->GetID()
          << "SetInputConnection"
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(this->ConnectionID,
    this->Servers, stream);
}

//----------------------------------------------------------------------------
vtkImageData* vtkSMImageTextureProxy::GetLoadedImage()
{
  vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("Source"))->UpdatePipeline();
  vtkAlgorithm* source = vtkAlgorithm::SafeDownCast(
    this->GetSubProxy("Source")->GetClientSideObject());
  return source? vtkImageData::SafeDownCast(source->GetOutputDataObject(0)): 0;
}

//----------------------------------------------------------------------------
void vtkSMImageTextureProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


