// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

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
