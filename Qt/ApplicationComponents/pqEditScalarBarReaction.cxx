// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqEditScalarBarReaction.h"

#include "pqDataRepresentation.h"
#include "pqProxyWidgetDialog.h"
#include "pqScalarBarVisibilityReaction.h"

#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"

#include <QAction>
#include <QDialog>
#include <QObject>
#include <QPointer>

//-----------------------------------------------------------------------------
pqEditScalarBarReaction::pqEditScalarBarReaction(QAction* parentObject, bool track_active_objects)
  : Superclass(parentObject)
{
  this->setScalarBarVisibilityReaction(
    this->createDefaultScalarBarVisibilityReaction(track_active_objects));
}

//-----------------------------------------------------------------------------
pqEditScalarBarReaction::~pqEditScalarBarReaction() = default;

//-----------------------------------------------------------------------------
void pqEditScalarBarReaction::setRepresentation(
  pqDataRepresentation* repr, int selectedPropertiesType)
{
  this->SBVReaction->setRepresentation(repr, selectedPropertiesType);
}

//-----------------------------------------------------------------------------
QPointer<pqScalarBarVisibilityReaction>
pqEditScalarBarReaction::createDefaultScalarBarVisibilityReaction(bool track_active_objects)
{
  QAction* tmp = new QAction(this);
  return new pqScalarBarVisibilityReaction(tmp, track_active_objects);
}

//-----------------------------------------------------------------------------
void pqEditScalarBarReaction::setScalarBarVisibilityReaction(
  pqScalarBarVisibilityReaction* reaction)
{
  if (this->SBVReaction)
  {
    this->SBVReaction->parentAction()->disconnect(this);
    delete this->SBVReaction;
    this->SBVReaction = nullptr;
  }
  if (reaction)
  {
    this->SBVReaction = reaction;
    QObject::connect(reaction->parentAction(), &QAction::changed, this,
      &pqEditScalarBarReaction::updateEnableState);
    this->updateEnableState();
  }
}

//-----------------------------------------------------------------------------
void pqEditScalarBarReaction::updateEnableState()
{
  this->parentAction()->setEnabled(this->SBVReaction->parentAction()->isEnabled() &&
    this->SBVReaction->parentAction()->isChecked());
}

//-----------------------------------------------------------------------------
void pqEditScalarBarReaction::onTriggered()
{
  this->editScalarBar();
}

//-----------------------------------------------------------------------------
bool pqEditScalarBarReaction::editScalarBar()
{
  auto scalarBarProxies = this->SBVReaction->scalarBarProxies();
  if (scalarBarProxies.empty())
  {
    return false;
  }
  if (vtkSMProxy* sbProxy = scalarBarProxies[0])
  {
    pqDataRepresentation* repr = this->SBVReaction->representation();

    auto copyProxy = vtk::TakeSmartPointer(
      repr->proxyManager()->NewProxy(sbProxy->GetXMLGroup(), sbProxy->GetXMLName()));
    copyProxy->Copy(sbProxy);

    pqProxyWidgetDialog dialog(sbProxy);
    dialog.setWindowTitle(tr("Edit Color Legend Properties"));
    dialog.setObjectName("ColorLegendEditor");
    dialog.setEnableSearchBar(true);
    dialog.setSettingsKey("ColorLegendEditor");

    QObject::connect(
      &dialog, &pqProxyWidgetDialog::accepted, repr, &pqDataRepresentation::renderViewEventually);
    const bool accepted = dialog.exec() == QDialog::Accepted;
    if (accepted)
    {
      auto changedProperties = sbProxy->GetPropertiesWithDifferentValues(copyProxy);
      for (size_t i = 1; i < scalarBarProxies.size(); ++i)
      {
        const auto otherSBProxy = scalarBarProxies[i];
        if (otherSBProxy && otherSBProxy != sbProxy)
        {
          for (const auto& prop : changedProperties)
          {
            otherSBProxy->GetProperty(prop.c_str())->Copy(sbProxy->GetProperty(prop.c_str()));
          }
        }
      }
    }
    return accepted;
  }
  return false;
}
