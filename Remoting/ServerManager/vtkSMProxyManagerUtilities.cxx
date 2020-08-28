/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyManagerUtilities.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyManagerUtilities.h"

#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSessionProxyManager.h"

#include <functional>

vtkStandardNewMacro(vtkSMProxyManagerUtilities);
vtkCxxSetObjectMacro(vtkSMProxyManagerUtilities, ProxyManager, vtkSMSessionProxyManager);
//----------------------------------------------------------------------------
vtkSMProxyManagerUtilities::vtkSMProxyManagerUtilities()
  : ProxyManager(nullptr)
{
}

//----------------------------------------------------------------------------
vtkSMProxyManagerUtilities::~vtkSMProxyManagerUtilities()
{
  this->SetProxyManager(nullptr);
}

//----------------------------------------------------------------------------
std::set<vtkSMProxy*> vtkSMProxyManagerUtilities::GetProxiesWithAnnotations(
  const std::map<std::string, std::string>& annotations, bool match_all)
{
  std::set<vtkSMProxy*> proxies;
  if (!this->ProxyManager)
  {
    vtkErrorMacro("No ProxyManager set.");
    return proxies;
  }

  vtkNew<vtkSMProxyIterator> iter;
  iter->SetSessionProxyManager(this->ProxyManager);
  iter->SkipPrototypesOn();
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
  {
    auto proxy = iter->GetProxy();
    if (proxy == nullptr || proxies.find(proxy) != proxies.end())
    {
      continue;
    }

    bool acceptable = false;
    for (const auto& pair : annotations)
    {
      if (proxy->HasAnnotation(pair.first.c_str()) &&
        proxy->GetAnnotation(pair.first.c_str()) == pair.second)
      {
        acceptable = true;
        if (!match_all)
        {
          break;
        }
      }
      else if (match_all)
      {
        acceptable = false;
        break;
      }
    }
    if (acceptable)
    {
      proxies.insert(proxy);
    }
  }
  return proxies;
}

//----------------------------------------------------------------------------
std::set<vtkSMProxy*> vtkSMProxyManagerUtilities::CollectHelpersAndRelatedProxies(
  const std::set<vtkSMProxy*>& proxies)
{
  std::set<vtkSMProxy*> all_proxies;

  vtkNew<vtkSMParaViewPipelineController> controller;
  std::function<void(vtkSMProxy*)> insert;
  insert = [&](vtkSMProxy* proxy) {
    if (proxy == nullptr || all_proxies.insert(proxy).second == false)
    {
      // already added.
      return;
    }

    auto helpers = controller->GetHelperProxyGroupName(proxy);
    if (!helpers.empty())
    {
      vtkNew<vtkSMProxyIterator> iter;
      iter->SetSessionProxyManager(this->ProxyManager);
      for (iter->Begin(helpers.c_str()); !iter->IsAtEnd(); iter->Next())
      {
        insert(iter->GetProxy());
      }
    }

    // add proxies on proxy-properties.
    auto piter = proxy->NewPropertyIterator();
    for (piter->Begin(); !piter->IsAtEnd(); piter->Next())
    {
      auto pp = vtkSMProxyProperty::SafeDownCast(piter->GetProperty());
      for (unsigned int cc = 0; pp && (pp->GetNumberOfProxies() > cc); cc++)
      {
        insert(pp->GetProxy(cc));
      }
    }
    piter->Delete();
  };

  for (auto& proxy : proxies)
  {
    insert(proxy);
  }

  return all_proxies;
}

//----------------------------------------------------------------------------
void vtkSMProxyManagerUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ProxyManager: " << this->ProxyManager << endl;
}
