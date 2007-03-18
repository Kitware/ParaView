/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUpdateInformationUndoElement.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMUpdateInformationUndoElement.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDefaultStateLoader.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMUpdateInformationUndoElement);
vtkCxxRevisionMacro(vtkSMUpdateInformationUndoElement, "1.1");
//-----------------------------------------------------------------------------
vtkSMUpdateInformationUndoElement::vtkSMUpdateInformationUndoElement()
{
}

//-----------------------------------------------------------------------------
vtkSMUpdateInformationUndoElement::~vtkSMUpdateInformationUndoElement()
{
}

//-----------------------------------------------------------------------------
bool vtkSMUpdateInformationUndoElement::CanLoadState(vtkPVXMLElement* elem)
{
  return (elem && elem->GetName() && 
    strcmp(elem->GetName(), "UpdateInformation") == 0);
}

//-----------------------------------------------------------------------------
void vtkSMUpdateInformationUndoElement::Updated(vtkSMProxy* proxy)
{
  vtkPVXMLElement* element = vtkPVXMLElement::New();
  element->SetName("UpdateInformation");

  element->AddAttribute("id", proxy->GetSelfIDAsString());
  this->SetXMLElement(element);
  element->Delete();
}

//-----------------------------------------------------------------------------
int vtkSMUpdateInformationUndoElement::Undo()
{
  // Nothing to do.
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSMUpdateInformationUndoElement::Redo()
{
  if (!this->XMLElement)
    {
    vtkErrorMacro("No State present to redo.");
    return 0;
    }
  int proxy_id;
  this->XMLElement->GetScalarAttribute("id", &proxy_id);

  vtkSMDefaultStateLoader* stateLoader = vtkSMDefaultStateLoader::New();
  stateLoader->SetConnectionID(this->ConnectionID);

  vtkSMProxy* proxy = stateLoader->NewProxy(proxy_id);
  if (proxy)
    {
    // When undo-redoing, UpdateVTKObjects() doesn't get called
    // until the end.
    proxy->UpdateVTKObjects();
    vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(proxy);
    if (sp)
      {
      sp->UpdatePipelineInformation();
      }
    proxy->UpdatePropertyInformation();
    proxy->Delete();
    }
  stateLoader->Delete();
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMUpdateInformationUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
