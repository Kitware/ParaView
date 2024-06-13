// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqScalarBarVisibilityReaction.h"

#include "pqActiveObjects.h"
#include "pqDataRepresentation.h"
#include "pqTimer.h"
#include "pqUndoStack.h"
#include "pqView.h"

#include "vtkAbstractArray.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMTransferFunctionProxy.h"

#include <QAction>
#include <QCursor>
#include <QDebug>
#include <QMessageBox>
#include <QObject>
#include <QPointer>
#include <QString>

#include <vector>

//-----------------------------------------------------------------------------
pqScalarBarVisibilityReaction::pqScalarBarVisibilityReaction(
  QAction* parentObject, bool track_active_objects)
  : Superclass(parentObject)
  , Timer(new pqTimer(this))
{
  parentObject->setCheckable(true);
  this->Timer->setSingleShot(true);
  this->Timer->setInterval(0);
  QObject::connect(
    this->Timer, &pqTimer::timeout, this, &pqScalarBarVisibilityReaction::updateEnableState);

  if (track_active_objects)
  {
    QObject::connect(&pqActiveObjects::instance(),
      QOverload<pqDataRepresentation*>::of(&pqActiveObjects::representationChanged), this,
      &pqScalarBarVisibilityReaction::setActiveRepresentation, Qt::QueuedConnection);
    this->setActiveRepresentation();
  }
  else
  {
    this->updateEnableState();
  }
}

//-----------------------------------------------------------------------------
pqScalarBarVisibilityReaction::~pqScalarBarVisibilityReaction() = default;

//-----------------------------------------------------------------------------
pqDataRepresentation* pqScalarBarVisibilityReaction::representation() const
{
  return this->Representation;
}

//-----------------------------------------------------------------------------
void pqScalarBarVisibilityReaction::setActiveRepresentation()
{
  this->setRepresentation(pqActiveObjects::instance().activeRepresentation());
}

//-----------------------------------------------------------------------------
void pqScalarBarVisibilityReaction::setRepresentation(
  pqDataRepresentation* repr, int selectedPropertiesType)
{
  if (this->Representation != nullptr && this->Representation == repr &&
    this->ColorMapEditorHelper->GetSelectedPropertiesType() == selectedPropertiesType)
  {
    return;
  }
  if (this->Representation)
  {
    this->Representation->disconnect(this->Timer);
    this->Timer->disconnect(this->Representation);
    this->Representation = nullptr;
  }
  if (this->View)
  {
    this->View->disconnect(this->Timer);
    this->Timer->disconnect(this->View);
    this->View = nullptr;
  }
  this->ColorMapEditorHelper->SetSelectedPropertiesType(selectedPropertiesType);

  if (repr && repr->getView())
  {
    this->Representation = repr;
    this->View = repr->getView();

    // connect the representation's and view's signals to a timer's start,
    // such that a state update occurs only when a previous start has timeout.
    QObject::connect(this->View, &pqView::representationVisibilityChanged, this->Timer,
      QOverload<>::of(&pqTimer::start));
    if (this->ColorMapEditorHelper->GetSelectedPropertiesType() ==
      vtkSMColorMapEditorHelper::SelectedPropertiesTypes::Blocks)
    {
      QObject::connect(this->Representation,
        &pqDataRepresentation::blockColorTransferFunctionModified, this->Timer,
        QOverload<>::of(&pqTimer::start));
      QObject::connect(this->Representation, &pqDataRepresentation::blockColorArrayNameModified,
        this->Timer, QOverload<>::of(&pqTimer::start));
    }
    else
    {
      QObject::connect(this->Representation, &pqDataRepresentation::colorTransferFunctionModified,
        this->Timer, QOverload<>::of(&pqTimer::start));
      QObject::connect(this->Representation, &pqDataRepresentation::colorArrayNameModified,
        this->Timer, QOverload<>::of(&pqTimer::start));
    }
  }
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqScalarBarVisibilityReaction::updateEnableState()
{
  vtkSMProxy* reprProxy = this->Representation ? this->Representation->getProxy() : nullptr;
  const bool canShowSB = this->Representation &&
    this->ColorMapEditorHelper->GetAnySelectedUsingScalarColoring(reprProxy);
  bool isShown = false;
  if (canShowSB)
  {
    // get whether the scalar bar is currently shown.
    vtkSMProxy* sb = this->scalarBarProxy();
    isShown = sb ? (vtkSMPropertyHelper(sb, "Visibility").GetAsInt() != 0) : false;
  }

  const bool prev = this->blockSignals(true);
  QAction* parentAction = this->parentAction();
  parentAction->setEnabled(canShowSB);
  parentAction->setChecked(isShown);
  this->blockSignals(prev);
}

//-----------------------------------------------------------------------------
std::vector<vtkSMProxy*> pqScalarBarVisibilityReaction::scalarBarProxies() const
{
  pqDataRepresentation* repr = this->Representation;
  vtkSMProxy* reprProxy = repr ? repr->getProxy() : nullptr;
  pqView* view = repr ? repr->getView() : nullptr;
  std::vector<vtkSMProxy*> proxies;
  if (this->ColorMapEditorHelper->GetAnySelectedUsingScalarColoring(reprProxy))
  {
    auto luts = this->ColorMapEditorHelper->GetSelectedLookupTables(reprProxy);
    for (auto& lut : luts)
    {
      proxies.push_back(
        vtkSMTransferFunctionProxy::FindScalarBarRepresentation(lut, view->getProxy()));
    }
  }
  return proxies;
}

//-----------------------------------------------------------------------------
void pqScalarBarVisibilityReaction::setScalarBarVisibility(bool visible)
{
  pqDataRepresentation* repr = this->Representation;
  if (!repr)
  {
    qCritical() << "Required active objects are not available.";
    return;
  }

  if (visible &&
    this->ColorMapEditorHelper->GetAnySelectedEstimatedNumberOfAnnotationsOnScalarBar(
      repr->getProxy(), repr->getView()->getProxy()) > vtkAbstractArray::MAX_DISCRETE_VALUES)
  {
    QPointer<QMessageBox> box =
      new QMessageBox(QMessageBox::Warning, tr("Number of annotations warning"),
        tr("The color map have been configured to show lots of annotations."
           " Showing the scalar bar in this situation may slow down the rendering"
           " and it may not be readable anyway. Do you really want to show the color map ?"),
        QMessageBox::Yes | QMessageBox::No);
    box->move(QCursor::pos());
    if (box->exec() == QMessageBox::No)
    {
      visible = false;
      const bool blocked = this->parentAction()->blockSignals(true);
      this->parentAction()->setChecked(false);
      this->parentAction()->blockSignals(blocked);
    }
  }
  const QString extraInfo = this->ColorMapEditorHelper->GetSelectedPropertiesType() ==
      vtkSMColorMapEditorHelper::SelectedPropertiesTypes::Blocks
    ? tr("Block ")
    : QString();
  const QString undoName = tr("Toggle ") + extraInfo + tr("Color Legend Visibility");
  BEGIN_UNDO_SET(extraInfo);
  this->ColorMapEditorHelper->SetSelectedScalarBarVisibility(
    repr->getProxy(), repr->getView()->getProxy(), visible);
  END_UNDO_SET();
  repr->renderViewEventually();
}
