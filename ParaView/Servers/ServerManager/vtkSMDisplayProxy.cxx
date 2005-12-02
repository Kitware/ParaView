/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDisplayProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDisplayProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMDisplayProxy);
vtkCxxRevisionMacro(vtkSMDisplayProxy, "1.7");

//-----------------------------------------------------------------------------
vtkSMDisplayProxy::vtkSMDisplayProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMDisplayProxy::~vtkSMDisplayProxy()
{
}
 
//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMDisplayProxy::GetInteractorProxy(vtkSMRenderModuleProxy* ren)
{
  return (ren?ren->GetInteractorProxy():0);
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMDisplayProxy::GetRendererProxy(vtkSMRenderModuleProxy* ren)
{
  return (ren?ren->GetRendererProxy():0);
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMDisplayProxy::GetRenderer2DProxy(vtkSMRenderModuleProxy* ren)
{
  return (ren?ren->GetRenderer2DProxy():0);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::AddPropToRenderer(
  vtkSMProxy* proxy, vtkSMRenderModuleProxy* ren)
{
  ren->AddPropToRenderer(proxy);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::AddPropToRenderer2D(
  vtkSMProxy* proxy, vtkSMRenderModuleProxy* ren)
{
  ren->AddPropToRenderer2D(proxy);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::RemovePropFromRenderer(
  vtkSMProxy* proxy, vtkSMRenderModuleProxy* ren)
{
  ren->RemovePropFromRenderer(proxy);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::RemovePropFromRenderer2D(
  vtkSMProxy* proxy, vtkSMRenderModuleProxy* ren)
{
  ren->RemovePropFromRenderer2D(proxy);
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::AddToRenderModule(vtkSMRenderModuleProxy* rm)
{
  vtkSMProxy* p = this->GetSubProxy("Prop");
  if (p)
    {
    this->AddPropToRenderer(p, rm);
    }
  p = this->GetSubProxy("Prop2D");
  if (p)
    {
    this->AddPropToRenderer2D(p, rm);
    }
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::RemoveFromRenderModule(vtkSMRenderModuleProxy* rm)
{
  vtkSMProxy* p = this->GetSubProxy("Prop");
  if (p)
    {
    this->RemovePropFromRenderer(p, rm);
    }
  p = this->GetSubProxy("Prop2D");
  if (p)
    {
    this->RemovePropFromRenderer2D(p, rm);
    }
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::SetVisibilityCM(int v)
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
int vtkSMDisplayProxy::GetVisibilityCM()
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
void vtkSMDisplayProxy::SaveInBatchScript(ofstream* file)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Display Proxy not created!");
    return;
    }

  // Some displays do not have VTKClassName set and hence only create Subproxies.
  // For such displays we use their self ids. 
  
  unsigned int count = this->GetNumberOfIDs();
  vtkClientServerID id = (count)? this->GetID(0) : this->GetSelfID();
  count = (count)? count : 1;
   
  for (unsigned int kk = 0; kk < count ; kk++)
    {
    if (kk > 0)
      {
      id = this->GetID(kk);
      }
    
    *file << endl;
    *file << "set pvTemp" << id
      << " [$proxyManager NewProxy " << this->GetXMLGroup() << " "
      << this->GetXMLName() << "]" << endl;
    *file << "  $proxyManager RegisterProxy " << this->GetXMLGroup()
      << " pvTemp" << id <<" $pvTemp" << id << endl;
    *file << "  $pvTemp" << id << " UnRegister {}" << endl;

    //First set the input to the display.
    vtkSMInputProperty* ipp;
    ipp = vtkSMInputProperty::SafeDownCast(
      this->GetProperty("Input"));
    if (ipp && ipp->GetNumberOfProxies() > 0)
      {
      *file << "  [$pvTemp" << id << " GetProperty Input] "
        " AddProxy $pvTemp" << ipp->GetProxy(0)->GetID(0)
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

      if (!p->GetSaveable())
        {
        *file << "  # skipping not-saveable property " << p->GetXMLName() << endl;
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
          *file << "  [$pvTemp" << id << " GetProperty "
            << ivp->GetXMLName() << "] SetElement "
            << i << " " << ivp->GetElement(i) 
            << endl;
          }
        }
      else if (dvp)
        {
        for (unsigned int i=0; i < dvp->GetNumberOfElements(); i++)
          {
          *file << "  [$pvTemp" << id << " GetProperty "
            << dvp->GetXMLName() << "] SetElement "
            << i << " " << dvp->GetElement(i) 
            << endl;
          }
        }
      else if (svp)
        {
        for (unsigned int i=0; i < svp->GetNumberOfElements(); i++)
          {
          *file << "  [$pvTemp" << id << " GetProperty "
            << svp->GetXMLName() << "] SetElement "
            << i << " {" << svp->GetElement(i) << "}"
            << endl;
          }
        }
      else if (pp)
        {
        for (unsigned int i=0; i < pp->GetNumberOfProxies(); i++)
          {
          *file << "  [$pvTemp" << id << " GetProperty "
            << pp->GetXMLName() << "] AddProxy $pvTemp"
            << pp->GetProxy(i)->GetID(0) << endl;
          }
        }
      else
        {
        *file << "  # skipping property " << p->GetXMLName() << endl;
        }
      }

    iter->Delete();
    *file << "  $pvTemp" << id << " UpdateVTKObjects" << endl;
    }
}

//-----------------------------------------------------------------------------
void vtkSMDisplayProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
