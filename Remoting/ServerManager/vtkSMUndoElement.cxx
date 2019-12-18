/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUndoElement.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMUndoElement.h"

#include "vtkObjectFactory.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkWeakPointer.h"

//-----------------------------------------------------------------------------
vtkSMUndoElement::vtkSMUndoElement()
{
  this->Session = 0;
}

//-----------------------------------------------------------------------------
vtkSMUndoElement::~vtkSMUndoElement()
{
  this->SetSession(0);
}

//-----------------------------------------------------------------------------
void vtkSMUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSMUndoElement::SetSession(vtkSMSession* session)
{
  if (this->Session != session)
  {
    this->Session = session;
    this->Modified();
  }
}
//----------------------------------------------------------------------------
vtkSMSession* vtkSMUndoElement::GetSession()
{
  return this->Session.GetPointer();
}
//----------------------------------------------------------------------------
vtkSMSessionProxyManager* vtkSMUndoElement::GetSessionProxyManager()
{
  return vtkSMProxyManager::GetProxyManager()->GetSessionProxyManager(this->Session);
}
