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
#include "vtkSMAnimationSceneProxy.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSMAnimationSceneProxy);
//----------------------------------------------------------------------------
vtkSMAnimationSceneProxy::vtkSMAnimationSceneProxy()
{
}

//----------------------------------------------------------------------------
vtkSMAnimationSceneProxy::~vtkSMAnimationSceneProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects();
  if (vtkObject* object = vtkObject::SafeDownCast(this->GetClientSideObject()))
    {
    object->AddObserver(vtkCommand::ModifiedEvent,
      this, &vtkSMAnimationSceneProxy::UpdatePropertyInformation);
    }
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
