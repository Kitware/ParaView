/*=========================================================================

   Program: ParaView
   Module:    pqEditColorMapReaction.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqEditColorMapReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqPipelineRepresentation.h"
#include "pqUndoStack.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMTrace.h"

#include <QColorDialog>
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
  this->parentAction()->setEnabled(this->Representation != NULL);
}

//-----------------------------------------------------------------------------
void pqEditColorMapReaction::onTriggered()
{
  pqEditColorMapReaction::editColorMap(this->Representation);
}

//-----------------------------------------------------------------------------
void pqEditColorMapReaction::editColorMap(pqPipelineRepresentation* repr)
{
  if (repr == NULL)
  {
    repr =
      qobject_cast<pqPipelineRepresentation*>(pqActiveObjects::instance().activeRepresentation());
    if (!repr)
    {
      qCritical() << "No representation provided.";
      return;
    }
  }

  if (!vtkSMPVRepresentationProxy::GetUsingScalarColoring(repr->getProxy()))
  {
    // Get the color property.
    vtkSMProxy* proxy = repr->getProxy();
    SM_SCOPED_TRACE(PropertiesModified).arg(proxy).arg("comment", " change solid color");

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
        pqCoreUtilities::mainWidget(), "Pick Solid Color", QColorDialog::DontUseNativeDialog);
      if (color.isValid())
      {
        // Set the properties to the new color.
        rgb[0] = color.redF();
        rgb[1] = color.greenF();
        rgb[2] = color.blueF();
        BEGIN_UNDO_SET("Changed Solid Color");
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
