/*=========================================================================

   Program: ParaView
   Module:    pqUseSeparateColorMapReaction.cxx

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
#include "pqUseSeparateColorMapReaction.h"

#include "pqActiveObjects.h"
#include "pqDataRepresentation.h"
#include "pqDisplayColorWidget.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"

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
    reprProxy && colorProp && vtkSMPVRepresentationProxy::GetUsingScalarColoring(reprProxy);
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
  vtkSMPVRepresentationProxy* proxy = vtkSMPVRepresentationProxy::SafeDownCast(repr->getProxy());
  vtkSMProperty* mcmProperty = proxy->GetProperty("MultiComponentsMapping");
  if (vtkSMPropertyHelper(mcmProperty).GetAsInt() == 1)
  {
    vtkSMProperty* sepProperty = proxy->GetProperty("UseSeparateColorMap");
    if (vtkSMPropertyHelper(sepProperty).GetAsInt() == 0)
    {
      vtkSMPropertyHelper(mcmProperty).Set(0);
    }
  }

  // Force color widget to update representation and color map
  Q_EMIT this->ColorWidget->arraySelectionChanged();
}
