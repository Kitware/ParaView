/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRampKeyFrameProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMRampKeyFrameProxy.h"
#include "vtkObjectFactory.h"
#include "vtkSMDomain.h"
#include "vtkSMProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMAnimationCueProxy.h"
#include "vtkSMSourceProxy.h"
vtkCxxRevisionMacro(vtkSMRampKeyFrameProxy, "1.1");
vtkStandardNewMacro(vtkSMRampKeyFrameProxy);

//----------------------------------------------------------------------------
vtkSMRampKeyFrameProxy::vtkSMRampKeyFrameProxy()
{
}

//----------------------------------------------------------------------------
vtkSMRampKeyFrameProxy::~vtkSMRampKeyFrameProxy()
{
}

//----------------------------------------------------------------------------
// remeber that currenttime is 0 at the KeyTime of this key frame
// and 1 and the KeyTime of the next key frame. Hence,
// currenttime belongs to the interval [0,1).
void vtkSMRampKeyFrameProxy::UpdateValue(double currenttime,
    vtkSMAnimationCueProxy* cueProxy, vtkSMKeyFrameProxy* next)
{
  if (!next)
    {
    return;
    }
  double vmax = next->GetKeyValue();
  double vmin = this->GetKeyValue();
  double value = vmin + currenttime * (vmax - vmin);

  vtkSMDomain *domain = cueProxy->GetAnimatedDomain();
  vtkSMProperty *property = cueProxy->GetAnimatedProperty();
  vtkSMProxy *proxy = cueProxy->GetAnimatedProxy();

  if (domain && property)
    {
    domain->SetAnimationValue(property, cueProxy->GetAnimatedElement(),
      value);
    }
  if (proxy)
    {
    proxy->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
void vtkSMRampKeyFrameProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
