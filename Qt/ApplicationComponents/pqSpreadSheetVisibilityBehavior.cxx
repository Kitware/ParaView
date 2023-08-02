// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSpreadSheetVisibilityBehavior.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqPipelineSource.h"
#include "pqServerManagerModel.h"
#include "pqSpreadSheetView.h"
#include "vtkNew.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"

//-----------------------------------------------------------------------------
pqSpreadSheetVisibilityBehavior::pqSpreadSheetVisibilityBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(pqApplicationCore::instance()->getObjectBuilder(), SIGNAL(viewCreated(pqView*)),
    this, SLOT(showActiveSource(pqView*)));
}

//-----------------------------------------------------------------------------
void pqSpreadSheetVisibilityBehavior::showActiveSource(pqView* view)
{
  pqSpreadSheetView* spreadSheet = qobject_cast<pqSpreadSheetView*>(view);
  if (spreadSheet)
  {
    pqPipelineSource* source = pqActiveObjects::instance().activeSource();
    if (source != nullptr && source->modifiedState() != pqProxy::UNINITIALIZED)
    {
      pqOutputPort* port = pqActiveObjects::instance().activePort();
      if (!port)
      {
        port = source->getOutputPort(0);
      }

      vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
      controller->Show(source->getSourceProxy(), port->getPortNumber(), view->getViewProxy());
      // trigger an eventual-render.
      view->render();
    }
  }
}
