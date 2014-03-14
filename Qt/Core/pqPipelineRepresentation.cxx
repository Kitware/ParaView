/*=========================================================================

   Program: ParaView
   Module:    pqPipelineRepresentation.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

/// \file pqPipelineRepresentation.cxx
/// \date 4/24/2006

#include "pqPipelineRepresentation.h"


// ParaView Server Manager includes.
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkGeometryRepresentation.h"
#include "vtkMath.h"
#include "vtkProcessModule.h"
#include "vtkProperty.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVTemporalDataInformation.h"
#include "vtkScalarsToColors.h"
#include "vtkSmartPointer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMGlobalPropertiesManager.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTransferFunctionProxy.h"

// Qt includes.
#include <QList>
#include <QPair>
#include <QPointer>
#include <QRegExp>
#include <QtDebug>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqDisplayPolicy.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqRenderView.h"
#include "pqScalarsToColors.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "pqSMAdaptor.h"
#include "pqUndoStack.h"

//-----------------------------------------------------------------------------
class pqPipelineRepresentation::pqInternal
{
public:
  vtkSmartPointer<vtkSMRepresentationProxy> RepresentationProxy;

  pqInternal()
    {
    }

  static vtkPVArrayInformation* getArrayInformation(const pqPipelineRepresentation* repr,
    const char* arrayname, int fieldType)
    {
    if (!arrayname || !arrayname[0] || !repr)
      {
      return NULL;
      }

    vtkPVDataInformation* dataInformation = repr->getInputDataInformation();
    vtkPVArrayInformation* arrayInfo = NULL;
    if (dataInformation)
      {
      arrayInfo = dataInformation->GetAttributeInformation(fieldType)->
        GetArrayInformation(arrayname);
      }
    if (!arrayInfo)
      {
      dataInformation = repr->getRepresentedDataInformation();
      if (dataInformation)
        {
        arrayInfo = dataInformation->GetAttributeInformation(fieldType)->
          GetArrayInformation(arrayname);
        }
      }
    return arrayInfo;
    }

};

//-----------------------------------------------------------------------------
pqPipelineRepresentation::pqPipelineRepresentation(
  const QString& group,
  const QString& name,
  vtkSMProxy* display,
  pqServer* server, QObject* p/*=null*/):
  Superclass(group, name, display, server, p)
{
  this->Internal = new pqPipelineRepresentation::pqInternal();
  this->Internal->RepresentationProxy
    = vtkSMRepresentationProxy::SafeDownCast(display);

  if (!this->Internal->RepresentationProxy)
    {
    qFatal("Display given is not a vtkSMRepresentationProxy.");
    }

  QObject::connect(this, SIGNAL(visibilityChanged(bool)),
    this, SLOT(updateScalarBarVisibility(bool)));

  // Whenever the pipeline gets be updated, it's possible that the scalar ranges
  // change. If that happens, we try to ensure that the lookuptable range is big
  // enough to show the entire data (unless of course, the user locked the
  // lookuptable ranges).
  this->connect(this, SIGNAL(dataUpdated()), SLOT(onDataUpdated()));
  this->UpdateLUTRangesOnDataUpdate = true;
}

//-----------------------------------------------------------------------------
pqPipelineRepresentation::~pqPipelineRepresentation()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
vtkSMRepresentationProxy* pqPipelineRepresentation::getRepresentationProxy() const
{
  return this->Internal->RepresentationProxy;
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::onInputChanged()
{
  if (this->getInput())
    {
    QObject::disconnect(this->getInput(),
      SIGNAL(modifiedStateChanged(pqServerManagerModelItem*)),
      this, SLOT(onInputAccepted()));
    }

  this->Superclass::onInputChanged();

  if (this->getInput())
    {
    /// We need to try to update the LUT ranges only when the user manually
    /// changed the input source object (not necessarily ever pipeline update
    /// which can happen when time changes -- for example). So we listen to this
    /// signal. BUG #10062.
    QObject::connect(this->getInput(),
      SIGNAL(modifiedStateChanged(pqServerManagerModelItem*)),
      this, SLOT(onInputAccepted()));
    }
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::resetLookupTableScalarRange()
{
  vtkSMProxy* proxy = this->getProxy();
  if (vtkSMPVRepresentationProxy::GetUsingScalarColoring(proxy))
    {
    vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRange(proxy);
    }
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::resetLookupTableScalarRangeOverTime()
{
  vtkSMProxy* proxy = this->getProxy();
  if (vtkSMPVRepresentationProxy::GetUsingScalarColoring(proxy))
    {
    vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRangeOverTime(proxy);
    }
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::onInputAccepted()
{
  // BUG #10062
  // This slot gets called when the input to the representation is "accepted".
  // We mark this representation's LUT ranges dirty so that when the pipeline
  // finally updates, we can reset the LUT ranges.
  if (this->getInput()->modifiedState() == pqProxy::MODIFIED)
    {
    this->UpdateLUTRangesOnDataUpdate = true;
    }
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::onDataUpdated()
{
  if (this->UpdateLUTRangesOnDataUpdate ||
    pqScalarsToColors::colorRangeScalingMode() ==
    pqScalarsToColors::GROW_ON_UPDATED)
    {
    // BUG #10062
    // Since this part of the code happens every time the pipeline is updated, we
    // don't need to record it on the undo stack. It will happen automatically
    // each time.
    BEGIN_UNDO_EXCLUDE();
    this->UpdateLUTRangesOnDataUpdate = false;
    this->updateLookupTableScalarRange();
    END_UNDO_EXCLUDE();
    }
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::updateLookupTableScalarRange()
{
  pqScalarsToColors* lut = this->getLookupTable();
  if (!lut || lut->getScalarRangeLock())
    {
    return;
    }

  vtkSMProxy* proxy = this->getProxy();
  if (vtkSMPVRepresentationProxy::GetUsingScalarColoring(proxy))
    {
    // if scale range was initialized, we extend it, other we simply reset it.
    vtkSMPropertyHelper helper(lut->getProxy(), "ScalarRangeInitialized", true);
    bool extend_lut = helper.GetAsInt() == 1;
    vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRange(proxy,
      extend_lut);
    // mark the scalar range as initialized.
    helper.Set(0, 1);
    lut->getProxy()->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::updateScalarBarVisibility(bool visible)
{
  qDebug("FIXME");
}
//-----------------------------------------------------------------------------
const char* pqPipelineRepresentation::UNSTRUCTURED_GRID_OUTLINE_THRESHOLD()
{
  // BUG #13564. I am renaming this key since we want to change the default
  // value for this key. Renaming the key makes it possible to override the
  // value without forcing users to reset their .config files.
  return "/representation/UnstructuredGridOutlineThreshold2";
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::setUnstructuredGridOutlineThreshold(double numcells)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqSettings* settings = core->settings();
  if (settings)
    {
    settings->setValue(
      pqPipelineRepresentation::UNSTRUCTURED_GRID_OUTLINE_THRESHOLD(), 
      QVariant(numcells));
    }
}

//-----------------------------------------------------------------------------
double pqPipelineRepresentation::getUnstructuredGridOutlineThreshold()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqSettings* settings = core->settings();
  if (settings && settings->contains(
      pqPipelineRepresentation::UNSTRUCTURED_GRID_OUTLINE_THRESHOLD()))
    {
    bool ok;
    double numcells = settings->value(
      pqPipelineRepresentation::UNSTRUCTURED_GRID_OUTLINE_THRESHOLD()).toDouble(&ok);
    if (ok)
      {
      return numcells;
      }
    }

  return 250; //  250 million cells.
}
