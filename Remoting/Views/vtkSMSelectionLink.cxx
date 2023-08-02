// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMSelectionLink.h"

#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMMessage.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSelectionNode.h"
#include "vtkWeakPointer.h"

#include <list>

vtkStandardNewMacro(vtkSMSelectionLink);
//-----------------------------------------------------------------------------
class vtkSMSelectionLinkObserver : public vtkCommand
{
public:
  static vtkSMSelectionLinkObserver* New() { return new vtkSMSelectionLinkObserver; }
  vtkSMSelectionLinkObserver()
  {
    this->Link = nullptr;
    this->InProgress = false;
  }
  ~vtkSMSelectionLinkObserver() override { this->Link = nullptr; }

  void Execute(vtkObject* c, unsigned long event, void* port) override
  {
    if (this->InProgress)
    {
      return;
    }

    if (this->Link && !this->Link->GetEnabled())
    {
      return;
    }

    this->InProgress = true;
    vtkSMSourceProxy* caller = vtkSMSourceProxy::SafeDownCast(c);
    if (this->Link && caller)
    {
      if (event == vtkCommand::SelectionChangedEvent)
      {
        this->Link->SelectionModified(caller, *((unsigned int*)port));
      }
    }
    this->InProgress = false;
  }

  vtkSMSelectionLink* Link;
  bool InProgress;
};

//-----------------------------------------------------------------------------
class vtkSMSelectionLinkInternals
{
public:
  struct LinkedSelection
  {
  public:
    LinkedSelection(vtkSMSourceProxy* proxy, int updateDir)
      : Proxy(proxy)
      , UpdateDirection(updateDir)
    {
    }

    ~LinkedSelection() = default;

    vtkWeakPointer<vtkSMSourceProxy> Proxy;
    int UpdateDirection;
  };

  typedef std::list<LinkedSelection> LinkedSelectionType;
  LinkedSelectionType LinkedSelections;
  vtkSMSelectionLinkObserver* SelectionObserver;
};

//-----------------------------------------------------------------------------
vtkSMSelectionLink::vtkSMSelectionLink()
{
  this->Internals = new vtkSMSelectionLinkInternals;
  this->Internals->SelectionObserver = vtkSMSelectionLinkObserver::New();
  this->Internals->SelectionObserver->Link = this;
  this->ConvertToIndices = true;
}

//-----------------------------------------------------------------------------
vtkSMSelectionLink::~vtkSMSelectionLink()
{
  for (auto& linkedSelection : this->Internals->LinkedSelections)
  {
    if (linkedSelection.Proxy.GetPointer() != nullptr)
    {
      if (linkedSelection.UpdateDirection & INPUT)
      {
        linkedSelection.Proxy->RemoveObserver(this->Internals->SelectionObserver);
      }
      if (linkedSelection.UpdateDirection & OUTPUT)
      {
        for (unsigned int i = 0; i < linkedSelection.Proxy->GetNumberOfAlgorithmOutputPorts(); i++)
        {
          this->Internals->SelectionObserver->InProgress = true;
          linkedSelection.Proxy->CleanSelectionInputs(i);
          this->Internals->SelectionObserver->InProgress = false;
        }
      }
    }
  }
  this->Internals->SelectionObserver->Delete();
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void vtkSMSelectionLink::AddLinkedSelection(vtkSMProxy* proxy, int updateDir)
{
  vtkSMSourceProxy* sourceProxy = vtkSMSourceProxy::SafeDownCast(proxy);
  if (sourceProxy != nullptr)
  {
    int addToList = 1;
    int addObserver = updateDir & INPUT;

    for (auto& linkedSelection : this->Internals->LinkedSelections)
    {
      if (linkedSelection.Proxy.GetPointer() == sourceProxy &&
        linkedSelection.UpdateDirection == updateDir)
      {
        addObserver = 0;
        addToList = 0;
      }
    }

    if (addToList)
    {
      this->Internals->LinkedSelections.push_back(
        vtkSMSelectionLinkInternals::LinkedSelection(sourceProxy, updateDir));
    }

    if (addObserver)
    {
      sourceProxy->AddObserver(
        vtkCommand::SelectionChangedEvent, this->Internals->SelectionObserver);

      for (unsigned int i = 0; i < sourceProxy->GetNumberOfAlgorithmOutputPorts(); i++)
      {
        if (sourceProxy->GetSelectionInput(i) != nullptr)
        {
          this->Internals->SelectionObserver->InProgress = true;
          this->SelectionModified(sourceProxy, i);
          this->Internals->SelectionObserver->InProgress = false;
        }
      }
    }

    this->Modified();

    // Update state and push it to share
    this->UpdateState();
    this->PushStateToSession();
  }
}

//-----------------------------------------------------------------------------
void vtkSMSelectionLink::RemoveAllLinks()
{
  this->Internals->LinkedSelections.clear();
  this->State->ClearExtension(LinkState::link);
  this->Modified();

  // Update state and push it to share
  this->UpdateState();
  this->PushStateToSession();
}

//-----------------------------------------------------------------------------
void vtkSMSelectionLink::RemoveLinkedSelection(vtkSMProxy* proxy)
{
  for (auto iter = this->Internals->LinkedSelections.begin();
       iter != this->Internals->LinkedSelections.end();)
  {
    if (iter->Proxy.Get() == proxy)
    {
      iter = this->Internals->LinkedSelections.erase(iter);
      this->Modified();

      // Update state and push it to share
      this->UpdateState();
      this->PushStateToSession();
    }
    else
    {
      ++iter;
    }
  }
}

//-----------------------------------------------------------------------------
unsigned int vtkSMSelectionLink::GetNumberOfLinkedObjects()
{
  return static_cast<unsigned int>(this->Internals->LinkedSelections.size());
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMSelectionLink::GetLinkedProxy(int index)
{
  auto iter = this->Internals->LinkedSelections.begin();
  for (int i = 0; i < index && iter != this->Internals->LinkedSelections.end(); i++)
  {
    iter++;
  }
  if (iter == this->Internals->LinkedSelections.end())
  {
    return nullptr;
  }
  return iter->Proxy;
}

//-----------------------------------------------------------------------------
int vtkSMSelectionLink::GetLinkedObjectDirection(int index)
{
  auto iter = this->Internals->LinkedSelections.begin();
  for (int i = 0; i < index && iter != this->Internals->LinkedSelections.end(); i++)
  {
    iter++;
  }
  if (iter == this->Internals->LinkedSelections.end())
  {
    return NONE;
  }
  return iter->UpdateDirection;
}

//-----------------------------------------------------------------------------
void vtkSMSelectionLink::SaveXMLState(const char* linkname, vtkPVXMLElement* parent)
{
  vtkNew<vtkPVXMLElement> root;
  root->SetName("SelectionLink");
  root->AddAttribute("name", linkname);
  for (auto& linkedSelection : this->Internals->LinkedSelections)
  {
    vtkNew<vtkPVXMLElement> child;
    child->SetName("Selection");
    child->AddAttribute(
      "id", static_cast<unsigned int>(linkedSelection.Proxy.GetPointer()->GetGlobalID()));
    child->AddAttribute(
      "direction", ((linkedSelection.UpdateDirection & INPUT) ? "input" : "output"));
    root->AddNestedElement(child);
  }
  parent->AddNestedElement(root);
}

//-----------------------------------------------------------------------------
int vtkSMSelectionLink::LoadXMLState(vtkPVXMLElement* linkElement, vtkSMProxyLocator* locator)
{
  unsigned int numElems = linkElement->GetNumberOfNestedElements();
  for (unsigned int cc = 0; cc < numElems; cc++)
  {
    vtkPVXMLElement* child = linkElement->GetNestedElement(cc);
    if (!child->GetName() || strcmp(child->GetName(), "Selection") != 0)
    {
      vtkWarningMacro("Invalid element in link state. Ignoring.");
      continue;
    }
    const char* direction = child->GetAttribute("direction");
    if (!direction)
    {
      vtkErrorMacro("State missing required attribute direction.");
      return 0;
    }
    int idirection;
    if (strcmp(direction, "input") == 0)
    {
      idirection = INPUT;
    }
    else if (strcmp(direction, "output") == 0)
    {
      idirection = OUTPUT;
    }
    else
    {
      vtkErrorMacro("Invalid value for direction: " << direction);
      return 0;
    }
    int id;
    if (!child->GetScalarAttribute("id", &id))
    {
      vtkErrorMacro("State missing required attribute id.");
      return 0;
    }
    vtkSMProxy* proxy = locator->LocateProxy(id);
    if (!proxy)
    {
      vtkErrorMacro("Failed to locate proxy with ID: " << id);
      return 0;
    }

    this->AddLinkedSelection(proxy, idirection);
  }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMSelectionLink::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkSMSelectionLink::LoadState(const vtkSMMessage* msg, vtkSMProxyLocator* locator)
{
  this->Superclass::LoadState(msg, locator);

  // Reset old state
  this->Internals->LinkedSelections.clear();

  // Load Selection Links
  int numberOfLinks = msg->ExtensionSize(LinkState::link);
  for (int i = 0; i < numberOfLinks; i++)
  {
    const LinkState_LinkDescription* link = &msg->GetExtension(LinkState::link, i);
    vtkSMProxy* proxy = locator->LocateProxy(link->proxy());

    if (proxy)
    {
      switch (link->direction())
      {
        case LinkState_LinkDescription::NONE:
          this->AddLinkedSelection(proxy, vtkSMLink::NONE);
          break;
        case LinkState_LinkDescription::INPUT:
          this->AddLinkedSelection(proxy, vtkSMLink::INPUT);
          break;
        case LinkState_LinkDescription::OUTPUT:
          this->AddLinkedSelection(proxy, vtkSMLink::OUTPUT);
          break;
      }
    }
    else
    {
      vtkDebugMacro("Proxy not found with ID: " << link->proxy());
    }
  }
}

//-----------------------------------------------------------------------------
void vtkSMSelectionLink::UpdateState()
{
  if (this->Session == nullptr)
  {
    return;
  }

  this->State->ClearExtension(LinkState::link);

  for (auto& linkedSelection : this->Internals->LinkedSelections)
  {
    if (!linkedSelection.Proxy.GetPointer())
    {
      continue;
    }
    LinkState_LinkDescription* link = this->State->AddExtension(LinkState::link);
    link->set_proxy(linkedSelection.Proxy.GetPointer()->GetGlobalID());
    switch (linkedSelection.UpdateDirection)
    {
      case vtkSMLink::NONE:
        link->set_direction(LinkState_LinkDescription::NONE);
        break;
      case vtkSMLink::INPUT:
        link->set_direction(LinkState_LinkDescription::INPUT);
        break;
      case vtkSMLink::OUTPUT:
        link->set_direction(LinkState_LinkDescription::OUTPUT);
        break;
      default:
        vtkErrorMacro("Invalid Link direction");
        break;
    }
  }
}

//-----------------------------------------------------------------------------
void vtkSMSelectionLink::SelectionModified(vtkSMSourceProxy* caller, unsigned int portIndex)
{
  bool callerFound = false;
  for (auto& linkedSelection : this->Internals->LinkedSelections)
  {
    if (caller == linkedSelection.Proxy.Get())
    {
      if (linkedSelection.UpdateDirection & INPUT)
      {
        callerFound = true;
        break;
      }
    }
  }
  if (callerFound)
  {
    vtkSMSourceProxy* appendSelections = caller->GetSelectionInput(portIndex);
    if (this->ConvertToIndices && appendSelections)
    {
      // Convert selection input to indices based selection
      bool selectionChanged;
      appendSelections =
        vtkSMSourceProxy::SafeDownCast(vtkSMSelectionHelper::ConvertAppendSelections(
          vtkSelectionNode::INDICES, appendSelections, caller, portIndex, selectionChanged));
    }
    if (!appendSelections)
    {
      for (auto& linkedSelection : this->Internals->LinkedSelections)
      {
        if (linkedSelection.UpdateDirection & OUTPUT && linkedSelection.Proxy != caller)
        {
          linkedSelection.Proxy->CleanSelectionInputs(portIndex);
        }
      }
    }
    else
    {
      for (auto& linkedSelection : this->Internals->LinkedSelections)
      {
        if (linkedSelection.UpdateDirection & OUTPUT && linkedSelection.Proxy != caller)
        {
          linkedSelection.Proxy->SetSelectionInput(portIndex, appendSelections, 0);
        }
      }
      // Delete registered proxy
      if (this->ConvertToIndices)
      {
        appendSelections->Delete();
      }
    }
  }
}
