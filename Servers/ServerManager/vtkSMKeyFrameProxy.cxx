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
#include "vtkSMAnimationCueProxy.h"
#include "vtkObjectFactory.h"
#include "vtkClientServerID.h"

#include <vtkstd/vector>
//----------------------------------------------------------------------------
class vtkSMKeyFrameProxyInternals 
{
public:
  typedef vtkstd::vector<double> VectorOfDoubles;
  VectorOfDoubles KeyValues;
};
//----------------------------------------------------------------------------


vtkCxxRevisionMacro(vtkSMKeyFrameProxy, "1.8");
vtkStandardNewMacro(vtkSMKeyFrameProxy);
//----------------------------------------------------------------------------
vtkSMKeyFrameProxy::vtkSMKeyFrameProxy()
{
  this->KeyTime = -1.0;
  this->ObjectsCreated = 1; //no serverside objects for this proxy.
  this->Internals = new vtkSMKeyFrameProxyInternals;
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
void vtkSMKeyFrameProxy::SaveInBatchScript(ofstream* file)
{
  *file << endl;
  *file << "set pvTemp" << this->GetSelfIDAsString()
    << " [$proxyManager NewProxy " 
    << this->GetXMLGroup() <<" "
    << this->GetXMLName() << "]" << endl;

  vtkSMKeyFrameProxyInternals::VectorOfDoubles::iterator iter = 
    this->Internals->KeyValues.begin();
  int i = 0;
  for (; iter != this->Internals->KeyValues.end(); ++iter)
    {
    *file << "[$pvTemp" << this->GetSelfIDAsString() << " GetProperty KeyValues]"
      << " SetElement " << i << " " << (*iter) << endl;
    i++;
    }

  *file << "[$pvTemp" << this->GetSelfIDAsString() << " GetProperty KeyTime]"
    << " SetElements1 " << this->KeyTime << endl;
  *file << "$pvTemp" << this->GetSelfIDAsString() << " UpdateVTKObjects" << endl;
}

//----------------------------------------------------------------------------
void vtkSMKeyFrameProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "KeyTime: " << this->KeyTime << endl;
}
