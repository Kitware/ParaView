/*=========================================================================

  Program:   ParaView
  Module:    vtkSMExponentialKeyFrameProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMExponentialKeyFrameProxy.h"
#include "vtkObjectFactory.h"
#include "vtkSMDomain.h"
#include "vtkSMProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMAnimationCueProxy.h"

#include <math.h>

vtkCxxRevisionMacro(vtkSMExponentialKeyFrameProxy, "1.1");
vtkStandardNewMacro(vtkSMExponentialKeyFrameProxy);

//----------------------------------------------------------------------------
vtkSMExponentialKeyFrameProxy::vtkSMExponentialKeyFrameProxy()
{
  this->Base = 2.0;
  this->StartPower = -1.0;
  this->EndPower = -5.0;
}

//----------------------------------------------------------------------------
vtkSMExponentialKeyFrameProxy::~vtkSMExponentialKeyFrameProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMExponentialKeyFrameProxy::SaveInBatchScript(ofstream* file)
{
  this->Superclass::SaveInBatchScript(file);
  *file << "  [$pvTemp" << this->SelfID << " GetProperty Base]"
    << " SetElements1 " << this->Base << endl;
  *file << "  [$pvTemp" << this->SelfID << " GetProperty StartPower]"
    << " SetElements1 " << this->StartPower << endl;
  *file << "  [$pvTemp" << this->SelfID << " GetProperty EndPower]"
    << " SetElements1 " << this->EndPower << endl;
  *file << "  $pvTemp" << this->SelfID << " UpdateVTKObjects";
  *file << endl;
}

//----------------------------------------------------------------------------
// remeber that currenttime is 0 at the KeyTime of this key frame
// and 1 and the KeyTime of the next key frame. Hence,
// currenttime belongs to the interval [0,1).
void vtkSMExponentialKeyFrameProxy::UpdateValue(double currenttime,
    vtkSMAnimationCueProxy* cueProxy, vtkSMKeyFrameProxy* next)
{
  if (!next)
    {
    return;
    }

  if (this->Base == 1)
    {
    vtkErrorMacro("Exponential with base 1");
    }

  double tcur = pow(this->Base, this->StartPower + currenttime * 
    (this->EndPower- this->StartPower)); 
  double tmin = pow(this->Base, this->StartPower);
  double tmax = pow(this->Base, this->EndPower);

  double t = (this->Base!= 1) ? (tcur - tmin) / (tmax - tmin): 0;

  double vmax = next->GetKeyValue();
  double vmin = this->GetKeyValue();
  double value = vmin + t * (vmax - vmin);

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
  //cout << " Value : " << value << endl;
}

//----------------------------------------------------------------------------
void vtkSMExponentialKeyFrameProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Base: " << this->Base << endl;
  os << indent << "StartPower: " << this->StartPower << endl;
  os << indent << "EndPower: " << this->EndPower << endl;
}
