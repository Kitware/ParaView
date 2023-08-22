// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqEditColorMapReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqPipelineRepresentation.h"
#include "pqUndoStack.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMTrace.h"

#include <QColorDialog>
#include <QCoreApplication>
#include <QDebug>
#include <QDockWidget>

//-----------------------------------------------------------------------------
pqEditColorMapReaction::pqEditColorMapReaction(QAction* parentObject, bool track_active_objects)
  : Superclass(parentObject)
{
  if (track_active_objects)
  {
    QObject::connect(&pqActiveObjects::instance(),
      SIGNAL(representationChanged(pqDataRepresentation*)), this,
      SLOT(setRepresentation(pqDataRepresentation*)));
    this->setRepresentation(pqActiveObjects::instance().activeRepresentation());
  }
}

//-----------------------------------------------------------------------------
void pqEditColorMapReaction::setRepresentation(pqDataRepresentation* repr)
{
  this->Representation = qobject_cast<pqPipelineRepresentation*>(repr);
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqEditColorMapReaction::updateEnableState()
{
  this->parentAction()->setEnabled(this->Representation != nullptr);
}

//-----------------------------------------------------------------------------
void pqEditColorMapReaction::onTriggered()
{
  pqEditColorMapReaction::editColorMap(this->Representation);
}

//-----------------------------------------------------------------------------
void pqEditColorMapReaction::editColorMap(pqPipelineRepresentation* repr)
{
  if (repr == nullptr)
  {
    repr =
      qobject_cast<pqPipelineRepresentation*>(pqActiveObjects::instance().activeRepresentation());
    if (!repr)
    {
      qCritical() << "No representation provided.";
      return;
    }
  }

  if (!vtkSMColorMapEditorHelper::GetUsingScalarColoring(repr->getProxy()))
  {
    // Get the color property.
    vtkSMProxy* proxy = repr->getProxy();
    SM_SCOPED_TRACE(PropertiesModified)
      .arg(proxy)
      .arg("comment", qPrintable(tr(" change solid color")));

    vtkSMProperty* diffuse = proxy->GetProperty("DiffuseColor");
    vtkSMProperty* ambient = proxy->GetProperty("AmbientColor");
    if (diffuse == nullptr && ambient == nullptr)
    {
      diffuse = proxy->GetProperty("Color");
    }

    if (diffuse || ambient)
    {
      // Get the current color from the property.
      double rgb[3];
      vtkSMPropertyHelper(diffuse ? diffuse : ambient).Get(rgb, 3);

      // Let the user pick a new color.
      auto color = QColorDialog::getColor(QColor::fromRgbF(rgb[0], rgb[1], rgb[2]),
        pqCoreUtilities::mainWidget(), tr("Pick Solid Color"), QColorDialog::DontUseNativeDialog);
      if (color.isValid())
      {
        // Set the properties to the new color.
        rgb[0] = color.redF();
        rgb[1] = color.greenF();
        rgb[2] = color.blueF();
        BEGIN_UNDO_SET(tr("Changed Solid Color"));
        vtkSMPropertyHelper(diffuse, /*quiet=*/true).Set(rgb, 3);
        vtkSMPropertyHelper(ambient, /*quiet=*/true).Set(rgb, 3);
        proxy->UpdateVTKObjects();
        repr->renderViewEventually();
        END_UNDO_SET();
      }
    }
  }
  else
  {
    // Raise the color editor is present in the application.
    QDockWidget* widget =
      qobject_cast<QDockWidget*>(pqApplicationCore::instance()->manager("COLOR_EDITOR_PANEL"));
    if (widget)
    {
      widget->setVisible(true);
      // widget->setFloating(true);
      widget->raise();
    }
    else
    {
      qDebug("Failed to find 'COLOR_EDITOR_PANEL'.");
    }
  }
}
