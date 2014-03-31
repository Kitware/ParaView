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
