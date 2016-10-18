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
#include "pqSMAdaptor.h"
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
    QString reprType = vtkSMPropertyHelper(proxy, "Representation", /*quiet=*/true).GetAsString();
    bool use_ambient = (reprType == "Wireframe" || reprType == "Points" || reprType == "Outline");
    if (diffuse && ambient)
    {
      // Get the current color from the property.
      QList<QVariant> rgb = pqSMAdaptor::getMultipleElementProperty(diffuse);
      QColor color(Qt::white);
      if (rgb.size() >= 3)
      {
        color = QColor::fromRgbF(rgb[0].toDouble(), rgb[1].toDouble(), rgb[2].toDouble());
      }

      // Let the user pick a new color.
      color = QColorDialog::getColor(color, pqCoreUtilities::mainWidget(), "Pick Solid Color",
        QColorDialog::DontUseNativeDialog);
      if (color.isValid())
      {
        // Set the properties to the new color.
        rgb.clear();
        rgb.append(color.redF());
        rgb.append(color.greenF());
        rgb.append(color.blueF());
        BEGIN_UNDO_SET("Changed Solid Color");
        pqSMAdaptor::setMultipleElementProperty(use_ambient ? ambient : diffuse, rgb);
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
