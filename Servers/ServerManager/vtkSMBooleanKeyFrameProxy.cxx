/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBooleanKeyFrameProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMBooleanKeyFrameProxy.h"
#include "vtkObjectFactory.h"
#include "vtkSMDomain.h"
#include "vtkSMProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMAnimationCueProxy.h"
vtkCxxRevisionMacro(vtkSMBooleanKeyFrameProxy, "1.1");
vtkStandardNewMacro(vtkSMBooleanKeyFrameProxy);

//----------------------------------------------------------------------------
vtkSMBooleanKeyFrameProxy::vtkSMBooleanKeyFrameProxy()
{
}

//----------------------------------------------------------------------------
vtkSMBooleanKeyFrameProxy::~vtkSMBooleanKeyFrameProxy()
{
}

//----------------------------------------------------------------------------
// remeber that currenttime is 0 at the KeyTime of this key frame
// and 1 and the KeyTime of the next key frame. Hence,
// currenttime belongs to the interval [0,1).
void vtkSMBooleanKeyFrameProxy::UpdateValue(double currenttime,
    vtkSMAnimationCueProxy* cueProxy, vtkSMKeyFrameProxy* next)
{
  vtkSMDomain *domain = cueProxy->GetAnimatedDomain();
  vtkSMProperty *property = cueProxy->GetAnimatedProperty();
  vtkSMProxy *proxy = cueProxy->GetAnimatedProxy();

  if (domain && property)
    {
    domain->SetAnimationValue(property, cueProxy->GetAnimatedElement(),
      this->GetKeyValue());
    }
  if (proxy)
    {
    proxy->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
void vtkSMBooleanKeyFrameProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
