/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyClipboard.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyClipboard.h"

#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyLocator.h"
#include <cassert>
#include <map>
#include <sstream>

//============================================================================
class vtkSMProxyClipboardPropertyIterator : public vtkSMPropertyIterator
{
public:
  static vtkSMProxyClipboardPropertyIterator* New();
  vtkTypeMacro(vtkSMProxyClipboardPropertyIterator, vtkSMPropertyIterator);
  void Next() override
  {
    do
    {
      this->Superclass::Next();
    } while (!this->IsAtEnd() && this->Skip(this->GetKey(), this->GetProperty()));
  }
  void Begin() override
  {
    this->Superclass::Begin();
    if (!this->IsAtEnd() && this->Skip(this->GetKey(), this->GetProperty()))
    {
      this->Next();
    }
  }

protected:
  vtkSMProxyClipboardPropertyIterator() {}
  ~vtkSMProxyClipboardPropertyIterator() override {}

  bool Skip(const char* vtkNotUsed(pname), vtkSMProperty* prop) const
  {
    if (prop->GetPanelVisibility() == NULL || strcmp(prop->GetPanelVisibility(), "never") == 0)
    {
      return true;
    }
    if (vtkSMInputProperty::SafeDownCast(prop) != NULL)
    {
      // FIXME: don't skip selection inputs.
      return true;
    }
    else if (vtkSMProxyProperty::SafeDownCast(prop) &&
      (prop->GetRepeatable() || prop->FindDomain<vtkSMProxyListDomain>()))
    {
      // we skip repeatable properties to skip properties like Representations,
      // Props etc.
      return true;
    }
    return false;
  }

private:
  vtkSMProxyClipboardPropertyIterator(const vtkSMProxyClipboardPropertyIterator&);
  void operator=(const vtkSMProxyClipboardPropertyIterator&);
};
vtkStandardNewMacro(vtkSMProxyClipboardPropertyIterator);
//============================================================================

//============================================================================
class vtkSMProxyClipboardInternals
{
  vtkSmartPointer<vtkPVXMLElement> CopiedState;

  vtkPVXMLElement* Save(vtkSMProxy* source)
  {
    vtkNew<vtkSMProxyClipboardPropertyIterator> iter;
    iter->SetProxy(source);
    vtkPVXMLElement* sourceState = source->SaveXMLState(NULL, iter.GetPointer());
    if (!sourceState)
    {
      return NULL;
    }
    // Now save state for proxies on proxy list domains.
    vtkSmartPointer<vtkSMPropertyIterator> piter;
    piter.TakeReference(source->NewPropertyIterator());
    for (piter->Begin(); !piter->IsAtEnd(); piter->Next())
    {
      vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(piter->GetProperty());
      auto pld = pp ? pp->FindDomain<vtkSMProxyListDomain>() : nullptr;
      if (!pld)
      {
        continue;
      }
      if (pp->GetNumberOfProxies() == 0)
      {
        continue;
      }
      iter->SetProxy(source);
      vtkPVXMLElement* proxyState = this->Save(pp->GetProxy(0));
      if (proxyState)
      {
        vtkNew<vtkPVXMLElement> container;
        container->SetName("ClipboardState");
        container->SetAttribute("property_name", piter->GetKey());
        container->SetAttribute("value_xmlgroup", pp->GetProxy(0)->GetXMLGroup());
        container->SetAttribute("value_xmlname", pp->GetProxy(0)->GetXMLName());
        container->AddNestedElement(proxyState);
        proxyState->Delete();
        sourceState->AddNestedElement(container.GetPointer());
      }
    }
    return sourceState;
  }

  bool Load(vtkSMProxy* target, vtkPVXMLElement* targetState) const
  {
    if (!target || !targetState)
    {
      return true;
    }
    vtkNew<vtkSMProxyLocator> locator;
    locator->UseSessionToLocateProxy(true);
    locator->SetSession(target->GetSession());
    if (!target->LoadXMLState(targetState, locator.GetPointer()))
    {
      return false;
    }

    for (unsigned int cc = 0, max = targetState->GetNumberOfNestedElements(); cc < max; ++cc)
    {
      vtkPVXMLElement* elem = targetState->GetNestedElement(cc);
      if (elem == NULL || elem->GetName() == NULL || strcmp(elem->GetName(), "ClipboardState") != 0)
      {
        continue;
      }
      vtkSMProxyProperty* pp =
        vtkSMProxyProperty::SafeDownCast(target->GetProperty(elem->GetAttribute("property_name")));
      auto pld = pp ? pp->FindDomain<vtkSMProxyListDomain>() : nullptr;
      if (!pld)
      {
        continue;
      }
      vtkSMProxy* newValue =
        pld->FindProxy(elem->GetAttribute("value_xmlgroup"), elem->GetAttribute("value_xmlname"));
      if (!newValue || !this->Load(newValue, elem->GetNestedElement(0)))
      {
        continue;
      }
      pp->SetProxy(0, newValue);
    }
    target->UpdateVTKObjects();
    return true;
  }

public:
  bool CanPaste(vtkSMProxy* vtkNotUsed(target)) const { return this->CopiedState != NULL; }
  void Clear() { this->CopiedState = NULL; }

  bool Copy(vtkSMProxy* source)
  {
    this->CopiedState.TakeReference(this->Save(source));
    return this->CopiedState != NULL;
  }

  bool Paste(vtkSMProxy* source) const
  {
    if (this->CopiedState == NULL || source == NULL)
    {
      return false;
    }
    return this->Load(source, this->CopiedState);
  }
};
//============================================================================

vtkStandardNewMacro(vtkSMProxyClipboard);
//----------------------------------------------------------------------------
vtkSMProxyClipboard::vtkSMProxyClipboard()
  : Internals(new vtkSMProxyClipboardInternals())
{
}

//----------------------------------------------------------------------------
vtkSMProxyClipboard::~vtkSMProxyClipboard()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
bool vtkSMProxyClipboard::Copy(vtkSMProxy* source)
{
  this->Internals->Clear();
  if (!source)
  {
    return false;
  }
  return this->Internals->Copy(source);
}

//----------------------------------------------------------------------------
bool vtkSMProxyClipboard::CanPaste(vtkSMProxy* target)
{
  return (target != NULL) && this->Internals->CanPaste(target);
}

//----------------------------------------------------------------------------
bool vtkSMProxyClipboard::Paste(vtkSMProxy* target)
{
  if (!this->CanPaste(target))
  {
    return false;
  }

  return this->Internals->Paste(target);
}

//----------------------------------------------------------------------------
void vtkSMProxyClipboard::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
