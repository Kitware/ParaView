/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMSpriteTextureProxy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkSMSpriteTextureProxy
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>

#include "vtkSMSpriteTextureProxy.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMSourceProxy.h"
#include "vtkImageData.h"

vtkStandardNewMacro(vtkSMSpriteTextureProxy);
//----------------------------------------------------------------------------
vtkSMSpriteTextureProxy::vtkSMSpriteTextureProxy()
{
  this->SetServers(vtkProcessModule::CLIENT
    | vtkProcessModule::RENDER_SERVER);
}

//----------------------------------------------------------------------------
vtkSMSpriteTextureProxy::~vtkSMSpriteTextureProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMSpriteTextureProxy::CreateVTKObjects()
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
vtkImageData* vtkSMSpriteTextureProxy::GetLoadedImage()
{
  vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("Source"))->UpdatePipeline();
  vtkAlgorithm* source = vtkAlgorithm::SafeDownCast(
    this->GetSubProxy("Source")->GetClientSideObject());
  return source? vtkImageData::SafeDownCast(source->GetOutputDataObject(0)): 0;
}

//----------------------------------------------------------------------------
void vtkSMSpriteTextureProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


