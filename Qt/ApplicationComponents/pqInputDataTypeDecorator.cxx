/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
    cur_input = ports.size() > 0 ? ports[0] : nullptr;
    int exclude = 0;
    this->xml()->GetScalarAttribute("exclude", &exclude);
    std::string dataname = this->xml()->GetAttribute("name");
    std::vector<std::string> parts = vtksys::SystemTools::SplitString(dataname, ' ');
    if (cur_input && parts.size())
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
