// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqZSpaceManager.h"

#include "pqApplicationCore.h"
#include "pqPipelineSource.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"
#include "pqView.h"
#include "vtkSMViewProxy.h"

//-----------------------------------------------------------------------------
pqZSpaceManager::pqZSpaceManager(QObject* p)
  : QObject(p)
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(
    smmodel, &pqServerManagerModel::preViewAdded, this, &pqZSpaceManager::onViewAdded);
  QObject::connect(
    smmodel, &pqServerManagerModel::preViewRemoved, this, &pqZSpaceManager::onViewRemoved);

  // Add currently existing ZSpace views
  for (pqView* view : smmodel->findItems<pqView*>())
  {
    this->onViewAdded(view);
  }
}

//-----------------------------------------------------------------------------
void pqZSpaceManager::onRenderEnded()
{
  pqView* view = dynamic_cast<pqView*>(sender());
  if (view != nullptr)
  {
    view->render();
  }
}

//-----------------------------------------------------------------------------
void pqZSpaceManager::onViewAdded(pqView* view)
{
  if (dynamic_cast<pqRenderView*>(view))
  {
    const std::string viewName = view->getViewProxy()->GetVTKClassName();
    if (viewName == "vtkPVZSpaceView")
    {
      this->ZSpaceViews.insert(view);
      QObject::connect(view, SIGNAL(endRender()), this, SLOT(onRenderEnded()));
    }
  }
}

//-----------------------------------------------------------------------------
void pqZSpaceManager::onViewRemoved(pqView* view)
{
  if (dynamic_cast<pqRenderView*>(view))
  {
    const std::string viewName = view->getViewProxy()->GetVTKClassName();
    if (viewName == "vtkPVZSpaceView")
    {
      QObject::disconnect(view, SIGNAL(endRender()), this, SLOT(onRenderEnded()));
      this->ZSpaceViews.erase(view);
    }
  }
}
