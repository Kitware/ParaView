// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqStreamLinesAnimationManager.h"

#include "pqApplicationCore.h"
#include "pqRenderView.h"
#include "pqRepresentation.h"
#include "pqServerManagerModel.h"
#include "pqView.h"

#include "vtkSMPropertyHelper.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMViewProxy.h"

#include <QtDebug>

//-----------------------------------------------------------------------------
pqStreamLinesAnimationManager::pqStreamLinesAnimationManager(QObject* p /*=nullptr*/)
  : QObject(p)
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smmodel, SIGNAL(preViewAdded(pqView*)), this, SLOT(onViewAdded(pqView*)));
  QObject::connect(smmodel, SIGNAL(preViewRemoved(pqView*)), this, SLOT(onViewRemoved(pqView*)));

  // Add currently existing views
  Q_FOREACH (pqView* view, smmodel->findItems<pqView*>())
  {
    this->onViewAdded(view);
  }
}

//-----------------------------------------------------------------------------
pqStreamLinesAnimationManager::~pqStreamLinesAnimationManager() = default;

//-----------------------------------------------------------------------------
void pqStreamLinesAnimationManager::onRenderEnded()
{
  pqView* view = dynamic_cast<pqView*>(sender());
  QList<pqRepresentation*> reprs = view->getRepresentations();
  for (int i = 0; i < reprs.count(); ++i)
  {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(reprs[i]->getProxy());
    if (repr && repr->GetProperty("Representation"))
    {
      const char* representation = vtkSMPropertyHelper(repr, "Representation").GetAsString();
      const int visible = vtkSMPropertyHelper(repr, "Visibility").GetAsInt();
      if (representation && (std::string(representation) == "Stream Lines") && visible)
      {
        // This view as a visible StreamLines representation.
        // Let's ask for a new render!
        view->render();
        break;
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqStreamLinesAnimationManager::onViewAdded(pqView* view)
{
  if (dynamic_cast<pqRenderView*>(view))
  {
    this->Views.insert(view);
    QObject::connect(view, SIGNAL(endRender()), this, SLOT(onRenderEnded()));
  }
}

//-----------------------------------------------------------------------------
void pqStreamLinesAnimationManager::onViewRemoved(pqView* view)
{
  if (dynamic_cast<pqRenderView*>(view))
  {
    QObject::disconnect(view, SIGNAL(endRender()), this, SLOT(onRenderEnded()));
    this->Views.erase(view);
  }
}
