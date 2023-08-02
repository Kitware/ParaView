// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqToggleInteractionViewMode.h"

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqRenderView.h"
#include "vtkPVRenderView.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMTrace.h"

//-----------------------------------------------------------------------------
pqToggleInteractionViewMode::pqToggleInteractionViewMode(QAction* parentObject, pqView* view)
  : Superclass(parentObject)
  , View(view)
{
  pqRenderView* renderView = qobject_cast<pqRenderView*>(this->View);
  if (renderView)
  {
    QObject::connect(
      view, SIGNAL(updateInteractionMode(int)), this, SLOT(updateInteractionLabel(int)));

    // Update label based on the current state (Needed when we load a state)
    int mode = -1;
    vtkSMPropertyHelper(view->getProxy(), "InteractionMode").Get(&mode);
    this->updateInteractionLabel(mode);
  }
}

//-----------------------------------------------------------------------------
void pqToggleInteractionViewMode::onTriggered()
{
  pqRenderView* view = qobject_cast<pqRenderView*>(this->View);
  SM_SCOPED_TRACE(PropertiesModified)
    .arg(view->getProxy())
    .arg("comment", "change interaction mode for render view");

  int currentMode = -1;
  int interactionMode = -1;
  vtkSMPropertyHelper(view->getProxy(), "InteractionMode").Get(&currentMode);
  if (currentMode == vtkPVRenderView::INTERACTION_MODE_3D)
  {
    interactionMode = vtkPVRenderView::INTERACTION_MODE_2D;
  }
  else
  {
    interactionMode = vtkPVRenderView::INTERACTION_MODE_3D;
  }

  // Update the interaction
  vtkSMPropertyHelper(view->getProxy(), "InteractionMode").Set(interactionMode);
  view->getProxy()->UpdateProperty("InteractionMode", 1);
  view->render();
}

//-----------------------------------------------------------------------------
void pqToggleInteractionViewMode::updateInteractionLabel(int mode)
{
  switch (mode)
  {
    case vtkPVRenderView::INTERACTION_MODE_2D:
      this->parentAction()->setIcon(QIcon(":/pqWidgets/Icons/pqInteractionMode2D.svg"));
      break;
    case vtkPVRenderView::INTERACTION_MODE_3D:
      this->parentAction()->setIcon(QIcon(":/pqWidgets/Icons/pqInteractionMode3D.svg"));
      break;
  }
}
