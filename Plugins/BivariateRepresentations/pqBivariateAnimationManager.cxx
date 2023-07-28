// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqBivariateAnimationManager.h"

#include "pqApplicationCore.h"
#include "pqRenderView.h"
#include "pqRepresentation.h"
#include "pqServerManagerModel.h"
#include "pqView.h"

#include "vtkSMPropertyHelper.h"
#include "vtkSMRepresentationProxy.h"

//-----------------------------------------------------------------------------
pqBivariateAnimationManager::pqBivariateAnimationManager(QObject* p)
  : QObject(p)
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smmodel, SIGNAL(preViewAdded(pqView*)), this, SLOT(onViewAdded(pqView*)));
  QObject::connect(smmodel, SIGNAL(preViewRemoved(pqView*)), this, SLOT(onViewRemoved(pqView*)));

  // Add currently existing views
  for (pqView* view : smmodel->findItems<pqView*>())
  {
    this->onViewAdded(view);
  }
}

//-----------------------------------------------------------------------------
pqBivariateAnimationManager::~pqBivariateAnimationManager() = default;

//-----------------------------------------------------------------------------
void pqBivariateAnimationManager::onRenderEnded()
{
  pqView* view = dynamic_cast<pqView*>(QObject::sender());
  QList<pqRepresentation*> reprs = view->getRepresentations();
  for (int i = 0; i < reprs.count(); ++i)
  {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(reprs[i]->getProxy());
    if (repr && repr->GetProperty("Representation"))
    {
      const char* rs = vtkSMPropertyHelper(repr, "Representation").GetAsString();
      const int visible = vtkSMPropertyHelper(repr, "Visibility").GetAsInt();
      if (rs && !strcmp(rs, "Bivariate Noise Surface") && visible)
      {
        // If the view has a visible bivariate noise representation,
        // then ask for a new render.
        view->render();
        break;
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqBivariateAnimationManager::onViewAdded(pqView* view)
{
  if (dynamic_cast<pqRenderView*>(view))
  {
    this->Views.insert(view);
    QObject::connect(view, SIGNAL(endRender()), this, SLOT(onRenderEnded()));
  }
}

//-----------------------------------------------------------------------------
void pqBivariateAnimationManager::onViewRemoved(pqView* view)
{
  if (dynamic_cast<pqRenderView*>(view))
  {
    QObject::disconnect(view, SIGNAL(endRender()), this, SLOT(onRenderEnded()));
    this->Views.erase(view);
  }
}
