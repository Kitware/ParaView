/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSinusoidKeyFrameProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSinusoidKeyFrameProxy.h"
#include "vtkObjectFactory.h"
#include "vtkSMDomain.h"
#include "vtkSMProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMAnimationCueProxy.h"

#include <math.h>

vtkCxxRevisionMacro(vtkSMSinusoidKeyFrameProxy, "1.2");
vtkStandardNewMacro(vtkSMSinusoidKeyFrameProxy);

//----------------------------------------------------------------------------
vtkSMSinusoidKeyFrameProxy::vtkSMSinusoidKeyFrameProxy()
{
  this->Phase = 0;
  this->Frequency = 1;
  this->Offset = 0;
}

//----------------------------------------------------------------------------
vtkSMSinusoidKeyFrameProxy::~vtkSMSinusoidKeyFrameProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMSinusoidKeyFrameProxy::SaveInBatchScript(ofstream* file)
{
  this->Superclass::SaveInBatchScript(file);
  *file << "  [$pvTemp" << this->SelfID << " GetProperty Phase]"
    << " SetElements1 " << this->Phase<< endl;
  *file << "  [$pvTemp" << this->SelfID << " GetProperty Frequency]"
    << " SetElements1 " << this->Frequency<< endl;
  *file << "  [$pvTemp" << this->SelfID << " GetProperty Offset]"
    << " SetElements1 " << this->Offset<< endl;
  *file << "  $pvTemp" << this->SelfID << " UpdateVTKObjects";
  *file << endl;
}

//----------------------------------------------------------------------------
// remeber that currenttime is 0 at the KeyTime of this key frame
// and 1 and the KeyTime of the next key frame. Hence,
// currenttime belongs to the interval [0,1).
void vtkSMSinusoidKeyFrameProxy::UpdateValue(double currenttime,
    vtkSMAnimationCueProxy* cueProxy, vtkSMKeyFrameProxy* next)
{
  if (!next)
    {
    return;
    }

  // ( start + (end-start)*sin( 2*pi* (freq*t + phase/360) )/2 )
  double t = sin ( 8.0 * atan(static_cast<double>(1.0)) *  
    (this->Frequency* currenttime + this->Phase/360.0)) /  2.0;

  double vmin = this->GetKeyValue();
  double vmax = vmin + this->Offset; 
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
}

//----------------------------------------------------------------------------
void vtkSMSinusoidKeyFrameProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Frequency: " << this->Frequency << endl;
  os << indent << "Phase: " << this->Phase << endl;
  os << indent << "Offset: " << this->Offset << endl;
}
