// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSession.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"

//----------------------------------------------------------------------------
vtkSession::vtkSession() = default;

//----------------------------------------------------------------------------
vtkSession::~vtkSession() = default;

//----------------------------------------------------------------------------
void vtkSession::Activate()
{
  if (vtkProcessModule* pm = vtkProcessModule::GetProcessModule())
  {
    pm->PushActiveSession(this);
  }
}

//----------------------------------------------------------------------------
void vtkSession::DeActivate()
{
  if (vtkProcessModule* pm = vtkProcessModule::GetProcessModule())
  {
    pm->PopActiveSession(this);
  }
}

//----------------------------------------------------------------------------
void vtkSession::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
