/*=========================================================================

   Program: ParaView
   Module:  pqSurfaceRepresentationBehavior.cxx

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
#include "pqSurfaceRepresentationBehavior.h"

#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqPipelineRepresentation.h"
#include "pqRepresentation.h"
#include "pqSMAdaptor.h"
#include "pqScalarsToColors.h"
#include "pqServerManagerModel.h"
#include "pqView.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSourceProxy.h"

#include <QVariant>

//-----------------------------------------------------------------------------
pqSurfaceRepresentationBehavior::pqSurfaceRepresentationBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(viewAdded(pqView*)), this, SLOT(onViewAdded(pqView*)));
}

//-----------------------------------------------------------------------------
void pqSurfaceRepresentationBehavior::onRepresentationAdded(pqRepresentation* rep)
{
  pqPipelineRepresentation* pipelineRep = qobject_cast<pqPipelineRepresentation*>(rep);
  vtkSMRepresentationProxy* smRep = vtkSMRepresentationProxy::SafeDownCast(rep->getProxy());
  if (pipelineRep && smRep)
  {

    // ------------------------------------------------------------------------
    // Let's pick a reptesentation type that we like instead of the default one
    // We will use our own set of priority order.
    // ------------------------------------------------------------------------
    QList<QVariant> list = pqSMAdaptor::getEnumerationPropertyDomain(
      pipelineRep->getProxy()->GetProperty("Representation"));

    // Set a representation type based on a priority
    int priority = 0;
    std::string finalValue;
    foreach (QVariant v, list)
    {
      if (v.toString() == "Surface" && priority < 1)
      {
        finalValue = "Surface";
        priority = 1;
      }

      if (v.toString() == "Slices" && priority < 2)
      {
        finalValue = "Slices";
        priority = 2;
      }
    }

    // Apply the new representation type is any available
    if (!finalValue.empty())
    {
      pipelineRep->setRepresentation(finalValue.c_str());
    }
    // ------------------------------------------------------------------------

    // ------------------------------------------------------------------------
    // Let's select some data array by default
    // ------------------------------------------------------------------------

    vtkSMProxy* input = vtkSMPropertyHelper(smRep, "Input").GetAsProxy();
    vtkSMSourceProxy* sourceProxy = vtkSMSourceProxy::SafeDownCast(input);
    if (sourceProxy)
    {
      vtkPVDataInformation* dataInfo = sourceProxy->GetDataInformation();

      if (dataInfo->GetPointDataInformation()->GetNumberOfArrays() > 0)
      {
        const char* scalarName =
          dataInfo->GetPointDataInformation()->GetArrayInformation(0)->GetName();
        pipelineRep->colorByArray(scalarName, 0); // 0: POINT_DATA / 1:CELL_DATA

        // --------------------------------------------------------------------
        // Let's change the data range for the lookup table to remove 1/8 of
        // the data range from the min and the max of the scalar range
        // --------------------------------------------------------------------
        QPair<double, double> range = pipelineRep->getLookupTable()->getScalarRange();
        double min = range.first + (range.second - range.first) / 8;
        double max = range.second - (range.second - range.first) / 8;

        // Apply changes
        pipelineRep->getLookupTable()->setScalarRangeLock(true);
        pipelineRep->getLookupTable()->setScalarRange(min, max);
      }
      else if (dataInfo->GetCellDataInformation()->GetNumberOfArrays() > 0)
      {
        const char* scalarName =
          dataInfo->GetCellDataInformation()->GetArrayInformation(0)->GetName();
        pipelineRep->colorByArray(scalarName, 1); // 0: POINT_DATA / 1:CELL_DATA

        // --------------------------------------------------------------------
        // Let's change the data range for the lookup table to remove 1/8 of
        // the data range from the min and the max of the scalar range
        // --------------------------------------------------------------------
        QPair<double, double> range = pipelineRep->getLookupTable()->getScalarRange();
        double min = range.first + (range.second - range.first) / 8;
        double max = range.second - (range.second - range.first) / 8;

        // Apply changes
        pipelineRep->getLookupTable()->setScalarRangeLock(true);
        pipelineRep->getLookupTable()->setScalarRange(min, max);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqSurfaceRepresentationBehavior::onViewAdded(pqView* view)
{
  QObject::connect(view, SIGNAL(representationAdded(pqRepresentation*)), this,
    SLOT(onRepresentationAdded(pqRepresentation*)), Qt::QueuedConnection);
}
