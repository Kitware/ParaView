// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkVRInteractorStyleFactory.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkObjectFactory.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPluginManager.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMVRInteractorStyleProxy.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVRInteractorStyleFactory);
vtkVRInteractorStyleFactory* vtkVRInteractorStyleFactory::Instance = nullptr;

//-----------------------------------------------------------------------------
vtkVRInteractorStyleFactory::vtkVRInteractorStyleFactory()
  : Initialized(false)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMPluginManager* pluginMgr = pxm->GetPluginManager();
  pluginMgr->AddObserver(
    vtkSMPluginManager::PluginLoadedEvent, this, &vtkVRInteractorStyleFactory::Initialize);
}

//-----------------------------------------------------------------------------
vtkVRInteractorStyleFactory::~vtkVRInteractorStyleFactory() = default;

//-----------------------------------------------------------------------------
void vtkVRInteractorStyleFactory::Initialize()
{
  if (!this->Initialized)
  {
    // Set up the settings proxies
    const char* groupName = "cave_interaction";
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    vtkSMSessionProxyManager* spxm = pxm->GetActiveSessionProxyManager();
    vtkSMProxyDefinitionManager* pdm = spxm->GetProxyDefinitionManager();
    vtkSmartPointer<vtkPVProxyDefinitionIterator> iter;
    iter.TakeReference(pdm->NewSingleGroupIterator(groupName));

    std::string prefix = "vtkSM";

    for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkPVXMLElement* proxyElement = pdm->GetProxyDefinition(groupName, iter->GetProxyName());
      std::string proxyClass = prefix + proxyElement->GetName();
      this->InteractorStyleClassNames.push_back(proxyClass);
      this->InteractorStyleDescriptions.push_back(iter->GetProxyName());
      this->Initialized = true;
    }

    this->InvokeEvent(vtkVRInteractorStyleFactory::INTERACTOR_STYLES_UPDATED);
  }
}

//-----------------------------------------------------------------------------
void vtkVRInteractorStyleFactory::SetInstance(vtkVRInteractorStyleFactory* ins)
{
  if (vtkVRInteractorStyleFactory::Instance)
  {
    vtkVRInteractorStyleFactory::Instance->UnRegister(nullptr);
  }

  if (ins)
  {
    ins->Register(nullptr);
  }

  vtkVRInteractorStyleFactory::Instance = ins;
}

//-----------------------------------------------------------------------------
vtkVRInteractorStyleFactory* vtkVRInteractorStyleFactory::GetInstance()
{
  return vtkVRInteractorStyleFactory::Instance;
}

//-----------------------------------------------------------------------------
std::vector<std::string> vtkVRInteractorStyleFactory::GetInteractorStyleClassNames()
{
  return this->InteractorStyleClassNames;
}

//-----------------------------------------------------------------------------
std::vector<std::string> vtkVRInteractorStyleFactory::GetInteractorStyleDescriptions()
{
  return this->InteractorStyleDescriptions;
}

//-----------------------------------------------------------------------------
std::string vtkVRInteractorStyleFactory::GetDescriptionFromClassName(const std::string& className)
{
  for (size_t count = 0; count < this->InteractorStyleClassNames.size(); ++count)
  {
    if (this->InteractorStyleClassNames[count] == className)
    {
      return this->InteractorStyleDescriptions[count];
    }
  }
  return std::string("Unknown");
}

//-----------------------------------------------------------------------------
vtkSMVRInteractorStyleProxy* vtkVRInteractorStyleFactory::NewInteractorStyleFromClassName(
  const std::string& name)
{
  for (size_t count = 0; count < this->InteractorStyleClassNames.size(); ++count)
  {
    if (this->InteractorStyleClassNames[count] == name)
    {
      return this->NewInteractorStyleFromDescription(this->InteractorStyleDescriptions[count]);
    }
  }

  vtkWarningMacro(<< "Failed to create new proxy, unrecognized class: " << name);
  return nullptr;
}

//-----------------------------------------------------------------------------
vtkSMVRInteractorStyleProxy* vtkVRInteractorStyleFactory::NewInteractorStyleFromDescription(
  const std::string& desc)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMSessionProxyManager* spxm = pxm->GetActiveSessionProxyManager();
  vtkSMProxy* genericProxy = pxm->NewProxy("cave_interaction", desc.c_str());

  if (genericProxy != nullptr)
  {
    return vtkSMVRInteractorStyleProxy::SafeDownCast(genericProxy);
  }

  vtkWarningMacro(<< "Failed to create new proxy: " << desc);
  return nullptr;
}

//-----------------------------------------------------------------------------
void vtkVRInteractorStyleFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->InteractorStyleClassNames.size() != this->InteractorStyleDescriptions.size())
  {
    os << indent << "Internal state invalid!\n";
    return;
  }
  os << indent << "Known interactor styles:" << endl;
  vtkIndent iindent = indent.GetNextIndent();
  for (size_t count = 0; count < this->InteractorStyleClassNames.size(); ++count)
  {
    os << iindent << "\"" << this->InteractorStyleDescriptions[count] << "\" ("
       << this->InteractorStyleClassNames[count] << ")\n";
  }
}
