// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqEditColorMapReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqColorMapEditor.h"
#include "pqCoreUtilities.h"
#include "pqPipelineRepresentation.h"
#include "pqUndoStack.h"

#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"

#include <QColorDialog>
#include <QCoreApplication>
#include <QDebug>
#include <QDockWidget>
#include <QString>

#include <array>

//-----------------------------------------------------------------------------
pqEditColorMapReaction::pqEditColorMapReaction(QAction* parentObject, bool track_active_objects)
  : Superclass(parentObject)
{
  if (track_active_objects)
  {
    QObject::connect(&pqActiveObjects::instance(),
      QOverload<pqDataRepresentation*>::of(&pqActiveObjects::representationChanged), this,
      &pqEditColorMapReaction::setActiveRepresentation);
    this->setActiveRepresentation();
  }
  else
  {
    this->updateEnableState();
  }
}

//-----------------------------------------------------------------------------
pqEditColorMapReaction::~pqEditColorMapReaction() = default;

//-----------------------------------------------------------------------------
void pqEditColorMapReaction::setActiveRepresentation()
{
  this->setRepresentation(pqActiveObjects::instance().activeRepresentation());
}

//-----------------------------------------------------------------------------
void pqEditColorMapReaction::setRepresentation(
  pqDataRepresentation* repr, int selectedPropertiesType)
{
  if (this->Representation != nullptr && this->Representation == repr &&
    selectedPropertiesType == this->ColorMapEditorHelper->GetSelectedPropertiesType())
  {
    return;
  }
  this->ColorMapEditorHelper->SetSelectedPropertiesType(selectedPropertiesType);
  this->Representation = qobject_cast<pqPipelineRepresentation*>(repr);
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqEditColorMapReaction::updateEnableState()
{
  this->parentAction()->setEnabled(this->Representation != nullptr);
  auto colorMapEditorDock =
    qobject_cast<QDockWidget*>(pqApplicationCore::instance()->manager("COLOR_EDITOR_PANEL"));
  this->parentAction()->setChecked(colorMapEditorDock && colorMapEditorDock->isVisible());
}

//-----------------------------------------------------------------------------
void pqEditColorMapReaction::onTriggered()
{
  pqEditColorMapReaction::editColorMap(this->Representation);
}

//-----------------------------------------------------------------------------
void pqEditColorMapReaction::editColorMap(pqPipelineRepresentation* repr)
{
  if (!repr)
  {
    repr =
      qobject_cast<pqPipelineRepresentation*>(pqActiveObjects::instance().activeRepresentation());
    if (!repr)
    {
      qCritical() << "No representation provided.";
      return;
    }
  }

  if (!this->ColorMapEditorHelper->GetAnySelectedUsingScalarColoring(repr->getProxy()))
  {
    // Get the color property.
    vtkSMProxy* proxy = repr->getProxy();
    std::array<double, 3> rgb = this->ColorMapEditorHelper->GetSelectedColors(proxy)[0];
    // if no valid color found, get the color of the representation
    QString extraInfo;
    if (this->ColorMapEditorHelper->GetSelectedPropertiesType() ==
      vtkSMColorMapEditorHelper::SelectedPropertiesTypes::Blocks)
    {
      extraInfo = tr("Block ");
      if (!vtkSMColorMapEditorHelper::IsColorValid(rgb))
      {
        rgb = this->ColorMapEditorHelper->GetColor(proxy);
      }
    }
    if (vtkSMColorMapEditorHelper::IsColorValid(rgb))
    {
      // Let the user pick a new color.
      const QString dialogTitle = tr("Pick ") + extraInfo + tr("Solid Color");
      auto color = QColorDialog::getColor(QColor::fromRgbF(rgb[0], rgb[1], rgb[2]),
        pqCoreUtilities::mainWidget(), dialogTitle, QColorDialog::DontUseNativeDialog);
      if (color.isValid())
      {
        // Set the properties to the new color.
        rgb = { color.redF(), color.greenF(), color.blueF() };
        const QString undoName = tr("Change ") + extraInfo + tr("Solid Color");
        BEGIN_UNDO_SET(undoName);
        this->ColorMapEditorHelper->SetSelectedColor(proxy, rgb);
        // no need to call repr->UpdateVTKObjects(), the SetSelectedColor call will do that
        repr->renderViewEventually();
        END_UNDO_SET();
      }
    }
    // Update the checked state of the toolbar action
    this->parentAction()->setChecked(false);
  }
  else
  {
    // Raise the color editor is present in the application.
    auto colorMapEditorDock =
      qobject_cast<QDockWidget*>(pqApplicationCore::instance()->manager("COLOR_EDITOR_PANEL"));
    if (colorMapEditorDock)
    {
      auto colorMapEditor = qobject_cast<pqColorMapEditor*>(colorMapEditorDock->widget());
      colorMapEditor->setSelectedPropertiesType(
        this->ColorMapEditorHelper->GetSelectedPropertiesType());
      colorMapEditorDock->setVisible(this->parentAction()->isChecked());
      colorMapEditorDock->raise();
      QObject::connect(colorMapEditorDock, &QDockWidget::visibilityChanged, this->parentAction(),
        &QAction::setChecked, Qt::UniqueConnection);
    }
    else
    {
      qDebug("Failed to find 'COLOR_EDITOR_PANEL'.");
    }
  }
}
