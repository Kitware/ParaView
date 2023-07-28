// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
    Q_FOREACH (QVariant v, list)
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
