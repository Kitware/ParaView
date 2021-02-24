/*=========================================================================

   Program: ParaView
   Module:  vtkVRInteractorStyle.cxx

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
#include "vtkVRInteractorStyle.h"

#include "pqApplicationCore.h"
#include "pqProxy.h"
#include "pqServerManagerModel.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkStringList.h"
#include "vtkVRQueue.h"

#include <algorithm>
#include <sstream>
#include <string>

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVRInteractorStyle);
vtkCxxSetObjectMacro(vtkVRInteractorStyle, ControlledProxy, vtkSMProxy);

// ----------------------------------------------------------------------------
// Constructor method
vtkVRInteractorStyle::vtkVRInteractorStyle()
  : Superclass()
  , ControlledProxy(nullptr)
  , ControlledPropertyName(nullptr)
{
}

// ----------------------------------------------------------------------------
// Destructor method
vtkVRInteractorStyle::~vtkVRInteractorStyle()
{
  this->SetControlledProxy(nullptr);
  this->SetControlledPropertyName(nullptr);
}

// ----------------------------------------------------------------------------
// PrintSelf() method
void vtkVRInteractorStyle::PrintSelf(ostream& os, vtkIndent indent)
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
  os << indent << "Analogs:" << endl;
  for (StringMap::const_iterator iter = this->Analogs.begin(), itEnd = this->Analogs.end();
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
// Configure() method -- to reread from a State file (PVSM file)
bool vtkVRInteractorStyle::Configure(vtkPVXMLElement* child, vtkSMProxyLocator* locator)
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
      if (strcmp(element->GetName(), "Analog") == 0)
      {
        const char* role = element->GetAttributeOrDefault("role", nullptr);
        const char* name = element->GetAttributeOrDefault("name", nullptr);
        if (role && name && *role && *name)
        {
          if (!this->SetAnalogName(role, name))
          {
            vtkWarningMacro(<< "Invalid analog role: " << role);
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
  for (StringMap::const_iterator iter = this->Analogs.begin(), itEnd = this->Analogs.end();
       iter != itEnd; ++iter)
  {
    if (iter->second.empty())
    {
      vtkWarningMacro(<< "Analog role not defined: " << iter->first.c_str());
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
  this->ControlledProxy = nullptr;
  if (hasProxyId)
  {
    this->ControlledProxy = locator->LocateProxy(id);
  }
  else if (hasProxyName)
  {
    pqApplicationCore* core = pqApplicationCore::instance();
    pqServerManagerModel* model = core->getServerManagerModel();
    pqProxy* proxy = model->findItem<pqProxy*>(child->GetAttribute("proxyName"));
    if (proxy)
    {
      this->ControlledProxy = proxy->getProxy();
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
// SaveConfiguration() method -- store into a State file (PVSM file)
vtkPVXMLElement* vtkVRInteractorStyle::SaveConfiguration() const
{
  vtkPVXMLElement* child = vtkPVXMLElement::New();
  child->SetName("Style");
  child->AddAttribute("class", this->GetClassName());

  // Look up proxy name -- we'll store both name and id.
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* model = core->getServerManagerModel();
  pqProxy* pqControlledProxy = model->findItem<pqProxy*>(this->ControlledProxy);
  QString name = pqControlledProxy ? pqControlledProxy->getSMName() : QString("(unknown)");
  child->AddAttribute("proxyName", qPrintable(name));

  child->AddAttribute(
    "proxy", this->ControlledProxy ? this->ControlledProxy->GetGlobalIDAsString() : "0");
  if (this->ControlledPropertyName != nullptr && this->ControlledPropertyName[0] != '\0')
  {
    child->AddAttribute("property", this->ControlledPropertyName);
  }

  for (StringMap::const_iterator iter = this->Analogs.begin(), itEnd = this->Analogs.end();
       iter != itEnd; ++iter)
  {
    vtkPVXMLElement* analog = vtkPVXMLElement::New();
    analog->SetName("Analog");
    analog->AddAttribute("role", iter->first.c_str());
    analog->AddAttribute("name", iter->second.c_str());
    child->AddNestedElement(analog);
    analog->FastDelete();
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
// Update() method -- empty in the generic class
bool vtkVRInteractorStyle::Update()
{
  return true;
}

// ----------------------------------------------------------------------------
// HandleButton() method -- empty in the generic class
void vtkVRInteractorStyle::HandleButton(const vtkVREvent& vtkNotUsed(event))
{
}

// ----------------------------------------------------------------------------
// HandleAnalog() method -- empty in the generic class
void vtkVRInteractorStyle::HandleAnalog(const vtkVREvent& vtkNotUsed(event))
{
}

// ----------------------------------------------------------------------------
// HandleTracker() method -- empty in the generic class
void vtkVRInteractorStyle::HandleTracker(const vtkVREvent& vtkNotUsed(event))
{
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// The methods below this divider are ones that are NOT over-written by the
// descendent classes.
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

std::vector<std::string> vtkVRInteractorStyle::Tokenize(std::string input)
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
void vtkVRInteractorStyle::AddAnalogRole(const std::string& role)
{
  this->Analogs.insert(StringMap::value_type(role, std::string()));
}

// ----------------------------------------------------------------------------
void vtkVRInteractorStyle::AddButtonRole(const std::string& role)
{
  this->Buttons.insert(StringMap::value_type(role, std::string()));
}

// ----------------------------------------------------------------------------
void vtkVRInteractorStyle::AddTrackerRole(const std::string& role)
{
  this->Trackers.insert(StringMap::value_type(role, std::string()));
}

// ----------------------------------------------------------------------------
void vtkVRInteractorStyle::MapKeysToStringList(const StringMap& source, vtkStringList* target)
{
  target->RemoveAllItems();
  for (StringMap::const_iterator iter = source.begin(), itEnd = source.end(); iter != itEnd; ++iter)
  {
    target->AddString(iter->first.c_str());
  }
}

// ----------------------------------------------------------------------------
bool vtkVRInteractorStyle::SetValueInMap(
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
std::string vtkVRInteractorStyle::GetValueInMap(const StringMap& map_, const std::string& key)
{
  StringMap::const_iterator iter = map_.find(key);
  return iter != map_.end() ? iter->second : std::string();
}

// ----------------------------------------------------------------------------
std::string vtkVRInteractorStyle::GetKeyInMap(const StringMap& map_, const std::string& value)
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
bool vtkVRInteractorStyle::HandleEvent(const vtkVREvent& event)
{
  switch (event.eventType)
  {
    case BUTTON_EVENT:
      this->HandleButton(event);
      break;
    case ANALOG_EVENT:
      this->HandleAnalog(event);
      break;
    case TRACKER_EVENT:
      this->HandleTracker(event);
      break;
  }
  return false;
}

// ----------------------------------------------------------------------------
void vtkVRInteractorStyle::GetAnalogRoles(vtkStringList* roles)
{
  this->MapKeysToStringList(this->Analogs, roles);
}

// ----------------------------------------------------------------------------
void vtkVRInteractorStyle::GetButtonRoles(vtkStringList* roles)
{
  this->MapKeysToStringList(this->Buttons, roles);
}

// ----------------------------------------------------------------------------
void vtkVRInteractorStyle::GetTrackerRoles(vtkStringList* roles)
{
  this->MapKeysToStringList(this->Trackers, roles);
}

// ----------------------------------------------------------------------------
int vtkVRInteractorStyle::GetNumberOfAnalogRoles()
{
  return static_cast<int>(this->Analogs.size());
}

// ----------------------------------------------------------------------------
int vtkVRInteractorStyle::GetNumberOfButtonRoles()
{
  return static_cast<int>(this->Buttons.size());
}

// ----------------------------------------------------------------------------
int vtkVRInteractorStyle::GetNumberOfTrackerRoles()
{
  return static_cast<int>(this->Trackers.size());
}

// ----------------------------------------------------------------------------
std::string vtkVRInteractorStyle::GetAnalogRole(const std::string& name)
{
  return this->GetKeyInMap(this->Analogs, name);
}

// ----------------------------------------------------------------------------
std::string vtkVRInteractorStyle::GetButtonRole(const std::string& name)
{
  return this->GetKeyInMap(this->Buttons, name);
}

// ----------------------------------------------------------------------------
std::string vtkVRInteractorStyle::GetTrackerRole(const std::string& name)
{
  return this->GetKeyInMap(this->Trackers, name);
}

// ----------------------------------------------------------------------------
bool vtkVRInteractorStyle::SetAnalogName(const std::string& role, const std::string& name)
{
  return this->SetValueInMap(this->Analogs, role, name);
}

// ----------------------------------------------------------------------------
std::string vtkVRInteractorStyle::GetAnalogName(const std::string& role)
{
  return this->GetValueInMap(this->Analogs, role);
}

// ----------------------------------------------------------------------------
bool vtkVRInteractorStyle::SetButtonName(const std::string& role, const std::string& name)
{
  return this->SetValueInMap(this->Buttons, role, name);
}

// ----------------------------------------------------------------------------
std::string vtkVRInteractorStyle::GetButtonName(const std::string& role)
{
  return this->GetValueInMap(this->Buttons, role);
}

// ----------------------------------------------------------------------------
bool vtkVRInteractorStyle::SetTrackerName(const std::string& role, const std::string& name)
{
  return this->SetValueInMap(this->Trackers, role, name);
}

// ----------------------------------------------------------------------------
std::string vtkVRInteractorStyle::GetTrackerName(const std::string& role)
{
  return this->GetValueInMap(this->Trackers, role);
}
