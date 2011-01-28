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
#include "vtkSMSessionCoreInterpreterHelper.h"

#include "vtkObjectFactory.h"
#include "vtkPMProxy.h"
#include "vtkSMSessionCore.h"

vtkStandardNewMacro(vtkSMSessionCoreInterpreterHelper);
//----------------------------------------------------------------------------
vtkSMSessionCoreInterpreterHelper::vtkSMSessionCoreInterpreterHelper()
{
}

//----------------------------------------------------------------------------
vtkSMSessionCoreInterpreterHelper::~vtkSMSessionCoreInterpreterHelper()
{
}

//----------------------------------------------------------------------------
void vtkSMSessionCoreInterpreterHelper::SetCore(vtkSMSessionCore* core)
{
  this->Core = core;
}

//----------------------------------------------------------------------------
vtkPMObject* vtkSMSessionCoreInterpreterHelper::GetPMObject(vtkTypeUInt32 gid)
{
  return this->Core->GetPMObject(gid);
}

//----------------------------------------------------------------------------
vtkObjectBase* vtkSMSessionCoreInterpreterHelper::GetVTKObject(vtkTypeUInt32 gid)
{
  return vtkPMProxy::SafeDownCast(this->Core->GetPMObject(gid))->GetVTKObject();
}
//----------------------------------------------------------------------------
void vtkSMSessionCoreInterpreterHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
