/*=========================================================================

  Program:   ParaView
  Module:    vtkSMObject.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMObject.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule2.h"
#include "vtkSMSession.h"

vtkStandardNewMacro(vtkSMObject);

//---------------------------------------------------------------------------
vtkSMObject::vtkSMObject()
{
}

//---------------------------------------------------------------------------
vtkSMObject::~vtkSMObject()
{
}

////---------------------------------------------------------------------------
vtkSMProxyManager* vtkSMObject::GetProxyManager()
{
  // Locate proxy manager from first SMSession and return it for backwards
  // compatibility.
  vtkSMSession* session = vtkSMSession::SafeDownCast(
    vtkProcessModule2::GetProcessModule()->GetSession());
  return session? session->GetProxyManager() : NULL;
}

////---------------------------------------------------------------------------
//vtkSMApplication* vtkSMObject::GetApplication()
//{
//  abort();
//  // FIXME
//  // return vtkSMApplication::GetInstance();
//  //return vtkSMObject::Application;
//  return NULL;
//}
//
//---------------------------------------------------------------------------
void vtkSMObject::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
