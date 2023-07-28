// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCustomViewpointsToolbar.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCameraDialog.h"
#include "pqCustomViewpointButtonDialog.h"
#include "pqCustomViewpointsController.h"
#include "pqCustomViewpointsDefaultController.h"
#include "pqRenderView.h"
#include "pqSettings.h"
#include <QApplication>
#include <QPainter>

//-----------------------------------------------------------------------------
void pqCustomViewpointsToolbar::constructor()
{
  // Use the default controller if no controller was given.
  if (!this->Controller)
  {
    this->Controller = new pqCustomViewpointsDefaultController(this);
  }

  this->Controller->setToolbar(this);

  // Create base pixmap
  this->BasePixmap.fill(QColor(0, 0, 0, 0));
  QPainter pixPaint(&this->BasePixmap);
  pixPaint.drawPixmap(0, 0, 48, 48, QPixmap(":/pqWidgets/Icons/pqCamera.svg"));

  // Create plus pixmap
  this->PlusPixmap = this->BasePixmap.copy();
  QPainter pixWithPlusPaint(&this->PlusPixmap);
  pixWithPlusPaint.drawPixmap(32, 32, 32, 32, QPixmap(":/QtWidgets/Icons/pqPlus.svg"));
  this->ConfigPixmap = this->BasePixmap.copy();
  QPainter pixWithConfigPaint(&this->ConfigPixmap);
  pixWithConfigPaint.drawPixmap(32, 32, 32, 32, QPixmap(":/pqWidgets/Icons/pqWrench.svg"));

  this->PlusAction = this->ConfigAction = nullptr;

  this->setWindowTitle(tr("Custom Viewpoints Toolbar"));
  this->updateCustomViewpointActions();
  this->connect(
    &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), SLOT(updateEnabledState()));
}

//-----------------------------------------------------------------------------
void pqCustomViewpointsToolbar::updateEnabledState()
{
  this->setEnabled(
    qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView()) != nullptr);
  auto actions = this->actions();
  actions[actions.size() - 2]->setEnabled(
    actions.size() < pqCustomViewpointButtonDialog::MAXIMUM_NUMBER_OF_ITEMS + 2);
}

//-----------------------------------------------------------------------------
void pqCustomViewpointsToolbar::updateCustomViewpointActions()
{
  // Recover tooltips from settings
  QStringList tooltips = this->Controller->getCustomViewpointToolTips();

  if (!this->ConfigAction)
  {
    this->ConfigAction = this->addAction(QIcon(this->ConfigPixmap),
      tr("Configure custom viewpoints"), this, SLOT(configureCustomViewpoints()));
    this->ConfigAction->setObjectName("ConfigAction");
  }

  if (!this->PlusAction)
  {
    this->PlusAction =
      this->addAction(QIcon(this->PlusPixmap), tr("Add current viewpoint as custom viewpoint"),
        this, SLOT(addCurrentViewpointToCustomViewpoints()));
    this->PlusAction->setObjectName("PlusAction");
  }

  // Remove unused actions
  for (int cc = this->ViewpointActions.size(); cc > tooltips.size(); cc--)
  {
    this->removeAction(this->ViewpointActions[cc - 1]);
  }
  if (this->ViewpointActions.size() > tooltips.size())
  {
    this->ViewpointActions.resize(tooltips.size());
  }

  // add / change actions for custom views.
  for (int cc = 0; cc < tooltips.size(); cc++)
  {
    QPixmap pixmap;
    if (this->ViewpointActions.size() > cc)
    {
      this->ViewpointActions[cc]->setToolTip(tooltips[cc]);
    }
    else
    {
      // action does not exist yet, create it
      pixmap = this->BasePixmap.copy();
      QPainter pixWithNumberPaint(&pixmap);
      QFont font = pixWithNumberPaint.font();
      font.setPixelSize(24);
      pixWithNumberPaint.setFont(font);
      pixWithNumberPaint.setPen(QApplication::palette().windowText().color());
      pixWithNumberPaint.drawText(
        pixmap.rect(), Qt::AlignRight | Qt::AlignBottom, QString::number(cc + 1));

      QAction* action = this->addAction(QIcon(pixmap), "", this, SLOT(applyCustomViewpoint()));
      action->setObjectName(QString("ViewpointAction%1").arg(cc));
      action->setToolTip(tooltips[cc]);
      action->setData(cc);
      this->ViewpointActions.push_back(action);
    }
  }

  this->updateEnabledState();
}

//-----------------------------------------------------------------------------
void pqCustomViewpointsToolbar::configureCustomViewpoints()
{
  this->Controller->configureCustomViewpoints();
  this->updateCustomViewpointActions();
}

//-----------------------------------------------------------------------------
void pqCustomViewpointsToolbar::setToCurrentViewpoint()
{
  QAction* action = qobject_cast<QAction*>(this->sender());
  if (!action)
  {
    return;
  }

  const int customViewpointIndex = action->data().toInt();
  this->Controller->setToCurrentViewpoint(customViewpointIndex);
  this->updateCustomViewpointActions();
}

//-----------------------------------------------------------------------------
void pqCustomViewpointsToolbar::deleteCustomViewpoint()
{
  QAction* action = qobject_cast<QAction*>(this->sender());
  if (!action)
  {
    return;
  }

  const int customViewpointIndex = action->data().toInt();
  this->Controller->deleteCustomViewpoint(customViewpointIndex);
  this->updateCustomViewpointActions();
}

//-----------------------------------------------------------------------------
void pqCustomViewpointsToolbar::addCurrentViewpointToCustomViewpoints()
{
  this->Controller->addCurrentViewpointToCustomViewpoints();
  this->updateCustomViewpointActions();
}

//-----------------------------------------------------------------------------
void pqCustomViewpointsToolbar::applyCustomViewpoint()
{
  QAction* action = qobject_cast<QAction*>(this->sender());
  if (!action)
  {
    return;
  }

  const int customViewpointIndex = action->data().toInt();
  this->Controller->applyCustomViewpoint(customViewpointIndex);
}
