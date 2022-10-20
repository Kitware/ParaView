/*=========================================================================

   Program: ParaView
   Module:  vtkVRInteractorStyleFactory.cxx


   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/

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
