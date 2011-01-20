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

#ifdef FIXME_COLLABORATION
//----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMComparativeAnimationCueProxy::SaveState(
  vtkPVXMLElement* root, vtkSMPropertyIterator *piter, int saveSubProxies)
{
  vtkPVXMLElement* proxyElem = this->Superclass::SaveState(
    root, piter, saveSubProxies);
  if (!proxyElem)
    {
    return NULL;
    }

  vtkstd::vector<vtkInternals::vtkCueCommand>::iterator iter;
  for (iter = this->Internals->CommandQueue.begin();
    iter != this->Internals->CommandQueue.end(); ++iter)
    {
    vtkPVXMLElement* commandElem = iter->ToXML();
    proxyElem->AddNestedElement(commandElem);
    commandElem->Delete();
    }
  return proxyElem;
}

//----------------------------------------------------------------------------
int vtkSMComparativeAnimationCueProxy::LoadState(
  vtkPVXMLElement* proxyElement, vtkSMProxyLocator* locator)
{
  if (!this->Superclass::LoadState(proxyElement, locator))
    {
    return 0;
    }

  bool state_change_xml = (strcmp(proxyElement->GetName(), "StateChange") != 0);
  if (state_change_xml)
    {
    // unless the state being loaded is a StateChange, we start from scratch.
    this->Internals->CommandQueue.clear();
    }

  // NOTE: In case of state_change_xml,
  // this assumes that all "removes" happen before any inserts which are
  // always appends.
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
      if (state_change_xml &&
        currentElement->GetScalarAttribute("remove", &remove) &&
        remove != 0)
        {
        this->Internals->RemoveCommand(cmd);
        }
      else
        {
        this->Internals->CommandQueue.push_back(cmd);
        }
      }
    }
  this->Modified();
  return 1;
}

//----------------------------------------------------------------------------
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
