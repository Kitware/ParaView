// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqScalarBarVisibilityReaction.h"

#include "pqActiveObjects.h"
#include "pqDataRepresentation.h"
#include "pqRenderViewBase.h"
#include "pqTimer.h"
#include "pqUndoStack.h"
#include "vtkAbstractArray.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMTransferFunctionProxy.h"

#include <QDebug>
#include <QMessageBox>

#include <cassert>

//-----------------------------------------------------------------------------
pqScalarBarVisibilityReaction::pqScalarBarVisibilityReaction(
  QAction* parentObject, bool track_active_objects)
  : Superclass(parentObject)
  , BlockSignals(false)
  , TrackActiveObjects(track_active_objects)
  , Timer(new pqTimer(this))
{
  parentObject->setCheckable(true);

  this->Timer->setSingleShot(true);
  this->Timer->setInterval(0);
  this->connect(this->Timer, SIGNAL(timeout()), SLOT(updateEnableState()));

  QObject::connect(&pqActiveObjects::instance(),
    SIGNAL(representationChanged(pqDataRepresentation*)), this, SLOT(updateEnableState()),
    Qt::QueuedConnection);

  this->updateEnableState();
}

//-----------------------------------------------------------------------------
pqScalarBarVisibilityReaction::~pqScalarBarVisibilityReaction()
{
  delete this->Timer;
}

//-----------------------------------------------------------------------------
void pqScalarBarVisibilityReaction::updateEnableState()
{
  pqDataRepresentation* cachedRepr = this->CachedRepresentation;
  this->setRepresentation(
    this->TrackActiveObjects ? pqActiveObjects::instance().activeRepresentation() : cachedRepr);
}

//-----------------------------------------------------------------------------
pqDataRepresentation* pqScalarBarVisibilityReaction::representation() const
{
  return this->CachedRepresentation;
}

//-----------------------------------------------------------------------------
void pqScalarBarVisibilityReaction::setRepresentation(pqDataRepresentation* repr)
{
  if (this->CachedRepresentation)
  {
    this->CachedRepresentation->disconnect(this->Timer);
    this->Timer->disconnect(this->CachedRepresentation);
    this->CachedRepresentation = nullptr;
  }
  if (this->CachedView)
  {
    this->CachedView->disconnect(this->Timer);
    this->Timer->disconnect(this->CachedView);
    this->CachedView = nullptr;
  }

  vtkSMProxy* reprProxy = repr ? repr->getProxy() : nullptr;
  pqView* view = repr ? repr->getView() : nullptr;

  bool can_show_sb = repr && vtkSMColorMapEditorHelper::GetUsingScalarColoring(reprProxy);
  bool is_shown = false;
  if (repr)
  {
    this->CachedRepresentation = repr;
    this->CachedView = view;

    this->Timer->connect(repr, SIGNAL(colorTransferFunctionModified()), SLOT(start()));
    this->Timer->connect(repr, SIGNAL(colorArrayNameModified()), SLOT(start()));
    this->Timer->connect(
      view, SIGNAL(representationVisibilityChanged(pqRepresentation*, bool)), SLOT(start()));
  }

  if (can_show_sb)
  {
    assert(repr);
    assert(view);

    // get whether the scalar bar is currently shown.
    vtkSMProxy* sb = this->scalarBarProxy();
    is_shown = sb ? (vtkSMPropertyHelper(sb, "Visibility").GetAsInt() != 0) : false;
  }

  QAction* parent_action = this->parentAction();
  this->BlockSignals = true;
  parent_action->setEnabled(can_show_sb);
  parent_action->setChecked(is_shown);
  this->BlockSignals = false;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqScalarBarVisibilityReaction::scalarBarProxy() const
{
  pqDataRepresentation* repr = this->CachedRepresentation;
  vtkSMProxy* reprProxy = repr ? repr->getProxy() : nullptr;
  pqView* view = repr ? repr->getView() : nullptr;
  if (vtkSMColorMapEditorHelper::GetUsingScalarColoring(reprProxy))
  {
    return vtkSMTransferFunctionProxy::FindScalarBarRepresentation(
      repr->getLookupTableProxy(), view->getProxy());
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
void pqScalarBarVisibilityReaction::setScalarBarVisibility(bool visible)
{
  if (this->BlockSignals)
  {
    return;
  }

  pqDataRepresentation* repr = this->CachedRepresentation;
  if (!repr)
  {
    qCritical() << "Required active objects are not available.";
    return;
  }

  if (visible &&
    vtkSMColorMapEditorHelper::GetEstimatedNumberOfAnnotationsOnScalarBar(
      repr->getProxy(), repr->getView()->getProxy()) > vtkAbstractArray::MAX_DISCRETE_VALUES)
  {
    QMessageBox* box = new QMessageBox(QMessageBox::Warning, tr("Number of annotations warning"),
      tr("The color map have been configured to show lots of annotations."
         " Showing the scalar bar in this situation may slow down the rendering"
         " and it may not be readable anyway. Do you really want to show the color map ?"),
      QMessageBox::Yes | QMessageBox::No);
    box->move(QCursor::pos());
    if (box->exec() == QMessageBox::No)
    {
      visible = false;
      int blocked = this->parentAction()->blockSignals(true);
      this->parentAction()->setChecked(false);
      this->parentAction()->blockSignals(blocked);
    }
    delete box;
  }
  BEGIN_UNDO_SET(tr("Toggle Color Legend Visibility"));
  vtkSMColorMapEditorHelper::SetScalarBarVisibility(
    repr->getProxy(), repr->getView()->getProxy(), visible);
  END_UNDO_SET();
  repr->renderViewEventually();
}
