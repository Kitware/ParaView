/*=========================================================================

  Program:   ParaView
  Module:    vtkSMImageDataParallelStrategy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMImageDataParallelStrategy.h"

#include "vtkObjectFactory.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMImageDataParallelStrategy);
//----------------------------------------------------------------------------
vtkSMImageDataParallelStrategy::vtkSMImageDataParallelStrategy()
{
}

//----------------------------------------------------------------------------
vtkSMImageDataParallelStrategy::~vtkSMImageDataParallelStrategy()
{
}

//----------------------------------------------------------------------------
void vtkSMImageDataParallelStrategy::CreatePipeline(
  vtkSMSourceProxy* input, int outputport)
{
  this->Superclass::CreatePipeline(input, outputport);

  // Collect filter must be told the output data type since the data may not be
  // available on all processess.
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->Collect->GetProperty("OutputDataType"));
  ivp->SetElement(0, VTK_IMAGE_DATA);
  this->Collect->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMImageDataParallelStrategy::CreateLODPipeline(
  vtkSMSourceProxy* input, int outputport)
{
  this->Superclass::CreateLODPipeline(input, outputport);

  // Collect filter must be told the output data type since the data may not be
  // available on all processess.
  // The LOD pipeline outputs polydata (and not UnstructuredGrid).
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CollectLOD->GetProperty("OutputDataType"));
  ivp->SetElement(0, VTK_IMAGE_DATA);
  this->CollectLOD->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMImageDataParallelStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


