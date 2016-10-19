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
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"

vtkStandardNewMacro(vtkSMGlobalPropertiesLinkUndoElement);
//----------------------------------------------------------------------------
vtkSMGlobalPropertiesLinkUndoElement::vtkSMGlobalPropertiesLinkUndoElement()
{
  this->GlobalPropertyManagerName = NULL;
  this->GlobalPropertyName = NULL;
  this->ProxyPropertyName = NULL;
  this->ProxyGlobalID = 0;
}

//----------------------------------------------------------------------------
vtkSMGlobalPropertiesLinkUndoElement::~vtkSMGlobalPropertiesLinkUndoElement()
{
  this->SetGlobalPropertyManagerName(NULL);
  this->SetGlobalPropertyName(NULL);
  this->SetProxyPropertyName(NULL);
}

//-----------------------------------------------------------------------------
void vtkSMGlobalPropertiesLinkUndoElement::SetLinkState(const char* mgrname,
  const char* globalpropname, vtkSMProxy* proxy, const char* propname, bool isRegisterAction)
{
  this->SetGlobalPropertyManagerName(mgrname);
  this->SetGlobalPropertyName(globalpropname);
  this->SetProxyPropertyName(propname);
  this->ProxyGlobalID = proxy->GetGlobalID();
  this->IsLinkAdded = isRegisterAction;
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
  (void)undo;
  // FIXME:
  //  if (this->ProxyGlobalID == 0)
  //    {
  //    vtkErrorMacro("No State present to undo.");
  //    return 0;
  //    }
  //
  //  vtkSMSessionProxyManager* pxm =
  //      vtkSMProxyManager::GetProxyManager()->GetSessionProxyManager(this->GetSession());
  //  vtkSMProxy* proxy =
  //      vtkSMProxy::SafeDownCast(
  //          this->GetSession()->GetRemoteObject(this->ProxyGlobalID));
  //  vtkSMGlobalPropertiesManager* propertyManager =
  //      pxm->GetGlobalPropertiesManager(this->GlobalPropertyManagerName);
  //
  //  if ( (undo && !this->IsLinkAdded) || (!undo && this->IsLinkAdded) )
  //    {
  //    propertyManager->SetGlobalPropertyLink( this->GlobalPropertyName,
  //                                            proxy, this->ProxyPropertyName);
  //    }
  //  else
  //    {
  //    propertyManager->RemoveGlobalPropertyLink( this->GlobalPropertyName,
  //                                               proxy, this->ProxyPropertyName);
  //    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkSMGlobalPropertiesLinkUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
