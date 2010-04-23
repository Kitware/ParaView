/*=========================================================================

  Program:   ParaView
  Module:    pqProxyModifiedStateUndoElement.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "pqProxyModifiedStateUndoElement.h"

#include "pqApplicationCore.h"
#include "pqProxy.h"
#include "pqServerManagerModel.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"

vtkStandardNewMacro(pqProxyModifiedStateUndoElement);
//----------------------------------------------------------------------------
pqProxyModifiedStateUndoElement::pqProxyModifiedStateUndoElement()
{
}

//----------------------------------------------------------------------------
pqProxyModifiedStateUndoElement::~pqProxyModifiedStateUndoElement()
{
}

//----------------------------------------------------------------------------
bool pqProxyModifiedStateUndoElement::CanLoadState(vtkPVXMLElement* elem)
{
  return (elem && elem->GetName() && 
    strcmp(elem->GetName(), "ProxyModifiedState") == 0);
}

//----------------------------------------------------------------------------
void pqProxyModifiedStateUndoElement::MadeUnmodified(pqProxy* source)
{
  vtkPVXMLElement* elem = vtkPVXMLElement::New();
  elem->SetName("ProxyModifiedState");
  elem->AddAttribute("id", source->getProxy()->GetSelfIDAsString());
  elem->AddAttribute("reverse", 0);
  this->SetXMLElement(elem);
  elem->Delete();
}

//----------------------------------------------------------------------------
void pqProxyModifiedStateUndoElement::MadeUninitialized(pqProxy* source)
{
  vtkPVXMLElement* elem = vtkPVXMLElement::New();
  elem->SetName("ProxyModifiedState");
  elem->AddAttribute("id", source->getProxy()->GetSelfIDAsString());
  elem->AddAttribute("reverse", 1);
  this->SetXMLElement(elem);
  elem->Delete();
}

//----------------------------------------------------------------------------
bool pqProxyModifiedStateUndoElement::InternalUndoRedo(bool undo)
{
  vtkPVXMLElement* element = this->XMLElement;
  int id = 0;
  element->GetScalarAttribute("id",&id);
  if (!id)
    {
    vtkErrorMacro("Failed to locate proxy id.");
    return false;
    }

  int reverse = 0;
  element->GetScalarAttribute("reverse", &reverse);

  vtkSMProxyLocator* locator = this->GetProxyLocator();
  vtkSMProxy* proxy = locator->LocateProxy(id);

  if (!proxy)
    {
    vtkErrorMacro("Failed to locate the proxy to register.");
    return false;
    }

  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smModel = core->getServerManagerModel();
  pqProxy* pqproxy = smModel->findItem<pqProxy*>(proxy);
  if (pqproxy && !reverse)
    {
    pqproxy->setModifiedState(undo? pqProxy::UNINITIALIZED :
      pqProxy::UNMODIFIED);
    }
  else if (pqproxy && reverse)
    {
    pqproxy->setModifiedState(undo? pqProxy::UNMODIFIED:
      pqProxy::UNINITIALIZED);
    }
  return true;
}

//----------------------------------------------------------------------------
void pqProxyModifiedStateUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


