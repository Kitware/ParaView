// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqInputDataTypeDecorator.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqPropertyWidget.h"
#include "pqServerManagerModel.h"

#include "vtkCommand.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <vtksys/SystemTools.hxx>

#include <string>
#include <vector>

//-----------------------------------------------------------------------------
pqInputDataTypeDecorator::pqInputDataTypeDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject)
  , ObserverId(0)
{
  vtkSMProxy* proxy = parentObject->proxy();
  vtkSMProperty* prop = proxy ? proxy->GetProperty("Input") : nullptr;
  if (!prop)
  {
    qDebug("Could not locate property named 'Input'. "
           "pqInputDataTypeDecorator will have no effect.");
    return;
  }

  this->ObservedObject = prop;
  this->ObserverId = pqCoreUtilities::connect(
    prop, vtkCommand::UncheckedPropertyModifiedEvent, this, SIGNAL(enableStateChanged()));
}

//-----------------------------------------------------------------------------
pqInputDataTypeDecorator::~pqInputDataTypeDecorator()
{
  if (this->ObservedObject && this->ObserverId)
  {
    this->ObservedObject->RemoveObserver(this->ObserverId);
  }
}

//-----------------------------------------------------------------------------
bool pqInputDataTypeDecorator::enableWidget() const
{
  const char* enableWidgetActive = this->xml()->GetAttribute("mode");
  // By default, if enable_toggle is not set, we go through
  if (!enableWidgetActive || !strcmp(enableWidgetActive, "enabled_state"))
  {
    if (!this->processState())
    {
      return false;
    }
  }
  return this->Superclass::enableWidget();
}

//-----------------------------------------------------------------------------
bool pqInputDataTypeDecorator::canShowWidget(bool show_advanced) const
{
  const char* showWidgetActive = this->xml()->GetAttribute("mode");
  // By default, if show_toggle is not set, we DO NOT go through
  if (showWidgetActive && !strcmp(showWidgetActive, "visibility"))
  {
    if (!this->processState())
    {
      return false;
    }
  }
  return this->Superclass::canShowWidget(show_advanced);
}

//-----------------------------------------------------------------------------
bool pqInputDataTypeDecorator::processState() const
{
  pqPropertyWidget* parentObject = this->parentWidget();
  vtkSMProxy* proxy = parentObject->proxy();
  vtkSMProperty* prop = proxy ? proxy->GetProperty("Input") : nullptr;
  if (prop)
  {
    pqPipelineSource* source =
      pqApplicationCore::instance()->getServerManagerModel()->findItem<pqPipelineSource*>(
        vtkSMUncheckedPropertyHelper(prop).GetAsProxy());

    if (!source)
    {
      return false;
    }
    pqOutputPort* cur_input = nullptr;
    QList<pqOutputPort*> ports = source->getOutputPorts();
    cur_input = ports.empty() ? nullptr : ports[0];
    int exclude = 0;
    this->xml()->GetScalarAttribute("exclude", &exclude);
    std::string dataname = this->xml()->GetAttribute("name");
    std::vector<std::string> parts = vtksys::SystemTools::SplitString(dataname, ' ');
    if (cur_input && !parts.empty())
    {
      vtkPVDataInformation* dataInfo = cur_input->getDataInformation();
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
