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

#include "vtkSMInteractiveSelectionPipeline.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"

vtkStandardNewMacro(vtkSMInteractiveSelectionPipeline);

//----------------------------------------------------------------------------
vtkSMInteractiveSelectionPipeline::vtkSMInteractiveSelectionPipeline() = default;

//----------------------------------------------------------------------------
vtkSMInteractiveSelectionPipeline::~vtkSMInteractiveSelectionPipeline() = default;

//----------------------------------------------------------------------------
vtkSMInteractiveSelectionPipeline* vtkSMInteractiveSelectionPipeline::GetInstance()
{
  static vtkSmartPointer<vtkSMInteractiveSelectionPipeline> Instance;
  if (Instance.GetPointer() == nullptr)
  {
    vtkSMInteractiveSelectionPipeline* pipeline = vtkSMInteractiveSelectionPipeline::New();
    Instance = pipeline;
    pipeline->FastDelete();
  }

  return Instance;
}

//----------------------------------------------------------------------------
void vtkSMInteractiveSelectionPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  (void)os;
  (void)indent;
}
