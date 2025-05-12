// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkInputDataTypeDecorator.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <vtksys/SystemTools.hxx>

#include <string>
#include <vector>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkInputDataTypeDecorator);

//-----------------------------------------------------------------------------
vtkInputDataTypeDecorator::vtkInputDataTypeDecorator() = default;

//-----------------------------------------------------------------------------
vtkInputDataTypeDecorator::~vtkInputDataTypeDecorator()
{
  if (this->ObservedObject && this->ObserverId)
  {
    this->ObservedObject->RemoveObserver(this->ObserverId);
  }
}

//-----------------------------------------------------------------------------
void vtkInputDataTypeDecorator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ObserverId : " << this->ObserverId << "\n";
  os << indent << "ObservedObject : " << this->ObservedObject << "\n";
}

//-----------------------------------------------------------------------------
void vtkInputDataTypeDecorator::Initialize(vtkPVXMLElement* config, vtkSMProxy* proxy)
{
  this->Superclass::Initialize(config, proxy);
  this->ObserverId = 0;

  vtkSMProperty* prop = proxy ? proxy->GetProperty("Input") : nullptr;
  if (!prop)
  {
    vtkWarningWithObjectMacro(this,
      "Could not locate property named 'Input'. vtkInputDataTypeDecorator will have no effect.");
    return;
  }

  this->ObservedObject = prop;
  this->ObserverId = prop->AddObserver(vtkCommand::UncheckedPropertyModifiedEvent, this,
    &vtkInputDataTypeDecorator::InvokeEnableStateChangedEvent);
}

//-----------------------------------------------------------------------------
bool vtkInputDataTypeDecorator::Enable() const
{
  const char* enableWidgetActive = this->XML()->GetAttribute("mode");
  // By default, if enable_toggle is not set, we go through
  if (!enableWidgetActive || !strcmp(enableWidgetActive, "enabled_state"))
  {
    if (!this->ProcessState())
    {
      return false;
    }
  }
  return this->Superclass::Enable();
}

//-----------------------------------------------------------------------------
bool vtkInputDataTypeDecorator::CanShow(bool show_advanced) const
{
  const char* showWidgetActive = this->XML()->GetAttribute("mode");
  // By default, if show_toggle is not set, we DO NOT go through
  if (showWidgetActive && !strcmp(showWidgetActive, "visibility"))
  {
    if (!this->ProcessState())
    {
      return false;
    }
  }
  return this->Superclass::CanShow(show_advanced);
}

//-----------------------------------------------------------------------------
bool vtkInputDataTypeDecorator::ProcessState() const
{
  vtkSMProxy* proxy = this->Proxy();
  vtkSMProperty* prop = proxy ? proxy->GetProperty("Input") : nullptr;
  if (prop)
  {
    vtkSMSourceProxy* source =
      vtkSMSourceProxy::SafeDownCast(vtkSMUncheckedPropertyHelper(prop).GetAsProxy());
    if (!source)
    {
      return false;
    }

    vtkSMOutputPort* cur_input =
      source->GetNumberOfOutputPorts() == 0 ? nullptr : source->GetOutputPort((unsigned int)(0));
    int exclude = 0;
    this->XML()->GetScalarAttribute("exclude", &exclude);
    std::string dataname = this->XML()->GetAttribute("name");
    std::vector<std::string> parts = vtksys::SystemTools::SplitString(dataname, ' ');
    if (cur_input && !parts.empty())
    {
      vtkPVDataInformation* dataInfo = cur_input->GetDataInformation();
      for (std::size_t i = 0; i < parts.size(); ++i)
      {
        const bool match = (parts[i] == "Structured") ? dataInfo->IsDataStructured()
                                                      : dataInfo->DataSetTypeIsA(parts[i].c_str());
        if (match)
        {
          return !exclude;
        }
      }
      return exclude;
    }
    else
    {
      return false;
    }
  }
  return true;
}
