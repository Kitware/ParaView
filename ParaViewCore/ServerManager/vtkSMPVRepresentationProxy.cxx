/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPVRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPVRepresentationProxy.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"

#include <vtkstd/set>
#include <vtkstd/string>

class vtkSMPVRepresentationProxy::vtkStringSet :
  public vtkstd::set<vtkstd::string> {};

vtkStandardNewMacro(vtkSMPVRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMPVRepresentationProxy::vtkSMPVRepresentationProxy()
{
  this->SetSIClassName("vtkSIPVRepresentationProxy");
  this->RepresentationSubProxies = new vtkStringSet();
  this->InReadXMLAttributes = false;
}

//----------------------------------------------------------------------------
vtkSMPVRepresentationProxy::~vtkSMPVRepresentationProxy()
{
  delete this->RepresentationSubProxies;
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects();
  if (!this->ObjectsCreated)
    {
    return;
    }

  // Ensure that we update the RepresentationTypesInfo property and the domain
  // for "Representations" property before CreateVTKObjects() is finished. This
  // ensure that all representations have valid Representations domain.
  this->UpdatePropertyInformation();
  this->GetProperty("RepresentationTypesInfo")->UpdateDependentDomains();

  // Whenever the "Representation" property is modified, we ensure that the
  // this->InvalidateDataInformation() is called.
  this->AddObserver(vtkCommand::UpdatePropertyEvent,
    this, &vtkSMPVRepresentationProxy::OnPropertyUpdated);
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::OnPropertyUpdated(vtkObject*,
  unsigned long, void* calldata)
{
  const char* pname = reinterpret_cast<const char*>(calldata);
  if (pname && strcmp(pname, "Representation") == 0)
    {
    this->InvalidateDataInformation();
    }
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::SetPropertyModifiedFlag(const char* name, int flag)
{
  if (!this->InReadXMLAttributes && name && strcmp(name, "Input") == 0)
    {
    vtkSMProxy* selectionRepr = this->GetSubProxy("SelectionRepresentation");
    vtkSMPropertyHelper helper(this, name);
    for (unsigned int cc=0; cc < helper.GetNumberOfElements(); cc++)
      {
      vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(
        helper.GetAsProxy(cc));
      if (input && selectionRepr)
        {
        input->CreateSelectionProxies();
        vtkSMSourceProxy* esProxy = input->GetSelectionOutput(
          helper.GetOutputPort(cc));
        if (!esProxy)
          {
          vtkErrorMacro("Input proxy does not support selection extraction.");
          }
        else
          {
          vtkSMPropertyHelper(selectionRepr, "Input").Set(esProxy);
          selectionRepr->UpdateVTKObjects();
          }
        }
      }

    // Next we ensure that input is set on all input properties for the
    // subproxies. This ensures that domains and such on subproxies can still
    // function correctly.

    // This piece of code is a disgrace :). We shouldn't have to do these kinds of
    // hacks. It implies that something's awry with the way domains and
    // updated/defined for sub-proxies.
    for (vtkStringSet::iterator iter = this->RepresentationSubProxies->begin();
      iter != this->RepresentationSubProxies->end(); ++iter)
      {
      vtkSMProxy* subProxy = this->GetSubProxy((*iter).c_str());
      if (subProxy && subProxy->GetProperty("Input"))
        {
        subProxy->GetProperty("Input")->Copy(this->GetProperty("Input"));
        subProxy->UpdateProperty("Input");
        subProxy->GetProperty("Input")->UpdateDependentDomains();
        }
      }
    }

  this->Superclass::SetPropertyModifiedFlag(name, flag);
}

//----------------------------------------------------------------------------
int vtkSMPVRepresentationProxy::ReadXMLAttributes(
  vtkSMProxyManager* pm, vtkPVXMLElement* element)
{
  this->InReadXMLAttributes = true;
  for (unsigned int cc=0; cc < element->GetNumberOfNestedElements(); ++cc)
    {
    vtkPVXMLElement* child = element->GetNestedElement(cc);
    if ( child->GetName() &&
         strcmp(child->GetName(), "RepresentationType") == 0 &&
         child->GetAttribute("subproxy") != NULL )
      {
      this->RepresentationSubProxies->insert(child->GetAttribute("subproxy"));
      }
    }

  int retVal = this->Superclass::ReadXMLAttributes(pm, element);
  this->InReadXMLAttributes = false;
  return retVal;
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


