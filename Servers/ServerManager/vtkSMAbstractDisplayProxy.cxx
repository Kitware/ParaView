/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAbstractDisplayProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMAbstractDisplayProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMStringVectorProperty.h"

vtkCxxRevisionMacro(vtkSMAbstractDisplayProxy, "1.1");

//-----------------------------------------------------------------------------
vtkSMAbstractDisplayProxy::vtkSMAbstractDisplayProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMAbstractDisplayProxy::~vtkSMAbstractDisplayProxy()
{
}
 
//-----------------------------------------------------------------------------
void vtkSMAbstractDisplayProxy::SetVisibilityCM(int v)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("Visibility"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Visibility on DisplayProxy.");
    return;
    }
  ivp->SetElement(0, v);
  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
int vtkSMAbstractDisplayProxy::GetVisibilityCM()
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("Visibility"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property Visibility on DisplayProxy.");
    return 0;
    }
  return ivp->GetElement(0);
}

//-----------------------------------------------------------------------------
void vtkSMAbstractDisplayProxy::SaveInBatchScript(ofstream* file)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Display Proxy not created!");
    return;
    }
    
  *file << endl;
  *file << "set pvTemp" << this->GetSelfIDAsString()
        << " [$proxyManager NewProxy " << this->GetXMLGroup() << " "
        << this->GetXMLName() << "]" << endl;
  *file << "  $proxyManager RegisterProxy " << this->GetXMLGroup()
        << " pvTemp" << this->GetSelfIDAsString() <<" $pvTemp" 
        << this->GetSelfIDAsString() << endl;
  *file << "  $pvTemp" << this->GetSelfIDAsString() << " UnRegister {}" 
        << endl;
  
  //First set the input to the display.
  vtkSMInputProperty* ipp;
  ipp = vtkSMInputProperty::SafeDownCast(
    this->GetProperty("Input"));
  if (ipp && ipp->GetNumberOfProxies() > 0)
    {
    *file << "  [$pvTemp" << this->GetSelfIDAsString() 
          << " GetProperty Input] AddProxy $pvTemp" 
          << ipp->GetProxy(0)->GetSelfIDAsString()
          << endl;
    }
  else
    {
    *file << "# Input to Display Proxy not set properly or takes no Input." 
          << endl;
    }
  
  // Now, we save all the properties that are not Input.
  // Also note that only exposed properties are getting saved.
  
  vtkSMPropertyIterator* iter = this->NewPropertyIterator();
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProperty* p = iter->GetProperty();
    if (vtkSMInputProperty::SafeDownCast(p))
      {
      // Input property has already been saved...so skip it.
      continue;
      }
    
    if (p->GetIsInternal())
      {
      *file << "  # skipping internal property " << iter->GetKey() << endl;
      continue;
      }
    
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(p);
    vtkSMDoubleVectorProperty* dvp = 
      vtkSMDoubleVectorProperty::SafeDownCast(p);
    vtkSMStringVectorProperty* svp = 
      vtkSMStringVectorProperty::SafeDownCast(p);
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(p);
    if (ivp)
      {
      for (unsigned int i=0; i < ivp->GetNumberOfElements(); i++)
        {
        *file << "  [$pvTemp" << this->GetSelfIDAsString() << " GetProperty "
              << iter->GetKey() << "] SetElement "
              << i << " " << ivp->GetElement(i) 
              << endl;
        }
      }
    else if (dvp)
      {
      for (unsigned int i=0; i < dvp->GetNumberOfElements(); i++)
        {
        *file << "  [$pvTemp" << this->GetSelfIDAsString() << " GetProperty "
              << iter->GetKey() << "] SetElement "
              << i << " " << dvp->GetElement(i) 
              << endl;
        }
      }
    else if (svp)
      {
      for (unsigned int i=0; i < svp->GetNumberOfElements(); i++)
        {
        *file << "  [$pvTemp" << this->GetSelfIDAsString() << " GetProperty "
              << iter->GetKey() << "] SetElement "
              << i << " {" << svp->GetElement(i) << "}"
              << endl;
        }
      }
    else if (pp)
      {
      for (unsigned int i=0; i < pp->GetNumberOfProxies(); i++)
        {
        *file << "  [$pvTemp" << this->GetSelfIDAsString() << " GetProperty "
              << iter->GetKey() << "] AddProxy $pvTemp"
              << pp->GetProxy(i)->GetSelfIDAsString() << endl;
        }
      }
    else
      {
      *file << "  # skipping property " << iter->GetKey() << endl;
      }
    }
  
  iter->Delete();
  *file << "  $pvTemp" << this->GetSelfIDAsString() 
        << " UpdateVTKObjects" << endl;
}

//-----------------------------------------------------------------------------
void vtkSMAbstractDisplayProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

