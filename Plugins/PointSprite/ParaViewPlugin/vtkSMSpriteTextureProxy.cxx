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

#include "vtkSMSession.h"
#include "vtkSIObject.h"
#include "vtkSIProxy.h"

vtkStandardNewMacro(vtkSMSpriteTextureProxy);
//----------------------------------------------------------------------------
vtkSMSpriteTextureProxy::vtkSMSpriteTextureProxy()
{
  this->SetLocation( vtkProcessModule::CLIENT |
                     vtkProcessModule::RENDER_SERVER);
}

//----------------------------------------------------------------------------
vtkSMSpriteTextureProxy::~vtkSMSpriteTextureProxy()
{
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


