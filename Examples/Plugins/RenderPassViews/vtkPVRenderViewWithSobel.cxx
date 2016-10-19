/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVRenderViewWithSobel.h"

#include "vtkObjectFactory.h"
#include "vtkPVSynchronizedRenderer.h"
#include "vtkSobelGradientMagnitudePass.h"

vtkStandardNewMacro(vtkPVRenderViewWithSobel);
//----------------------------------------------------------------------------
vtkPVRenderViewWithSobel::vtkPVRenderViewWithSobel()
{
}

//----------------------------------------------------------------------------
vtkPVRenderViewWithSobel::~vtkPVRenderViewWithSobel()
{
}

//----------------------------------------------------------------------------
void vtkPVRenderViewWithSobel::Initialize(unsigned int id)
{
  this->Superclass::Initialize(id);

  vtkSobelGradientMagnitudePass* sobel = vtkSobelGradientMagnitudePass::New();
  this->SynchronizedRenderers->SetImageProcessingPass(sobel);
  sobel->Delete();
}

//----------------------------------------------------------------------------
void vtkPVRenderViewWithSobel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
