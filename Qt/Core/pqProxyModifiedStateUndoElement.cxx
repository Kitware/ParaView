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
#include "vtkSMSession.h"

vtkStandardNewMacro(pqProxyModifiedStateUndoElement);
//----------------------------------------------------------------------------
pqProxyModifiedStateUndoElement::pqProxyModifiedStateUndoElement() = default;

//----------------------------------------------------------------------------
pqProxyModifiedStateUndoElement::~pqProxyModifiedStateUndoElement() = default;

//----------------------------------------------------------------------------
void pqProxyModifiedStateUndoElement::MadeUnmodified(pqProxy* source)
{
  this->ProxySourceGlobalId = source->getProxy()->GetGlobalID();
  this->Reverse = false;
}

//----------------------------------------------------------------------------
void pqProxyModifiedStateUndoElement::MadeUninitialized(pqProxy* source)
{
  this->ProxySourceGlobalId = source->getProxy()->GetGlobalID();
  this->Reverse = true;
}

//----------------------------------------------------------------------------
bool pqProxyModifiedStateUndoElement::InternalUndoRedo(bool undo)
{
  vtkSMProxy* proxy =
    vtkSMProxy::SafeDownCast(this->GetSession()->GetRemoteObject(this->ProxySourceGlobalId));

  if (!proxy)
  {
    vtkErrorMacro("Failed to locate the proxy to register.");
    return false;
  }

  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smModel = core->getServerManagerModel();
  pqProxy* pqproxy = smModel->findItem<pqProxy*>(proxy);
  if (pqproxy && !this->Reverse)
  {
    pqproxy->setModifiedState(undo ? pqProxy::UNINITIALIZED : pqProxy::UNMODIFIED);
  }
  else if (pqproxy && this->Reverse)
  {
    pqproxy->setModifiedState(undo ? pqProxy::UNMODIFIED : pqProxy::UNINITIALIZED);
  }
  return true;
}

//----------------------------------------------------------------------------
void pqProxyModifiedStateUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
