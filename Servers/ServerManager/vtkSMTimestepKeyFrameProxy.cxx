/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTimestepKeyFrameProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMTimestepKeyFrameProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMAnimationCueProxy.h"
#include "vtkSMDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMTimestepKeyFrameProxy);
vtkCxxRevisionMacro(vtkSMTimestepKeyFrameProxy, "1.3");
//-----------------------------------------------------------------------------
vtkSMTimestepKeyFrameProxy::vtkSMTimestepKeyFrameProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMTimestepKeyFrameProxy::~vtkSMTimestepKeyFrameProxy()
{
}

//-----------------------------------------------------------------------------
// remeber that currenttime is 0 at the KeyTime of this key frame
// and 1 and the KeyTime of the next key frame. Hence,
// currenttime belongs to the interval [0,1).
void vtkSMTimestepKeyFrameProxy::UpdateValue(double currenttime,
    vtkSMAnimationCueProxy* cueProxy, vtkSMKeyFrameProxy* next)
{
  if (!next)
    {
    return;
    }
  vtkSMDomain *domain = cueProxy->GetAnimatedDomain();
  vtkSMProperty *property = cueProxy->GetAnimatedProperty();
  vtkSMProxy *proxy = cueProxy->GetAnimatedProxy();
  int animated_element = cueProxy->GetAnimatedElement();

  if (!proxy || !domain || !property)
    {
    vtkErrorMacro("Cue does not have domain or property set!");
    return;
    }

  if (animated_element == -1)
    {
    vtkErrorMacro("This keyframe cannot animate all elements in a property."
      " Please specify an index to animate.");
    return;
    }

  vtkSMDoubleVectorProperty* timestepValues = 
    vtkSMDoubleVectorProperty::SafeDownCast(property->GetInformationProperty());
  if (!timestepValues)
    {
    vtkErrorMacro("vtkSMTimestepKeyFrameProxy requires that the animated property "
      "has an information property.");
    return;
    }

  unsigned int curv = static_cast<unsigned int>(this->GetKeyValue());
  unsigned int nextv = static_cast<unsigned int>(next->GetKeyValue());

  
  unsigned int numOfTimestepValues = timestepValues->GetNumberOfElements();
  if (curv >= numOfTimestepValues || nextv >= numOfTimestepValues)
    {
    vtkErrorMacro("Timestep value index is invalid.");
    return;
    }

  double t1 = timestepValues->GetElement(curv);
  double t2 = timestepValues->GetElement(nextv);

  double currentTimestep = t1 + (t2-t1) * currenttime;

  unsigned int vmin = (curv < nextv ? curv : nextv);
  unsigned int vmax = (curv < nextv ? nextv : curv);
  unsigned int index = vmin;

  // Now find the maximum index  <= currentTimestep.
  for (unsigned int cc=vmin+1; cc < vmax; ++cc)
    {
    if (timestepValues->GetElement(cc) > currentTimestep)
      {
      break;
      }
    index = cc;
    }
  domain->SetAnimationValue(property, animated_element, index);
  proxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMTimestepKeyFrameProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
