// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPointPickingHelper.h"

#include "pqKeySequences.h"
#include "pqModalShortcut.h"
#include "pqRenderView.h"
#include "pqShortcutDecorator.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMRenderViewProxy.h"

#include <QDebug>
#include <QShortcut>

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
#define QT_ENDL endl
#else
#define QT_ENDL Qt::endl
#endif

#include <cmath>

//-----------------------------------------------------------------------------
pqPointPickingHelper::pqPointPickingHelper(const QKeySequence& keySequence, bool pick_on_mesh,
  pqPropertyWidget* propWidget, PickOption pickOpt, bool pickCameraFocalInfo)
  : Superclass(propWidget)
  , PickOnMesh(pick_on_mesh)
  , PickOpt(pickOpt)
  , PickCameraFocalInfo(pickCameraFocalInfo)
{
  if (!propWidget)
  {
    throw std::invalid_argument("Parent property-widget must be non-null.");
  }
  auto* decorator = propWidget->findChild<pqShortcutDecorator*>();
  if (!decorator)
  {
    decorator = new pqShortcutDecorator(propWidget);
  }
  this->Shortcut =
    pqKeySequences::instance().addModalShortcut(keySequence, /* action */ nullptr, propWidget);
  decorator->addShortcut(this->Shortcut);
  this->connect(this->Shortcut, SIGNAL(activated()), SLOT(pickPoint()));
}

//-----------------------------------------------------------------------------
pqPointPickingHelper::~pqPointPickingHelper()
{
  delete this->Shortcut;
}

//-----------------------------------------------------------------------------
void pqPointPickingHelper::setView(pqView* view)
{
  pqRenderView* rview = qobject_cast<pqRenderView*>(view);
  if (rview == this->View)
  {
    return;
  }
  this->View = rview;
  if (rview)
  {
    this->Shortcut->setContextWidget(view->widget());
  }
}

//-----------------------------------------------------------------------------
void pqPointPickingHelper::pickPoint()
{
  pqRenderView* rview = this->View;
  if (rview && rview->getRenderViewProxy())
  {
    vtkRenderWindowInteractor* rwi = rview->getRenderViewProxy()->GetInteractor();
    if (!rwi)
    {
      return;
    }

    // Only pick if the keypress event actually happened in the window
    QPointF pos = rview->widget()->mapFromGlobal(QCursor::pos());
    QSize sz = rview->getSize();
    bool outside = pos.x() < 0 || pos.x() > sz.width() || pos.y() < 0 || pos.y() > sz.height();
    if (outside)
    {
      return;
    }

    // Get region
    const int* eventpos = rwi->GetEventPosition();
    double position[3], normal[3];
    if (rview->getRenderViewProxy()->ConvertDisplayToPointOnSurface(
          eventpos, position, normal, this->PickOnMesh) ||
      this->PickCameraFocalInfo)
    {
      auto lambdaIsValidVector = [](const double x[3]) {
        return (!std::isnan(x[0]) || !std::isnan(x[1]) || !std::isnan(x[2]));
      };
      switch (this->PickOpt)
      {
        case PickOption::Coordinates:
        {
          if (lambdaIsValidVector(position))
          {
            Q_EMIT this->pick(position[0], position[1], position[2]);
          }
          // else statement is not needed because vtkPVRayCastPickingHelper prints an error message
        }
        break;
        case PickOption::Normal:
        {
          if (lambdaIsValidVector(normal))
          {
            Q_EMIT this->pick(normal[0], normal[1], normal[2]);
          }
          else
          {
            qWarning() << "The intersection normal was not available" << QT_ENDL;
          }
        }
        break;
        case PickOption::CoordinatesAndNormal:
        {
          if (lambdaIsValidVector(position) && lambdaIsValidVector(normal))
          {
            Q_EMIT this->pickNormal(
              position[0], position[1], position[2], normal[0], normal[1], normal[2]);
          }
          else
          {
            qWarning() << "The intersection normal was not available" << QT_ENDL;
          }
        }
        break;
      }
    }
  }
}
