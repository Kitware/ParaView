/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUnstructuredGridParallelStrategy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMUnstructuredGridParallelStrategy.h"

#include "vtkObjectFactory.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMUnstructuredGridParallelStrategy);
vtkCxxRevisionMacro(vtkSMUnstructuredGridParallelStrategy, "1.1.2.1");
//----------------------------------------------------------------------------
vtkSMUnstructuredGridParallelStrategy::vtkSMUnstructuredGridParallelStrategy()
{
}

//----------------------------------------------------------------------------
vtkSMUnstructuredGridParallelStrategy::~vtkSMUnstructuredGridParallelStrategy()
{
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridParallelStrategy::CreatePipeline(vtkSMSourceProxy* input)
{
  this->Superclass::CreatePipeline(input);

  // Collect filter must be told the output data type since the data may not be
  // available on all processess.
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->Collect->GetProperty("OutputDataType"));
  ivp->SetElement(0, VTK_UNSTRUCTURED_GRID);
  this->Collect->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridParallelStrategy::CreateLODPipeline(vtkSMSourceProxy* input)
{
  this->Superclass::CreateLODPipeline(input);

  // Collect filter must be told the output data type since the data may not be
  // available on all processess.
  // The LOD pipeline outputs polydata (and not UnstructuredGrid).
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CollectLOD->GetProperty("OutputDataType"));
  ivp->SetElement(0, VTK_POLY_DATA);
  this->CollectLOD->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridParallelStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


