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
#include "vtkReservedRemoteObjectIds.h"

vtkStandardNewMacro(vtkSMAnimationSceneProxy);
//----------------------------------------------------------------------------
vtkSMAnimationSceneProxy::vtkSMAnimationSceneProxy()
{
  this->SetGlobalID(vtkSMAnimationSceneProxy::GetReservedGlobalID());
}

//----------------------------------------------------------------------------
vtkSMAnimationSceneProxy::~vtkSMAnimationSceneProxy()
{
}

//----------------------------------------------------------------------------
vtkTypeUInt32 vtkSMAnimationSceneProxy::GetReservedGlobalID()
{
  return vtkReservedRemoteObjectIds::RESERVED_ANIMATION_SCENE_ID;
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
//----------------------------------------------------------------------------
vtkTypeUInt32 vtkSMAnimationSceneProxy::GetGlobalID()
{
  if(!this->HasGlobalID())
    {
    this->SetGlobalID(vtkSMAnimationSceneProxy::GetReservedGlobalID());
    }
  return this->Superclass::GetGlobalID();
}
