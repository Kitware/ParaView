/*=========================================================================

   Program: ParaView
   Module:    pqToggleInteractionViewMode.cxx

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
