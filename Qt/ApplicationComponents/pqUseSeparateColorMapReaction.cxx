// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqUseSeparateColorMapReaction.h"

#include "pqActiveObjects.h"
#include "pqDataRepresentation.h"
#include "pqDisplayColorWidget.h"
#include "pqView.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMScalarBarWidgetRepresentationProxy.h"
#include "vtkSMTransferFunctionProxy.h"

//-----------------------------------------------------------------------------
pqUseSeparateColorMapReaction::pqUseSeparateColorMapReaction(
  QAction* parentObject, pqDisplayColorWidget* colorWidget, bool track_active_objects)
  : Superclass(parentObject)
  , ColorWidget(colorWidget)
  , TrackActiveObjects(track_active_objects)
  , BlockSignals(false)
{
  parentObject->setCheckable(true);
  QObject::connect(&pqActiveObjects::instance(),
    SIGNAL(representationChanged(pqDataRepresentation*)), this, SLOT(updateEnableState()),
    Qt::QueuedConnection);
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
pqUseSeparateColorMapReaction::~pqUseSeparateColorMapReaction() = default;

//-----------------------------------------------------------------------------
void pqUseSeparateColorMapReaction::updateEnableState()
{
  pqDataRepresentation* cachedRepr = this->CachedRepresentation;
  this->setRepresentation(
    this->TrackActiveObjects ? pqActiveObjects::instance().activeRepresentation() : cachedRepr);
}

//-----------------------------------------------------------------------------
pqDataRepresentation* pqUseSeparateColorMapReaction::representation() const
{
  return this->CachedRepresentation;
}

//-----------------------------------------------------------------------------
void pqUseSeparateColorMapReaction::setRepresentation(pqDataRepresentation* repr)
{
  this->Links.clear();
  if (this->CachedRepresentation)
  {
    QObject::disconnect(this->CachedRepresentation, SIGNAL(colorArrayNameModified()), this,
      SLOT(updateEnableState()));
  }
  this->CachedRepresentation = repr;
  if (repr)
  {
    QObject::connect(repr, SIGNAL(colorArrayNameModified()), this, SLOT(updateEnableState()),
      Qt::QueuedConnection);
  }

  // Recover proxy and action
  vtkSMProxy* reprProxy = repr ? repr->getProxy() : nullptr;
  QAction* parent_action = this->parentAction();

  // Set action state
  vtkSMProperty* colorProp = reprProxy ? reprProxy->GetProperty("UseSeparateColorMap") : nullptr;
  bool can_sep =
    reprProxy && colorProp && vtkSMColorMapEditorHelper::GetUsingScalarColoring(reprProxy);
  parent_action->setEnabled(can_sep);
  parent_action->setChecked(false);
  if (colorProp && reprProxy)
  {
    this->Links.addPropertyLink(
      parent_action, "checked", SIGNAL(toggled(bool)), reprProxy, colorProp);
  }
}

//-----------------------------------------------------------------------------
void pqUseSeparateColorMapReaction::onTriggered()
{
  // Disable Multi Components Mapping
  pqDataRepresentation* repr = this->CachedRepresentation.data();
  vtkSMRepresentationProxy* proxy = vtkSMRepresentationProxy::SafeDownCast(repr->getProxy());
  vtkSMProperty* mcmProperty = proxy->GetProperty("MultiComponentsMapping");
  if (vtkSMPropertyHelper(mcmProperty).GetAsInt() == 1)
  {
    vtkSMProperty* sepProperty = proxy->GetProperty("UseSeparateColorMap");
    if (vtkSMPropertyHelper(sepProperty).GetAsInt() == 0)
    {
      vtkSMPropertyHelper(mcmProperty).Set(0);
    }
  }

  pqView* view = pqActiveObjects::instance().activeView();
  vtkSMProxy* viewProxy = view ? view->getProxy() : nullptr;

  if (vtkSMColorMapEditorHelper::GetUsingScalarColoring(proxy))
  {
    if (vtkSMProperty* lutProperty = proxy->GetProperty("LookupTable"))
    {
      vtkSMPropertyHelper lutPropertyHelper(lutProperty);
      if (lutPropertyHelper.GetNumberOfElements() != 0 && lutPropertyHelper.GetAsProxy(0))
      {
        vtkSMProxy* lutProxy = lutPropertyHelper.GetAsProxy(0);

        vtkSMScalarBarWidgetRepresentationProxy* sbProxy =
          vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(
            vtkSMTransferFunctionProxy::FindScalarBarRepresentation(lutProxy, viewProxy));

        sbProxy->RemoveRange(proxy);
        sbProxy->UpdateVTKObjects();
      }
      else
      {
        qWarning("Failed to determine the LookupTable being used.");
      }
    }
    else
    {
      qWarning("Missing 'LookupTable' property");
    }
  }

  // Force color widget to update representation and color map
  Q_EMIT this->ColorWidget->arraySelectionChanged();
}
