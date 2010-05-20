/*=========================================================================

  Program:   ParaView
  Module:    vtkSMKeyFrameProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMKeyFrameProxy.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMAnimationCueProxy.h"

#include <vtkstd/vector>
//----------------------------------------------------------------------------
class vtkSMKeyFrameProxyInternals 
{
public:
  typedef vtkstd::vector<double> VectorOfDoubles;
  VectorOfDoubles KeyValues;
};
//----------------------------------------------------------------------------


vtkStandardNewMacro(vtkSMKeyFrameProxy);
//----------------------------------------------------------------------------
vtkSMKeyFrameProxy::vtkSMKeyFrameProxy()
{
  this->KeyTime = -1.0;
  this->Internals = new vtkSMKeyFrameProxyInternals;
  this->SetServers(vtkProcessModule::CLIENT);
}

//----------------------------------------------------------------------------
vtkSMKeyFrameProxy::~vtkSMKeyFrameProxy()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkSMKeyFrameProxy::UpdateValue(double vtkNotUsed(currenttime), 
  vtkSMAnimationCueProxy* vtkNotUsed(cueProxy), vtkSMKeyFrameProxy* vtkNotUsed(next))
{
}

//----------------------------------------------------------------------------
void vtkSMKeyFrameProxy::RemoveAllKeyValues()
{
  this->Internals->KeyValues.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSMKeyFrameProxy::SetKeyValue(unsigned int index, double value)
{
  if (index >= this->GetNumberOfKeyValues())
    {
    this->SetNumberOfKeyValues(index+1);
    }
  this->Internals->KeyValues[index] = value;
  this->Modified();
}
//----------------------------------------------------------------------------
void vtkSMKeyFrameProxy::Copy(vtkSMProxy* src, const char* exceptionClass, 
    int proxyPropertyCopyFlag)
{
  this->Superclass::Copy(src, exceptionClass, proxyPropertyCopyFlag);
  this->MarkAllPropertiesAsModified();
}

//----------------------------------------------------------------------------
double vtkSMKeyFrameProxy::GetKeyValue(unsigned int index)
{
  if (index >= this->GetNumberOfKeyValues())
    {
    return 0.0;
    }
  return this->Internals->KeyValues[index];
}

//----------------------------------------------------------------------------
void vtkSMKeyFrameProxy::SetNumberOfKeyValues(unsigned int num)
{
  this->Internals->KeyValues.resize(num);
}

//----------------------------------------------------------------------------
unsigned int vtkSMKeyFrameProxy::GetNumberOfKeyValues()
{
  return this->Internals->KeyValues.size();
}

//----------------------------------------------------------------------------
void vtkSMKeyFrameProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "KeyTime: " << this->KeyTime << endl;
}
