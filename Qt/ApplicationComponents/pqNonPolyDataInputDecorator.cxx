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
#include "pqNonPolyDataInputDecorator.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqPropertyWidget.h"
#include "pqServerManagerModel.h"

#include "vtkCommand.h"
#include "vtkPVDataInformation.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

//-----------------------------------------------------------------------------
pqNonPolyDataInputDecorator::pqNonPolyDataInputDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject),
  ObserverId(0)
{
  vtkSMProxy* proxy = parentObject->proxy();
  vtkSMProperty* prop = proxy? proxy->GetProperty("Input") : NULL;
  if (!prop)
    {
    qDebug("Could not locate property named 'Input'. "
      "pqNonPolyDataInputDecorator will have no effect.");
    return;
    }

  this->ObservedObject = prop;
  this->ObserverId = pqCoreUtilities::connect(
    prop, vtkCommand::UncheckedPropertyModifiedEvent,
    this, SIGNAL(enableStateChanged()));
}

//-----------------------------------------------------------------------------
pqNonPolyDataInputDecorator::~pqNonPolyDataInputDecorator()
{
  if (this->ObservedObject && this->ObserverId)
    {
    this->ObservedObject->RemoveObserver(this->ObserverId);
    }
}

//-----------------------------------------------------------------------------
bool pqNonPolyDataInputDecorator::enableWidget() const
{
  pqPropertyWidget* parentObject = this->parentWidget();
  vtkSMProxy* proxy = parentObject->proxy();
  vtkSMProperty* prop = proxy? proxy->GetProperty("Input") : NULL;
  if (prop)
    {
    pqPipelineSource* source =
      pqApplicationCore::instance()->getServerManagerModel()->
      findItem<pqPipelineSource*>(
      vtkSMUncheckedPropertyHelper(prop).GetAsProxy());

    if (!source)
      {
      return false;
      }
    pqOutputPort* cur_input = NULL;
    QList<pqOutputPort*> ports = source->getOutputPorts();
    cur_input = ports.size() > 0? ports[0] : NULL;

    bool is_input_polydata = false;
    bool is_data_structured = false;
    if (cur_input)
      {
      vtkPVDataInformation* dataInfo = cur_input->getDataInformation();
      return !dataInfo->DataSetTypeIsA("vtkPolyData");
      }
    }

  return this->Superclass::enableWidget();
}
