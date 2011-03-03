/*=========================================================================

  Program:   ParaView
  Module:    vtkSMComparativeAnimationCueProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMComparativeAnimationCueProxy.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVComparativeAnimationCue.h"
#include "vtkPVSession.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDomain.h"

vtkStandardNewMacro(vtkSMComparativeAnimationCueProxy);
//----------------------------------------------------------------------------
vtkSMComparativeAnimationCueProxy::vtkSMComparativeAnimationCueProxy()
{
  this->SetLocation(vtkPVSession::CLIENT);
}

//----------------------------------------------------------------------------
vtkSMComparativeAnimationCueProxy::~vtkSMComparativeAnimationCueProxy()
{
}

//----------------------------------------------------------------------------
vtkPVComparativeAnimationCue* vtkSMComparativeAnimationCueProxy::GetCue()
{
  this->CreateVTKObjects();
  return vtkPVComparativeAnimationCue::SafeDownCast(
    this->GetClientSideObject());
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMComparativeAnimationCueProxy::SaveXMLState(
  vtkPVXMLElement* root, vtkSMPropertyIterator *piter)
{
  vtkPVComparativeAnimationCue* vtkClass =
      vtkPVComparativeAnimationCue::SafeDownCast(this->GetClientSideObject());
  return vtkClass->AppendCommandInfo(this->Superclass::SaveXMLState(root, piter));
}

//----------------------------------------------------------------------------
int vtkSMComparativeAnimationCueProxy::LoadXMLState(
  vtkPVXMLElement* proxyElement, vtkSMProxyLocator* locator)
{
  if (!this->Superclass::LoadXMLState(proxyElement, locator))
    {
    return 0;
    }
  vtkPVComparativeAnimationCue* vtkClass =
      vtkPVComparativeAnimationCue::SafeDownCast(this->GetClientSideObject());
  vtkClass->LoadCommandInfo(proxyElement);
  this->Modified();
  return 1;
}

//----------------------------------------------------------------------------
#ifdef FIXME_COLLABORATION

int vtkSMComparativeAnimationCueProxy::RevertState(
  vtkPVXMLElement* proxyElement, vtkSMProxyLocator* vtkNotUsed(locator))
{
  unsigned int numElems = proxyElement->GetNumberOfNestedElements();
  for (unsigned int i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = proxyElement->GetNestedElement(i);
    const char* name =  currentElement->GetName();
    if (name && strcmp(name, "CueCommand") == 0)
      {
      vtkInternals::vtkCueCommand cmd;
      if (cmd.FromXML(currentElement) == false)
        {
        vtkErrorMacro("Error when loading CueCommand.");
        return 0;
        }

      int remove = 0;
      int position = -1;
      currentElement->GetScalarAttribute("remove", &remove);
      currentElement->GetScalarAttribute("position", &position);
      if (remove)
        {
        this->Internals->InsertCommand(cmd, position);
        }
      else
        {
        this->Internals->RemoveCommand(cmd);
        }
      }
    }
  this->Modified();
  return 1;
}
#endif

//----------------------------------------------------------------------------
void vtkSMComparativeAnimationCueProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
