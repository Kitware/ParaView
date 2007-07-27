/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSelectionLink.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSelectionLink.h"

#include "vtkObjectFactory.h"
#include "vtkSMPropertyLink.h"

vtkStandardNewMacro(vtkSMSelectionLink);
vtkCxxRevisionMacro(vtkSMSelectionLink, "1.1");
//----------------------------------------------------------------------------
vtkSMSelectionLink::vtkSMSelectionLink()
{
  this->PropertyLink = vtkSMPropertyLink::New();
}

//----------------------------------------------------------------------------
vtkSMSelectionLink::~vtkSMSelectionLink()
{
  this->PropertyLink->Delete();
}

//----------------------------------------------------------------------------
void vtkSMSelectionLink::AddSelectionLink(vtkSMProxy* proxy, int updateDir,
  const char* pname)
{
  this->PropertyLink->AddLinkedProperty(proxy, pname, updateDir);
}

//----------------------------------------------------------------------------
void vtkSMSelectionLink::RemoveAllLinks()
{
  this->PropertyLink->RemoveAllLinks();
}

//----------------------------------------------------------------------------
void vtkSMSelectionLink::RemoveSelectionLink(vtkSMProxy* proxy,
  const char* pname)
{
  this->PropertyLink->RemoveLinkedProperty(proxy, pname);
}

//----------------------------------------------------------------------------
void vtkSMSelectionLink::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


