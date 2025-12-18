// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMVRInteractorStyleProxy.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqProxy.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"
#include "pqView.h"

#include "vtkCamera.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkStringList.h"
#include "vtkStringScanner.h"
#include "vtkVRQueue.h"

#include <algorithm>
#include <sstream>
#include <string>

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMVRInteractorStyleProxy);
vtkCxxSetObjectMacro(vtkSMVRInteractorStyleProxy, ControlledProxy, vtkSMProxy);

// ----------------------------------------------------------------------------
vtkSMVRInteractorStyleProxy::vtkSMVRInteractorStyleProxy()
  : vtkSMProxy()
  , ControlledProxy(nullptr)
  , ControlledPropertyName(nullptr)
  , IsInternal(false)
{
}

// ----------------------------------------------------------------------------
vtkSMVRInteractorStyleProxy::~vtkSMVRInteractorStyleProxy()
{
  this->SetControlledProxy(nullptr);
  this->SetControlledPropertyName(nullptr);
}

// ----------------------------------------------------------------------------
void vtkSMVRInteractorStyleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ControlledPropertyName: " << this->ControlledPropertyName << endl;
  if (this->ControlledProxy)
  {
    os << indent << "ControlledProxy:" << endl;
    this->ControlledProxy->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "ControlledProxy: (None)" << endl;
  }

  vtkIndent nextIndent = indent.GetNextIndent();
  os << indent << "Valuators:" << endl;
  for (StringMap::const_iterator iter = this->Valuators.begin(), itEnd = this->Valuators.end();
       iter != itEnd; ++iter)
  {
    os << nextIndent << iter->first.c_str() << ": " << iter->second.c_str() << endl;
  }
  os << indent << "Buttons:" << endl;
  for (StringMap::const_iterator iter = this->Buttons.begin(), itEnd = this->Buttons.end();
       iter != itEnd; ++iter)
  {
    os << nextIndent << iter->first.c_str() << ": " << iter->second.c_str() << endl;
  }
  os << indent << "Trackers:" << endl;
  for (StringMap::const_iterator iter = this->Trackers.begin(), itEnd = this->Trackers.end();
       iter != itEnd; ++iter)
  {
    os << nextIndent << iter->first.c_str() << ": " << iter->second.c_str() << endl;
  }
}
// ----------------------------------------------------------------------------
// Read from a State file (PVSM file)
bool vtkSMVRInteractorStyleProxy::Configure(vtkPVXMLElement* child, vtkSMProxyLocator* locator)
{
  if (!child->GetName() || strcmp(child->GetName(), "Style") != 0 ||
    strcmp(this->GetClassName(), child->GetAttributeOrEmpty("class")) != 0)
  {
    return false;
  }

  // We'll need either the proxyName, or a proxy id and locator in order to set
  //   the proxy
  int id = -1;
  bool hasProxyName = static_cast<bool>(child->GetAttribute("proxyName"));
  bool hasProxyId =
    static_cast<bool>(locator) && static_cast<bool>(child->GetScalarAttribute("proxy", &id));
  if (!hasProxyId && !hasProxyName)
  {
    return false;
  }

  if (child->GetAttribute("property") == nullptr)
  {
    return false;
  }

  bool result = true;
  for (unsigned int neCount = 0; neCount < child->GetNumberOfNestedElements(); neCount++)
  {
    vtkPVXMLElement* element = child->GetNestedElement(neCount);
    if (element && element->GetName())
    {
      if (strcmp(element->GetName(), "Valuator") == 0)
      {
        const char* role = element->GetAttributeOrDefault("role", nullptr);
        const char* name = element->GetAttributeOrDefault("name", nullptr);
        if (role && name && *role && *name)
        {
          if (!this->SetValuatorName(role, name))
          {
            vtkWarningMacro(<< "Invalid valuator role: " << role);
            result = false;
          }
        }
      }
      else if (strcmp(element->GetName(), "Button") == 0)
      {
        const char* role = element->GetAttributeOrDefault("role", nullptr);
        const char* name = element->GetAttributeOrDefault("name", nullptr);
        if (role && name && *role && *name)
        {
          if (!this->SetButtonName(role, name))
          {
            vtkWarningMacro(<< "Invalid button role: " << role);
            result = false;
          }
        }
      }
      else if (strcmp(element->GetName(), "Tracker") == 0)
      {
        const char* role = element->GetAttributeOrDefault("role", nullptr);
        const char* name = element->GetAttributeOrDefault("name", nullptr);
        if (role && name && *role && *name)
        {
          if (!this->SetTrackerName(role, name))
          {
            vtkWarningMacro(<< "Invalid tracker role: " << role);
            result = false;
          }
        }
      }
    }
  }

  // Verify that all defined roles have named buttons:
  for (StringMap::const_iterator iter = this->Valuators.begin(), itEnd = this->Valuators.end();
       iter != itEnd; ++iter)
  {
    if (iter->second.empty())
    {
      vtkWarningMacro(<< "Valuator role not defined: " << iter->first.c_str());
      result = false;
    }
  }
  for (StringMap::const_iterator iter = this->Buttons.begin(), itEnd = this->Buttons.end();
       iter != itEnd; ++iter)
  {
    if (iter->second.empty())
    {
      vtkWarningMacro(<< "Button role not defined: " << iter->first.c_str());
      result = false;
    }
  }
  for (StringMap::const_iterator iter = this->Trackers.begin(), itEnd = this->Trackers.end();
       iter != itEnd; ++iter)
  {
    if (iter->second.empty())
    {
      vtkWarningMacro(<< "Tracker role not defined: " << iter->first.c_str());
      result = false;
    }
  }

  // Locate the proxy -- prefer ID lookup.
  this->SetControlledProxy(nullptr);
  if (hasProxyId)
  {
    this->SetControlledProxy(locator->LocateProxy(id));
  }
  else if (hasProxyName)
  {
    pqApplicationCore* core = pqApplicationCore::instance();
    pqServerManagerModel* model = core->getServerManagerModel();
    pqProxy* proxy = model->findItem<pqProxy*>(child->GetAttribute("proxyName"));
    if (proxy)
    {
      this->SetControlledProxy(proxy->getProxy());
    }
  }

  // Set property
  this->SetControlledPropertyName(child->GetAttribute("property"));

  // Verify settings
  if (this->ControlledProxy == nullptr || this->ControlledPropertyName == nullptr ||
    this->ControlledPropertyName[0] == '\0')
  {
    vtkWarningMacro(<< "Invalid Controlled Proxy or PropertyName. "
                    << "this->ControlledProxy: " << this->ControlledProxy << " "
                    << "Proxy id, locator: " << id << ", " << locator << " "
                    << "Proxy name: " << child->GetAttribute("proxyName") << " "
                    << "PropertyName: " << this->ControlledPropertyName);
    result = false;
  }

  return result;
}

// ----------------------------------------------------------------------------
// Save to a State file (PVSM file)
vtkPVXMLElement* vtkSMVRInteractorStyleProxy::SaveConfiguration()
{
  vtkPVXMLElement* child = vtkPVXMLElement::New();
  child->SetName("Style");
  child->AddAttribute("class", this->GetClassName());

  // Look up proxy name -- we'll store both name and id.
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* model = core->getServerManagerModel();
  pqProxy* pqControlledProxy = model->findItem<pqProxy*>(this->ControlledProxy);
  QString name = pqControlledProxy ? pqControlledProxy->getSMName() : QString("(unknown)");
  child->AddAttribute("proxyName", name.toUtf8().data());

  child->AddAttribute(
    "proxy", this->ControlledProxy ? this->ControlledProxy->GetGlobalIDAsString() : "0");
  if (this->ControlledPropertyName != nullptr && this->ControlledPropertyName[0] != '\0')
  {
    child->AddAttribute("property", this->ControlledPropertyName);
  }

  for (StringMap::const_iterator iter = this->Valuators.begin(), itEnd = this->Valuators.end();
       iter != itEnd; ++iter)
  {
    vtkPVXMLElement* valuator = vtkPVXMLElement::New();
    valuator->SetName("Valuator");
    valuator->AddAttribute("role", iter->first.c_str());
    valuator->AddAttribute("name", iter->second.c_str());
    child->AddNestedElement(valuator);
    valuator->FastDelete();
  }
  for (StringMap::const_iterator iter = this->Buttons.begin(), itEnd = this->Buttons.end();
       iter != itEnd; ++iter)
  {
    vtkPVXMLElement* button = vtkPVXMLElement::New();
    button->SetName("Button");
    button->AddAttribute("role", iter->first.c_str());
    button->AddAttribute("name", iter->second.c_str());
    child->AddNestedElement(button);
    button->FastDelete();
  }
  for (StringMap::const_iterator iter = this->Trackers.begin(), itEnd = this->Trackers.end();
       iter != itEnd; ++iter)
  {
    vtkPVXMLElement* tracker = vtkPVXMLElement::New();
    tracker->SetName("Tracker");
    tracker->AddAttribute("role", iter->first.c_str());
    tracker->AddAttribute("name", iter->second.c_str());
    child->AddNestedElement(tracker);
    tracker->FastDelete();
  }

  return child;
}

// -----------------------------------------------------------------------------
bool vtkSMVRInteractorStyleProxy::Update()
{
  return true;
}

// ----------------------------------------------------------------------------
void vtkSMVRInteractorStyleProxy::HandleButton(const vtkVREvent& vtkNotUsed(event)) {}

// ----------------------------------------------------------------------------
void vtkSMVRInteractorStyleProxy::HandleValuator(const vtkVREvent& vtkNotUsed(event)) {}

// ----------------------------------------------------------------------------
void vtkSMVRInteractorStyleProxy::HandleTracker(const vtkVREvent& vtkNotUsed(event)) {}

// ----------------------------------------------------------------------------
std::vector<std::string> vtkSMVRInteractorStyleProxy::Tokenize(std::string input)
{
  std::replace(input.begin(), input.end(), '.', ' ');
  std::istringstream stm(input);
  std::vector<std::string> token;
  for (;;)
  {
    std::string word;
    if (!(stm >> word))
      break;
    token.push_back(word);
  }
  return token;
}

// ----------------------------------------------------------------------------
void vtkSMVRInteractorStyleProxy::AddValuatorRole(const std::string& role)
{
  this->Valuators.insert(StringMap::value_type(role, std::string()));
}

// ----------------------------------------------------------------------------
void vtkSMVRInteractorStyleProxy::AddButtonRole(const std::string& role)
{
  this->Buttons.insert(StringMap::value_type(role, std::string()));
}

// ----------------------------------------------------------------------------
void vtkSMVRInteractorStyleProxy::AddTrackerRole(const std::string& role)
{
  this->Trackers.insert(StringMap::value_type(role, std::string()));
}

// ----------------------------------------------------------------------------
void vtkSMVRInteractorStyleProxy::ClearAllRoles()
{
  this->Valuators.clear();
  this->Buttons.clear();
  this->Trackers.clear();
}

// ----------------------------------------------------------------------------
void vtkSMVRInteractorStyleProxy::MapKeysToStringList(
  const StringMap& source, vtkStringList* target)
{
  target->RemoveAllItems();
  for (StringMap::const_iterator iter = source.begin(), itEnd = source.end(); iter != itEnd; ++iter)
  {
    target->AddString(iter->first.c_str());
  }
}

// ----------------------------------------------------------------------------
bool vtkSMVRInteractorStyleProxy::SetValueInMap(
  StringMap& map_, const std::string& key, const std::string& value)
{
  StringMap::iterator iter = map_.find(key);
  if (iter != map_.end())
  {
    iter->second = value;
    return true;
  }
  return false;
}

// ----------------------------------------------------------------------------
std::string vtkSMVRInteractorStyleProxy::GetValueInMap(
  const StringMap& map_, const std::string& key)
{
  StringMap::const_iterator iter = map_.find(key);
  return iter != map_.end() ? iter->second : std::string();
}

// ----------------------------------------------------------------------------
std::string vtkSMVRInteractorStyleProxy::GetKeyInMap(
  const StringMap& map_, const std::string& value)
{
  for (StringMap::const_iterator iter = map_.begin(), itEnd = map_.end(); iter != itEnd; ++iter)
  {
    if (iter->second == value)
    {
      return iter->first;
    }
  }
  return std::string();
}

// ----------------------------------------------------------------------------
unsigned int vtkSMVRInteractorStyleProxy::GetChannelIndexForValuatorRole(const std::string& role)
{
  StringMap::const_iterator iter = this->Valuators.find(role);

  if (iter == this->Valuators.end())
  {
    vtkErrorMacro(<< "Unknown valuator role: " << role);
    return 0;
  }

  std::vector<std::string> tokens = vtkSMVRInteractorStyleProxy::Tokenize(iter->second);
  auto& table = *(this->valuatorLookupTable);
  auto connMap = table[tokens[0]];
  StringMap::const_iterator eventIter = connMap.find(tokens[1]);

  if (eventIter == connMap.end())
  {
    vtkErrorMacro(<< "No known valuators for connection: " << tokens[0]);
    return 0;
  }

  std::vector<std::string> eventTokens = vtkSMVRInteractorStyleProxy::Tokenize(eventIter->second);

  unsigned int channelIndex;
  VTK_FROM_CHARS_IF_ERROR_RETURN(eventTokens[1], channelIndex, 0);
  return channelIndex;
}

// ----------------------------------------------------------------------------
bool vtkSMVRInteractorStyleProxy::HandleEvent(const vtkVREvent& event)
{
  switch (event.eventType)
  {
    case BUTTON_EVENT:
      this->HandleButton(event);
      break;
    case VALUATOR_EVENT:
      this->HandleValuator(event);
      break;
    case TRACKER_EVENT:
      this->HandleTracker(event);
      break;
  }
  return false;
}

// ----------------------------------------------------------------------------
void vtkSMVRInteractorStyleProxy::GetValuatorRoles(vtkStringList* roles)
{
  this->MapKeysToStringList(this->Valuators, roles);
}

// ----------------------------------------------------------------------------
void vtkSMVRInteractorStyleProxy::GetButtonRoles(vtkStringList* roles)
{
  this->MapKeysToStringList(this->Buttons, roles);
}

// ----------------------------------------------------------------------------
void vtkSMVRInteractorStyleProxy::GetTrackerRoles(vtkStringList* roles)
{
  this->MapKeysToStringList(this->Trackers, roles);
}

// ----------------------------------------------------------------------------
int vtkSMVRInteractorStyleProxy::GetNumberOfValuatorRoles()
{
  return static_cast<int>(this->Valuators.size());
}

// ----------------------------------------------------------------------------
int vtkSMVRInteractorStyleProxy::GetNumberOfButtonRoles()
{
  return static_cast<int>(this->Buttons.size());
}

// ----------------------------------------------------------------------------
int vtkSMVRInteractorStyleProxy::GetNumberOfTrackerRoles()
{
  return static_cast<int>(this->Trackers.size());
}

// ----------------------------------------------------------------------------
std::string vtkSMVRInteractorStyleProxy::GetButtonRole(const std::string& name)
{
  return this->GetKeyInMap(this->Buttons, name);
}

// ----------------------------------------------------------------------------
std::string vtkSMVRInteractorStyleProxy::GetTrackerRole(const std::string& name)
{
  return this->GetKeyInMap(this->Trackers, name);
}

// ----------------------------------------------------------------------------
bool vtkSMVRInteractorStyleProxy::SetValuatorName(const std::string& role, const std::string& name)
{
  return this->SetValueInMap(this->Valuators, role, name);
}

// ----------------------------------------------------------------------------
std::string vtkSMVRInteractorStyleProxy::GetValuatorName(const std::string& role)
{
  return this->GetValueInMap(this->Valuators, role);
}

// ----------------------------------------------------------------------------
bool vtkSMVRInteractorStyleProxy::SetButtonName(const std::string& role, const std::string& name)
{
  return this->SetValueInMap(this->Buttons, role, name);
}

// ----------------------------------------------------------------------------
std::string vtkSMVRInteractorStyleProxy::GetButtonName(const std::string& role)
{
  return this->GetValueInMap(this->Buttons, role);
}

// ----------------------------------------------------------------------------
bool vtkSMVRInteractorStyleProxy::SetTrackerName(const std::string& role, const std::string& name)
{
  return this->SetValueInMap(this->Trackers, role, name);
}

// ----------------------------------------------------------------------------
std::string vtkSMVRInteractorStyleProxy::GetTrackerName(const std::string& role)
{
  return this->GetValueInMap(this->Trackers, role);
}

// ----------------------------------------------------------------------------
vtkSMRenderViewProxy* vtkSMVRInteractorStyleProxy::GetActiveViewProxy()
{
  pqActiveObjects& activeObjs = pqActiveObjects::instance();

  if (pqView* pqview = activeObjs.activeView())
  {
    if (pqRenderView* rview = qobject_cast<pqRenderView*>(pqview))
    {
      return vtkSMRenderViewProxy::SafeDownCast(rview->getProxy());
    }
  }

  return nullptr;
}

// ----------------------------------------------------------------------------
vtkCamera* vtkSMVRInteractorStyleProxy::GetActiveCamera()
{
  vtkSMRenderViewProxy* rvProxy = vtkSMVRInteractorStyleProxy::GetActiveViewProxy();

  if (rvProxy)
  {
    return rvProxy->GetActiveCamera();
  }

  return nullptr;
}

// ----------------------------------------------------------------------------
vtkMatrix4x4* vtkSMVRInteractorStyleProxy::GetNavigationMatrix(vtkSMRenderViewProxy* proxy)
{
  vtkSMRenderViewProxy* viewProxy = proxy;
  if (viewProxy == nullptr)
  {
    viewProxy = vtkSMVRInteractorStyleProxy::GetActiveViewProxy();
  }

  if (viewProxy == nullptr)
  {
    vtkWarningMacro("Getting the navigation matrix requires a render view proxy");
    return nullptr;
  }

  vtkSMPropertyHelper(viewProxy, "ModelTransformMatrix").Get(*this->NavigationMatrix->Element, 16);

  return this->NavigationMatrix.GetPointer();
}

// ----------------------------------------------------------------------------
void vtkSMVRInteractorStyleProxy::SetNavigationMatrix(
  vtkMatrix4x4* matrix, vtkSMRenderViewProxy* proxy)
{
  vtkSMRenderViewProxy* viewProxy = proxy;
  if (viewProxy == nullptr)
  {
    viewProxy = vtkSMVRInteractorStyleProxy::GetActiveViewProxy();
  }

  if (viewProxy == nullptr)
  {
    vtkWarningWithObjectMacro(
      nullptr, "Setting the navigation matrix requires an active render view proxy");
    return;
  }

  vtkSMPropertyHelper(viewProxy, "ModelTransformMatrix").Set(*matrix->Element, 16);

  vtkNew<vtkMatrix4x4> physicalToWorld;
  physicalToWorld->DeepCopy(matrix);
  physicalToWorld->Invert();

  vtkSMPropertyHelper(viewProxy, "PhysicalToWorldMatrix").Set(physicalToWorld->Element[0], 16);

  viewProxy->InvokeEvent(INTERACTOR_STYLE_NAVIGATION, physicalToWorld);
}

// ----------------------------------------------------------------------------
std::vector<double> vtkSMVRInteractorStyleProxy::GetNavigationScale(vtkSMRenderViewProxy* proxy)
{
  vtkSMRenderViewProxy* viewProxy = proxy;
  if (viewProxy == nullptr)
  {
    viewProxy = vtkSMVRInteractorStyleProxy::GetActiveViewProxy();
  }

  std::vector<double> result;
  result.resize(3);

  if (viewProxy == nullptr)
  {
    vtkErrorWithObjectMacro(
      nullptr, "Getting the navigation matrix scale requires an active render view proxy");
    return result;
  }

  vtkNew<vtkMatrix4x4> tempMatrix;
  vtkSMPropertyHelper(viewProxy, "ModelTransformMatrix").Get(*tempMatrix->Element, 16);

  vtkVector3d columns[3];

  for (int dim = 0; dim < 3; ++dim)
  {
    columns[dim].Set(tempMatrix->GetElement(0, dim), tempMatrix->GetElement(1, dim),
      tempMatrix->GetElement(2, dim));
    result[dim] = columns[dim].Norm();
  }

  return result;
}

// ----------------------------------------------------------------------------
void vtkSMVRInteractorStyleProxy::SetNavigationScale(
  const std::vector<double>& scale, vtkSMRenderViewProxy* proxy)
{
  vtkSMRenderViewProxy* viewProxy = proxy;
  if (viewProxy == nullptr)
  {
    viewProxy = vtkSMVRInteractorStyleProxy::GetActiveViewProxy();
  }

  if (viewProxy == nullptr)
  {
    vtkWarningWithObjectMacro(
      nullptr, "Setting the navigation matrix scale requires an active render view proxy");
    return;
  }

  vtkNew<vtkMatrix4x4> tempMatrix;
  vtkSMPropertyHelper(viewProxy, "ModelTransformMatrix").Get(*tempMatrix->Element, 16);

  vtkVector3d columns[3];

  for (int dim = 0; dim < 3; ++dim)
  {
    // Initialize vectors from the rotation vectors of the matrix
    columns[dim].Set(tempMatrix->GetElement(0, dim), tempMatrix->GetElement(1, dim),
      tempMatrix->GetElement(2, dim));

    // Make them unit length
    columns[dim].Normalize();

    // Rescale the rotation vectors by the provided scale values
    columns[dim].Set(columns[dim].GetX() * scale[dim], columns[dim].GetY() * scale[dim],
      columns[dim].GetZ() * scale[dim]);

    // Update the temp matrix
    tempMatrix->SetElement(0, dim, columns[dim].GetX());
    tempMatrix->SetElement(1, dim, columns[dim].GetY());
    tempMatrix->SetElement(2, dim, columns[dim].GetZ());
  }

  vtkSMVRInteractorStyleProxy::SetNavigationMatrix(tempMatrix, viewProxy);
}

// ----------------------------------------------------------------------------
void vtkSMVRInteractorStyleProxy::UpdateMatrixProperty(
  vtkSMProxy* proxy, const char* propertyName, vtkMatrix4x4* matrix)
{
  vtkSMRenderViewProxy* rvProxy = vtkSMRenderViewProxy::SafeDownCast(proxy);

  if (rvProxy != nullptr && strcmp(propertyName, "ModelTransformMatrix") == 0)
  {
    this->SetNavigationMatrix(matrix, rvProxy);
  }
  else
  {
    vtkSMPropertyHelper(proxy, propertyName).Set(*matrix->Element, 16);
  }
}

//----------------------------------------------------------------------------
void vtkSMVRInteractorStyleProxy::SetValuatorLookupTable(std::shared_ptr<StringMapMap> table)
{
  this->valuatorLookupTable = table;
}
