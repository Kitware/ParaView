/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTimeAnimationCueProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMTimeAnimationCueProxy.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkSMDomain.h"
#include "vtkSMProperty.h"

vtkStandardNewMacro(vtkSMTimeAnimationCueProxy);
//----------------------------------------------------------------------------
vtkSMTimeAnimationCueProxy::vtkSMTimeAnimationCueProxy()
{
  this->UseAnimationTime = 1;
}

//----------------------------------------------------------------------------
vtkSMTimeAnimationCueProxy::~vtkSMTimeAnimationCueProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMTimeAnimationCueProxy::TickInternal(void* info)
{
  if (this->UseAnimationTime)
    {
    // Directly use the animation clock time.
    vtkSMDomain *domain = this->GetAnimatedDomain();
    vtkSMProperty *property = this->GetAnimatedProperty();
    vtkSMProxy *proxy = this->GetAnimatedProxy();

    if (domain && property)
      {
      domain->SetAnimationValue(property, 
        this->GetAnimatedElement(), this->GetClockTime());
      }

    if (proxy)
      {
      proxy->UpdateVTKObjects();
      }
    this->InvokeEvent(vtkCommand::AnimationCueTickEvent, info);
    }
  else
    {
    this->Superclass::TickInternal(info);
    }
}

//----------------------------------------------------------------------------
void vtkSMTimeAnimationCueProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseAnimationTime: " << this->UseAnimationTime << endl;
}


