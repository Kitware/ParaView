/*=========================================================================

  Program:   ParaView
  Module:    vtkSMGlobalPropertiesLinkUndoElement.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMGlobalPropertiesLinkUndoElement.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxyManager.h"
#include "vtkSMGlobalPropertiesManager.h"
#include "vtkSMProxyLocator.h"

vtkStandardNewMacro(vtkSMGlobalPropertiesLinkUndoElement);
//----------------------------------------------------------------------------
vtkSMGlobalPropertiesLinkUndoElement::vtkSMGlobalPropertiesLinkUndoElement()
{
}

//----------------------------------------------------------------------------
vtkSMGlobalPropertiesLinkUndoElement::~vtkSMGlobalPropertiesLinkUndoElement()
{
}

//-----------------------------------------------------------------------------
bool vtkSMGlobalPropertiesLinkUndoElement::CanLoadState(vtkPVXMLElement* elem)
{
  return (elem && elem->GetName() && 
    strcmp(elem->GetName(), "GlobalPropertiesLink") == 0);
}

//----------------------------------------------------------------------------
void vtkSMGlobalPropertiesLinkUndoElement::LinkAdded(const char* mgrname,
  const char* globalpropname,
  vtkSMProxy* proxy, const char* propname)
{
  vtkPVXMLElement* elem = vtkPVXMLElement::New();
  elem->SetName("GlobalPropertiesLink");
  elem->AddAttribute("mgrname", mgrname);
  elem->AddAttribute("globalpropname", globalpropname);
  elem->AddAttribute("id", proxy->GetSelfIDAsString());
  elem->AddAttribute("propname", propname);
  elem->AddAttribute("added", 1);
  this->SetXMLElement(elem);
  elem->Delete();
}

//----------------------------------------------------------------------------
void vtkSMGlobalPropertiesLinkUndoElement::LinkRemoved(const char* mgrname,
  const char* globalpropname,
  vtkSMProxy* proxy, const char* propname)
{
  vtkPVXMLElement* elem = vtkPVXMLElement::New();
  elem->SetName("GlobalPropertiesLink");
  elem->AddAttribute("mgrname", mgrname);
  elem->AddAttribute("globalpropname", globalpropname);
  elem->AddAttribute("id", proxy->GetSelfIDAsString());
  elem->AddAttribute("propname", propname);
  elem->AddAttribute("removed", 1);
  this->SetXMLElement(elem);
  elem->Delete();
}

//----------------------------------------------------------------------------
int vtkSMGlobalPropertiesLinkUndoElement::Undo()
{
  return this->UndoRedoInternal(true);
}

//----------------------------------------------------------------------------
int vtkSMGlobalPropertiesLinkUndoElement::Redo()
{
  return this->UndoRedoInternal(false);
}

//----------------------------------------------------------------------------
int vtkSMGlobalPropertiesLinkUndoElement::UndoRedoInternal(bool undo)
{
  if (!this->XMLElement)
    {
    vtkErrorMacro("No State present to undo.");
    return 0;
    }

  int proxy_id;
  this->XMLElement->GetScalarAttribute("id", &proxy_id);
  const char* propname = this->XMLElement->GetAttribute("propname");
  const char* globalpropname = this->XMLElement->GetAttribute("globalpropname");
  const char* mgrname = this->XMLElement->GetAttribute("mgrname");

  vtkSMProxyLocator* locator = this->GetProxyLocator();
  vtkSMProxy* proxy = locator->LocateProxy(proxy_id);
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  bool add = (this->XMLElement->GetAttribute("added") != NULL);
  if (undo)
    {
    add = !add;
    }

  if (add)
    {
    pxm->GetGlobalPropertiesManager(mgrname)->SetGlobalPropertyLink(
      globalpropname, proxy, propname);
    }
  else
    {
    pxm->GetGlobalPropertiesManager(mgrname)->RemoveGlobalPropertyLink(
      globalpropname, proxy, propname);
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkSMGlobalPropertiesLinkUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


